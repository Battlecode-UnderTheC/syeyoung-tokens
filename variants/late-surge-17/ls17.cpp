#include "ls17.hpp"

#include <algorithm>
#include <cmath>
#include <tuple>
#include <functional>
#ifdef LS17_TUNE
#include <cstdlib>
#include <string>
#endif
#include <unordered_map>
#include <unordered_set>
#if defined(LS17_DEBUG) || defined(LS17_TRACE)
#include <atomic>
#include <cstdio>
#include <cstdlib>
#endif

namespace ls17 {
#ifdef LS17_DEBUG
static long g_escape = 0, g_turnout = 0, g_calls = 0;
#define LS17_COUNT(x) ++(x)
// idle-loop measurement: turns where the last 12 heads cover <= 6 tiles with nothing eaten
static std::atomic<long> g_turns{0}, g_idle{0}, g_eaten{0}, g_cross{0}, g_bans{0};
static struct IdleReport {
  ~IdleReport() {
    if (g_turns) std::fprintf(stderr, "LS17 turns %ld idle %ld (%.2f%%) eaten %ld crossings %ld bans %ld\n", g_turns.load(),
                              g_idle.load(), 100.0 * g_idle / g_turns, g_eaten.load(), g_cross.load(), g_bans.load());
  }
} g_idle_report;
#else
#define LS17_COUNT(x)
#endif

StrategyAgent::StrategyAgent(const rl::Spawn& spawn, Params params)
    : p_(params), spawn_(spawn), board_(spawn.width, spawn.height), portal_(spawn.width * spawn.height, 0) {
#ifdef LS17_TUNE
  // LS17_P="name=value,..." overrides parameters (tuning builds only)
  if (const char* env = std::getenv("LS17_P")) {
    std::string all(env);
    size_t pos = 0;
    while (pos < all.size()) {
      size_t end = all.find(',', pos);
      if (end == std::string::npos) end = all.size();
      const std::string kv = all.substr(pos, end - pos);
      pos = end + 1;
      const size_t eq = kv.find('=');
      if (eq == std::string::npos) continue;
      const std::string k = kv.substr(0, eq);
      const double v = std::atof(kv.c_str() + eq + 1);
#define LS17_SET(name) if (k == #name) p_.name = static_cast<decltype(p_.name)>(v);
      LS17_SET(split_min_length) LS17_SET(split_until_round) LS17_SET(feed_from_round) LS17_SET(trap_cost)
      LS17_SET(room_margin) LS17_SET(room_min) LS17_SET(escape_slack) LS17_SET(escape_min) LS17_SET(check_split)
      LS17_SET(liberties) LS17_SET(share_factor) LS17_SET(patrol) LS17_SET(hunt_radius) LS17_SET(explore_value)
      LS17_SET(escape_steps) LS17_SET(trap_small)
      LS17_SET(split_headroom) LS17_SET(turnout_budget) LS17_SET(doom_first) LS17_SET(ally_memory) LS17_SET(avoid_blind) LS17_SET(doomed_split_room) LS17_SET(leader_split_until) LS17_SET(share_leader) LS17_SET(feed_distance) LS17_SET(big_split_length)
      LS17_SET(portal_cross) LS17_SET(portal_explore) LS17_SET(loop_break) LS17_SET(loop_ban)
      LS17_SET(wait_factor) LS17_SET(cluster_bonus) LS17_SET(frontier_radius) LS17_SET(open_centre_until) LS17_SET(open_centre_value)
#undef LS17_SET
    }
  }
#endif
}

// ---- what this dragon sees ------------------------------------------------------------------------

void StrategyAgent::Remember(const rl::Observation& o) {
  board_.Update(o);
  head_ = board_.Tile(o.head_x(), o.head_y());
  length_ = o.length;
  units_ = o.unit_count;
  round_ = o.round;
  if (first_turn_) {
    leader_ = o.round == 0;
    born_ = o.round;                         // alive at the start: not a split child
    first_turn_ = false;
    for (const rl::Segment& s : o.segments)         // the part of the body in view, head to tail
      if (s.id == spawn_.id) body_.push_back(board_.Tile(s.x, s.y));
  }
  if (body_.empty() || body_.front() != head_) body_.push_front(head_);
  while (static_cast<int32_t>(body_.size()) > length_) body_.pop_back();
  std::unordered_map<int32_t, int32_t> segments;
  // dragons whose last segment in view is on the window's border: their body may go on out of view
  std::unordered_map<int32_t, bool> cut;
  auto on_border = [&](const rl::Segment& s) {
    int32_t dx = std::abs(s.x - o.head_x()), dy = std::abs(s.y - o.head_y());
    dx = std::min(dx, board_.width() - dx), dy = std::min(dy, board_.height() - dy);
    return dx >= 3 || dy >= 3;
  };
  for (const rl::Segment& s : o.segments) {
    if (s.id == spawn_.id) continue;
    ++segments[s.id];
    cut[s.id] = on_border(s);
    Sighting& w = seen_[s.id];
    w.team = s.team;
    w.round = round_;
    if (s.head) w.head = board_.Tile(s.x, s.y);
  }
  for (const auto& [id, n] : segments) seen_[id].length = n;
  for (const uint64_t w : o.inbox) Decode(w);
  WatchTrades(o);
  // deaths: an ally long in view, its head beside ours last turn, is gone (we moved one plain step)
  if (prev_head_ >= 0) {
    auto near = [&](int32_t a, int32_t b, int32_t r) {
      int32_t dx = std::abs(board_.X(a) - board_.X(b)), dy = std::abs(board_.Y(a) - board_.Y(b));
      dx = std::min(dx, board_.width() - dx), dy = std::min(dy, board_.height() - dy);
      return dx <= r && dy <= r;
    };
    const bool plain = [&] {
      int32_t dx = std::abs(board_.X(prev_head_) - board_.X(head_)), dy = std::abs(board_.Y(prev_head_) - board_.Y(head_));
      dx = std::min(dx, board_.width() - dx), dy = std::min(dy, board_.height() - dy);
      return dx + dy <= 1;
    }();
    if (plain)
      for (const auto& [id, w] : seen_)
        if (w.team == spawn_.team && w.round == round_ - 1 && w.head >= 0 && w.length >= p_.obit_min_length &&
            near(w.head, head_, 1) && !segments.count(id))
          Note(Report{id, w.head, 0, round_, false});
  }
#ifdef LS17_TRACE
  if (born_ == round_ && round_ > 0) std::fprintf(stderr, "CHILDDBG r%d heard%d\n", round_, int(reports_.size()));
#endif
  // allies in view: their head position now (their length only if the whole body is in view:
  // otherwise keep the reported one)
  for (const auto& [id, n] : segments) {
    const Sighting& w = seen_[id];
    if (w.team != spawn_.team || w.head < 0 || w.round != round_) continue;
    Report& have = reports_[id];
    const bool fresh = have.id >= 0 && round_ - have.round <= p_.report_memory;
    have.id = id;
    have.tile = w.head;
    if (p_.lower_bound_reports) {
      const bool part = cut[id] || !p_.trust_visible;
      have.length = part && have.length > n ? have.length : n;
      have.lower = part;
    } else {
      have.length = fresh && (cut[id] || !p_.trust_visible) ? std::max(have.length, n) : n;
    }
    have.round = round_;
  }
  {
    // we split most of our length off: tell the team we are no longer that long (the child
    // carries it under a new id), or our old report outlives us as a ghost champion
    const Report self{spawn_.id, head_, length_, round_};
    const Report& was = reports_[spawn_.id];
    if (was.id >= 0 && was.length >= p_.obit_champion && length_ * 2 < was.length) {
      obit_ = self;
      obit_until_ = round_ + p_.obit_rounds;
    }
  }
  reports_[spawn_.id] = Report{spawn_.id, head_, length_, round_};
  prev_head_ = head_;
  // portal edges in view (both tiles beside each)
  {
    constexpr int R = dragons::kVisionRadius, V = dragons::kVision;
    const int32_t hx = o.head_x(), hy = o.head_y();
    for (int r = 0; r <= V; ++r)
      for (int c = 0; c < V; ++c)
        if (o.hedge[r * V + c] >= 0) {   // north side of (x, y)
          portal_[board_.Tile(hx - R + c, hy - R + r)] |= 1 << 0;
          portal_[board_.Tile(hx - R + c, hy - R + r - 1)] |= 1 << 2;
        }
    for (int r = 0; r < V; ++r)
      for (int c = 0; c <= V; ++c)
        if (o.vedge[r * (V + 1) + c] >= 0) {   // west side of (x, y)
          portal_[board_.Tile(hx - R + c, hy - R + r)] |= 1 << 3;
          portal_[board_.Tile(hx - R + c - 1, hy - R + r)] |= 1 << 1;
        }
  }
  // last turn's ping through an unknown portal beside our new head: clean (no dragon) echo?
  exit_clear_ = false;
  if (ping_round_ == round_ - 1 && ping_tile_ == head_ && o.echo_present) {
    bool any = false;
    for (int k = 1; k < 5; ++k) any |= o.echo[k] != "0";   // ally, ally head, enemy, enemy head
    exit_clear_ = !any;
  }
  // the loop breaker's history
  if (length_ > last_length_ && round_ > born_) last_eat_ = round_;
  last_length_ = length_;
  recent_.push_back(head_);
  while (static_cast<int32_t>(recent_.size()) > p_.loop_window) recent_.pop_front();
}

bool StrategyAgent::Looping() const {
  if (static_cast<int32_t>(recent_.size()) < p_.loop_window || round_ - last_eat_ < p_.loop_window) return false;
  std::vector<int32_t> v(recent_.begin(), recent_.end());
  std::sort(v.begin(), v.end());
  return std::unique(v.begin(), v.end()) - v.begin() <= p_.loop_distinct;
}

bool StrategyAgent::BodyInView() const {
  if (static_cast<int32_t>(body_.size()) < length_) return false;
  for (const int32_t t : body_)
    if (!board_.Current(t)) return false;
  return true;
}

bool StrategyAgent::Mine(int32_t t) const { return std::find(body_.begin(), body_.end(), t) != body_.end(); }

bool StrategyAgent::Occupied(int32_t t) const { return Mine(t) || (board_.Current(t) && board_.at(t).occupant >= 0); }

StrategyAgent::Cell StrategyAgent::Classify(int32_t t) const {
  if (t == kBlocked) return Cell::kWall;
  if (t == kUnknown) return Cell::kUnknown;
  if (!Occupied(t)) return Cell::kFree;
  if (Mine(t)) return Cell::kBody;
  const TileMemory& m = board_.at(t);
  if (!m.occupant_head || m.occupant == spawn_.id) return Cell::kBody;
  return m.occupant_team == spawn_.team ? Cell::kAllyHead : Cell::kEnemyHead;
}

int32_t StrategyAgent::EnemiesSeen() const {
  int32_t n = 0;
  for (const auto& [id, w] : seen_)
    if (w.team != spawn_.team && round_ - w.round <= p_.enemy_memory) ++n;
  return n;
}

// Both die in a head-to-head. Worth it when theirs is the bigger loss, or (for a worker)
// when we clearly outnumber them; never with our last dragon. The enemies seen are a
// lower bound on their numbers, hence the margin.
bool StrategyAgent::WorthTrading(int32_t enemy_id) const {
  if (units_ <= 1) return false;
  const auto it = seen_.find(enemy_id);
  const int32_t theirs = it == seen_.end() ? 0 : it->second.length;
  if (leader_) return theirs > length_;
  // trades are one for one: when they have more dragons, even trades walk us to elimination
  if (p_.outnumbered_factor > 0 && units_ < EnemiesSeen()) return theirs >= p_.outnumbered_factor * length_;
  if (outnumbered_ && theirs <= length_) return false;   // late-surge-14: no even trades while outnumbered
  return theirs >= length_ + TradeMargin() ||
         (p_.outnumber_factor > 0 && units_ >= p_.hunt_min_units &&
          units_ >= p_.outnumber_factor * std::max(1, EnemiesSeen()));
}

// Is the enemy trading into us? Last turn an ally head (or ours) and an enemy head were
// adjacent and now both are gone from a spot we still see well: a head-to-head. Or an enemy
// head that was in view and not next to any of ours now is next to one: it stepped in.
void StrategyAgent::WatchTrades(const rl::Observation& o) {
  std::vector<HeadSeen> heads;
  std::unordered_set<int32_t> present;
  for (const rl::Segment& s : o.segments) {
    present.insert(s.id);
    if (s.head) heads.push_back({s.id, board_.Tile(s.x, s.y), s.team});
  }
  auto adjacent = [&](int32_t a, int32_t b) {
    for (int d = 0; d < 4; ++d)
      if (board_.Step(a, d) == b) return true;
    return false;
  };
  auto near_me = [&](int32_t t) {  // well inside the window now (Chebyshev <= 2, torus)
    int32_t dx = std::abs(board_.X(t) - board_.X(head_)), dy = std::abs(board_.Y(t) - board_.Y(head_));
    dx = std::min(dx, board_.width() - dx);
    dy = std::min(dy, board_.height() - dy);
    return dx <= 2 && dy <= 2;
  };
  if (prev_round_ == round_ - 1 && round_ < p_.feed_from_round) {
    for (const HeadSeen& a : prev_heads_) {
      if (a.team != spawn_.team || present.count(a.id)) continue;
      for (const HeadSeen& e : prev_heads_)
        if (e.team != spawn_.team && !present.count(e.id) && adjacent(a.tile, e.tile) && near_me(a.tile) &&
            near_me(e.tile)) {
          ++trades_seen_;
          break;
        }
    }
    for (const HeadSeen& e : heads) {
      if (e.team == spawn_.team) continue;
      bool next_now = false;
      for (const HeadSeen& a : heads)
        if (a.team == spawn_.team && adjacent(a.tile, e.tile)) next_now = true;
      if (!next_now) continue;
      for (const HeadSeen& pe : prev_heads_) {
        if (pe.id != e.id || pe.tile == e.tile) continue;
        bool next_before = false;
        for (const HeadSeen& a : prev_heads_)
          if (a.team == spawn_.team && adjacent(a.tile, pe.tile)) next_before = true;
        if (!next_before) ++approaches_seen_;
      }
    }
    if (trades_seen_ >= p_.trader_trades || approaches_seen_ >= p_.trader_approaches) trader_ = true;
  }
  prev_heads_ = std::move(heads);
  prev_round_ = round_;
}

void StrategyAgent::UpdateOutnumbered() {
  outnumbered_ = false;
  if (!p_.om_mode || round_ >= p_.om_until) return;
  int32_t n = 0;
  for (const auto& [id, w] : seen_)
    if (w.team != spawn_.team && w.round >= 0 && round_ - w.round <= p_.om_memory) {
      ++n;
      if (first_enemy_round_ < 0 || w.round < first_enemy_round_) first_enemy_round_ = w.round;
    }
  if (n >= p_.om_min_enemies && n * 10 > p_.om_ratio_tenths * units_) outnumbered_ = true;
  if (SmallMap() && first_enemy_round_ >= 0 && first_enemy_round_ <= p_.om_early_round && n > units_) outnumbered_ = true;
}

bool StrategyAgent::ExposedNear(int32_t t) const {
  if (Exposed(t)) return true;
  if (!outnumbered_ || p_.om_dodge < 2) return false;
  for (int d = 0; d < 4; ++d) {
    const int32_t n = board_.Step(t, d);
    if (n < 0 || Classify(n) == Cell::kWall) continue;
    for (int e = 0; e < 4; ++e)
      if (const int32_t m = board_.Step(n, e); m >= 0 && m != t && Classify(m) == Cell::kEnemyHead &&
                                              !WorthTrading(board_.at(m).occupant))
        return true;
  }
  return false;
}

bool StrategyAgent::Exposed(int32_t t) const {
  if (!leader_ && !p_.avoid_heads) return false;
  for (int d = 0; d < 4; ++d)
    if (const int32_t n = board_.Step(t, d); n >= 0 && Classify(n) == Cell::kEnemyHead && !WorthTrading(board_.at(n).occupant))
      return true;
  return false;
}

// Torus distance to the nearest enemy head seen in the last few rounds, for each step.
int StrategyAgent::FleeDirection() const {
  std::vector<int32_t> heads;
  for (const auto& [id, w] : seen_)
    if (w.team != spawn_.team && w.head >= 0 && round_ - w.round <= 3) heads.push_back(w.head);
  if (heads.empty()) return -1;
  auto distance = [&](int32_t a, int32_t b) {
    const int32_t dx = std::abs(board_.X(a) - board_.X(b)), dy = std::abs(board_.Y(a) - board_.Y(b));
    return std::min(dx, board_.width() - dx) + std::min(dy, board_.height() - dy);
  };
  int best = -1, best_far = -1;
  for (int d = 0; d < 4; ++d) {
    const int32_t to = board_.Step(head_, d);
    if (Classify(to) != Cell::kFree) continue;
    int32_t nearest = 1 << 30;
    for (const int32_t h : heads) nearest = std::min(nearest, distance(to, h));
    if (nearest > best_far) {
      best_far = nearest;
      best = d;
    }
  }
  return best;
}

// ---- sonar: who the champion is ----------------------------------------------------------------------
// 64-bit word: magic (8) | team (1) | id (12) | x (6) | y (6) | length (12) | round (9)
static constexpr uint64_t kMagic = 0xA7;

uint64_t StrategyAgent::Encode(const Report& r) const {
  const uint64_t x = board_.X(r.tile) & 63, y = board_.Y(r.tile) & 63;
  return (kMagic << 56) | (uint64_t(spawn_.team & 1) << 55) | (uint64_t(r.id & 4095) << 43) | (x << 37) | (y << 31) |
         (uint64_t(std::min(r.length, 4095)) << 19) | (uint64_t(r.round & 511) << 10) | uint64_t(r.lower ? 1 : 0) | (trader_ ? 2u : 0u);
}

void StrategyAgent::Decode(uint64_t w) {
  if ((w >> 56) != kMagic || int32_t((w >> 55) & 1) != (spawn_.team & 1) || (w & 1020) != 0) return;
  // bit 0: lower-bound flag; bit 1: trader flag (late-surge); bits 2-9 must be zero
  if (w & 2) trader_ = true;
  Report r;
  r.id = int32_t((w >> 43) & 4095);
  const int32_t x = int32_t((w >> 37) & 63), y = int32_t((w >> 31) & 63);
  if (x >= board_.width() || y >= board_.height()) return;
  r.tile = board_.Tile(x, y);
  r.length = int32_t((w >> 19) & 4095);
  r.round = int32_t((w >> 10) & 511);
  r.lower = (w & 1) != 0;
  if (r.round > round_ || r.id == spawn_.id) return;
  Note(r);
}

void StrategyAgent::Note(const Report& r) {
  Report& have = reports_[r.id];
  if (r.length * 2 < have.length && !r.lower && r.round > have.round && have.length >= p_.obit_champion) {
    obit_ = r;
    obit_until_ = round_ + p_.obit_rounds;
#ifdef LS17_TRACE
    std::fprintf(stderr, "OBITDBG r%d me%d dead%d len%d src%d\n", round_, spawn_.id, r.id, have.length, int(r.round == round_));
#endif
  }
  if (r.round > have.round || (r.round == have.round && r.length > have.length)) {
    if (p_.lower_bound_reports && r.lower && have.id >= 0 && have.length > r.length) {
      have.tile = r.tile;   // it is still at least as long as we knew
      have.round = r.round;
      have.lower = true;
    } else {
      have = r;
    }
  }
}

StrategyAgent::Report StrategyAgent::Champion() const {
  // the longest, less a little per round since it was last heard of: a champion that died
  // (or split its mass off into a child out of a trap, leaving a stub under its id) is only
  // ever relayed with its last round, so its report fades and the heir takes over
  Report best;
  double best_score = 0;
  const bool late = round_ >= p_.late_report_from;
  const int32_t memory = late ? p_.late_report_memory : p_.report_memory;
  const double fade = late ? p_.late_report_fade : p_.report_fade;
  for (const auto& [id, r] : reports_) {
    if (round_ - r.round > memory) continue;
    const double score = r.length - fade * (round_ - r.round);
    if (best.id < 0 || score > best_score || (score == best_score && r.id < best.id)) {
      best = r;
      best_score = score;
    }
  }
  return best;
}

// ---- where to go --------------------------------------------------------------------------------

// A pearl seen there is worth 1, discounted by how long ago (someone may have eaten it);
// a bed is worth 1 once its countdown runs out (rounds until then: *ready), a bed that
// ran out while out of view a bit less. Occupied tiles and unseen ones are worth nothing.
double StrategyAgent::TileValue(int32_t t, int32_t dist, int32_t* ready) const {
  const TileMemory& m = board_.at(t);
  *ready = dist;
  if (m.seen_round < 0 || Occupied(t)) return 0.0;
  const int32_t age = round_ - m.seen_round;
  if (m.pearl) return std::pow(p_.pearl_decay, age);
  if (m.countdown < 0) return 0.0;                         // not a bed
  const int32_t due = m.countdown - age;                   // rounds until it grows a pearl
  if (due > 0) {
    *ready = p_.wait_factor > 0 ? dist + static_cast<int32_t>(p_.wait_factor * std::max(0, due - dist))
                                : std::max(dist, due);
    return 1.0;
  }
  return age == 0 ? 1.0 : p_.ripe_bed_value;               // due now (blocked while occupied)
}

int32_t StrategyAgent::RoomNeeded() const { return std::max(length_ + p_.room_margin, p_.room_min); }

StrategyAgent::Goal StrategyAgent::ChooseGoal(const Paths& paths) const {
  // hunting: a worker with dragons to spare goes for enemy heads it just saw
  if (!leader_ && HuntRadius() > 0 && units_ >= p_.hunt_min_units && round_ < p_.feed_from_round) {
    int32_t best = -1;
    for (const auto& [id, w] : seen_)
      if (w.team != spawn_.team && w.round == round_ && w.head >= 0 && paths.dist[w.head] > 0 &&
          paths.dist[w.head] <= HuntRadius() && WorthTrading(id) &&
          (best < 0 || paths.dist[w.head] < paths.dist[best]))
        best = w.head;
    if (best >= 0) return {best, false};
  }
  // every reachable tile scored by value per round to collect it; the unexplored edge too
  struct Candidate {
    int32_t tile;
    double score;
  };
  std::vector<Candidate> candidates;
  // allies in view now: a tile one of them is nearer to is probably theirs
  std::vector<int32_t> allies;
  const bool champion = round_ >= p_.feed_from_round && Champion().id == spawn_.id;
  if (p_.share_factor < 1.0 && (!leader_ || p_.share_leader) && !champion)
    for (const auto& [id, w] : seen_)
      if (w.team == spawn_.team && w.round == round_ && w.head >= 0) allies.push_back(w.head);
  // enemy heads in view now, with their ids: a pearl or bed one of them gets to first is theirs.
  // Turn order (id order) settles ties only at a bed's tick: both waiting next to it, the
  // lower id eats the pearl. For a pearl already there we move first from now, so we win ties.
  std::vector<std::pair<int32_t, int32_t>> enemies;  // (head tile, id)
  if (p_.contest_factor < 1.0)
    for (const auto& [id, w] : seen_)
      if (w.team != spawn_.team && w.round == round_ && w.head >= 0) enemies.push_back({w.head, id});
  auto torus = [&](int32_t a, int32_t b) {
    const int32_t dx = std::abs(board_.X(a) - board_.X(b)), dy = std::abs(board_.Y(a) - board_.Y(b));
    return std::min(dx, board_.width() - dx) + std::min(dy, board_.height() - dy);
  };
  // late-surge-17: each tile's value and rounds until ripe, once (for the cluster bonus)
  std::vector<double> base;
  std::vector<int32_t> due0;
  if (p_.cluster_bonus > 0) {
    base.assign(board_.size(), 0.0);
    due0.assign(board_.size(), 0);
    for (int32_t t = 0; t < board_.size(); ++t) {
      int32_t r;
      base[t] = TileValue(t, 0, &r);
      const TileMemory& m = board_.at(t);
      if (base[t] > 0 && !m.pearl) due0[t] = std::max(0, m.countdown - (round_ - m.seen_round));
    }
  }
  for (int32_t t = 0; t < board_.size(); ++t) {
    const int32_t d = paths.dist[t];
    if (d <= 0) continue;
    int32_t ready;
    double value = TileValue(t, d, &ready);
    if (value > 0 && p_.cluster_bonus > 0) {
      // late-surge-17: a pearl in a field is worth more than a lone one
      double near = 0;
      const int32_t R = p_.cluster_radius, x = board_.X(t), y = board_.Y(t);
      for (int32_t dy = -R; dy <= R; ++dy)
        for (int32_t dx = -R + std::abs(dy); dx <= R - std::abs(dy); ++dx) {
          if (dx == 0 && dy == 0) continue;
          const int32_t n = board_.Tile(x + dx, y + dy);
          const double v = base[n];
          if (v <= 0) continue;
          const int32_t dn = d + std::abs(dx) + std::abs(dy);
          const int32_t r2 = dn + static_cast<int32_t>(p_.wait_factor * std::max(0, due0[n] - dn));
          near += v * (d + 1.0) / (r2 + 1.0);   // a bed far from ripe adds little
        }
      value += p_.cluster_bonus * near;
    }
    double score = value / (ready + 1);
    if (score > 0)
      for (const int32_t a : allies)
        if (torus(a, t) < d) {
          score *= p_.share_factor;
          break;
        }
    if (score > 0 && !enemies.empty()) {
      const TileMemory& m = board_.at(t);
      const int32_t due = m.pearl ? 0 : std::max(0, m.countdown - (round_ - m.seen_round));
      for (const auto& [e, id] : enemies) {
        const int32_t theirs = std::max(torus(e, t), due);
        if (theirs < ready || (theirs == ready && due > 0 && ready == due && id < spawn_.id)) {
          score *= p_.contest_factor;
          break;
        }
      }
    }
    for (int dir = 0; dir < 4; ++dir)
      if (board_.Step(t, dir) == kUnknown && !(p_.portal_explore && (portal_[t] >> dir & 1) && !WillCross())) {
        double w = 1.0;
        if (p_.frontier_radius > 0) {
          // late-surge-17: an edge with much never-seen area behind it is worth more
          int32_t unseen = 0, total = 0;
          const int32_t R = p_.frontier_radius, x = board_.X(t), y = board_.Y(t);
          for (int32_t dy = -R; dy <= R; ++dy)
            for (int32_t dx = -R + std::abs(dy); dx <= R - std::abs(dy); ++dx) {
              ++total;
              if (board_.at(board_.Tile(x + dx, y + dy)).seen_round < 0) ++unseen;
            }
          w = p_.frontier_floor + (1.0 - p_.frontier_floor) * unseen / total;
        }
        score = std::max(score, p_.explore_value * w / (d + 1));
        break;
      }
    // late-surge-14: the opening centre race on small maps
    if (round_ < p_.centre_until && SmallMap()) {
      const int32_t centre = board_.Tile(board_.width() / 2, board_.height() / 2);
      score = std::max(score, p_.centre_value / (d + 1 + torus(t, centre)));
    } else if (round_ < p_.open_centre_until && p_.open_centre_value > 0) {
      // late-surge-17: every map, the opening leans towards the middle
      const int32_t centre = board_.Tile(board_.width() / 2, board_.height() / 2);
      score = std::max(score, p_.open_centre_value / (d + 1 + torus(t, centre)));
    }
    if (score > 0 && !Banned(t)) candidates.push_back({t, score});
  }
  const size_t k = std::min<size_t>(candidates.size(), p_.trap_candidates);
  std::partial_sort(candidates.begin(), candidates.begin() + k, candidates.end(),
                    [](const Candidate& a, const Candidate& b) { return a.score > b.score; });
  // the best few, checked for dead ends: a dead end is worth its pocket's pearls less the
  // cost of splitting out of it
  const int32_t need = RoomNeeded();
  const bool no_trap = p_.cap_safety && length_ >= 4 && !CanCountOnSplit();
  auto open = [&](int32_t t) { return !Occupied(t); };
  Goal best;
  double best_score = 0;
  std::vector<int32_t> pocket;
  for (size_t i = 0; i < k; ++i) {
    const Candidate& c = candidates[i];
    double score = c.score;
    bool trap = false;
    if (RoomAround(board_, c.tile, open, need, &pocket) < need) {
      if (no_trap) continue;                          // near the cap: a dead end is a grave
      double gain = 0;
      for (const int32_t t : pocket) {
        int32_t ready;
        gain += TileValue(t, paths.dist[t] < 0 ? paths.dist[c.tile] : paths.dist[t], &ready);
      }
      score = (gain - p_.trap_cost) / (paths.dist[c.tile] + 1);
      trap = true;
    }
    if (score > best_score) {
      best_score = score;
      best = {c.tile, trap};
    }
  }
  // nothing worth going for: patrol, towards what we have not seen for longest
  if (best.tile < 0 && p_.patrol) {
    double top = 0;
    for (int32_t t = 0; t < board_.size(); ++t) {
      const int32_t d = paths.dist[t];
      if (d <= 0 || Occupied(t) || Banned(t)) continue;
      const double score = (round_ - board_.at(t).seen_round) / (d + 4.0);
      if (score > top) {
        top = score;
        best = {t, false};
      }
    }
  }
  return best;
}

// ---- how to step ----------------------------------------------------------------------------------

#ifdef LS17_DEBUG
void StrategyAgent::Dump(const char* why) const {
  std::fprintf(stderr, "== id %d len %d round %d born %d units %d leader %d: %s (last: %s)\n", spawn_.id, length_, round_, born_, units_, leader_, why, reason_);
  const int R = 5;
  for (int dy = -R; dy <= R; ++dy) {
    std::string row, below;
    for (int dx = -R; dx <= R; ++dx) {
      const int32_t t = board_.Tile(board_.X(head_) + dx, board_.Y(head_) + dy);
      char c = '.';
      const TileMemory& m = board_.at(t);
      if (m.seen_round < 0) c = ' ';
      else if (t == head_) c = 'H';
      else if (Mine(t)) c = 'b';
      else if (board_.Current(t) && m.occupant >= 0) c = m.occupant_team == spawn_.team ? (m.occupant_head ? 'A' : 'a') : (m.occupant_head ? 'E' : 'e');
      else if (m.pearl) c = 'o';
      row += c;
      const int32_t e = board_.Step(t, 1), sdn = board_.Step(t, 2);
      row += e == kBlocked ? '|' : e == kUnknown ? ':' : (e != board_.Tile(board_.X(t) + 1, board_.Y(t)) ? 'p' : ' ');
      below += sdn == kBlocked ? '-' : sdn == kUnknown ? '~' : (sdn != board_.Tile(board_.X(t), board_.Y(t) + 1) ? 'p' : ' ');
      below += ' ';
    }
    std::fprintf(stderr, "%s\n%s\n", row.c_str(), below.c_str());
  }
}
#endif

// Exact search for a way out of a small region: the dragon's own moves, its body simulated
// (other dragons stand still). 2: it gets back onto a tile its body left, or to the unexplored
// edge; 0: every line of play dies within max_depth moves; 1: out of budget.
int StrategyAgent::TurnOut(const std::vector<int32_t>& body, int32_t max_depth) const {
  std::vector<uint8_t> orig(board_.size(), 0), occ(board_.size(), 0);
  for (const int32_t t : body) orig[t] = 1, ++occ[t];
  std::deque<int32_t> snake(body.begin(), body.end());
  int32_t budget = p_.turnout_budget;
  // returns 2 success, 0 dead, 1 budget
  std::function<int(int32_t)> go = [&](int32_t depth) -> int {
    if (depth >= max_depth) return 0;
    int result = 0;
    const int32_t head = snake.front();
    for (int d = 0; d < 4; ++d) {
      const int32_t n = board_.Step(head, d);
      if (n == kUnknown) return 2;
      if (n < 0 || occ[n]) continue;
      if (board_.Current(n) && board_.at(n).occupant >= 0 && board_.at(n).occupant != spawn_.id) continue;
      if (orig[n]) return 2;
      if (--budget < 0) return 1;
      LS17_COUNT(g_turnout);
      const int32_t tail = snake.back();
      snake.pop_back();
      --occ[tail];
      snake.push_front(n);
      ++occ[n];
      const int r = go(depth + 1);
      snake.pop_front();
      --occ[n];
      snake.push_back(tail);
      ++occ[tail];
      if (r == 2) return 2;
      result = std::max(result, r);
    }
    return result;
  };
  return go(0);
}

// Both halves of a split of the last n segments can get out.
bool StrategyAgent::SplitSafe(int32_t n) const {
  if (static_cast<int32_t>(body_.size()) < length_) return true;   // body not fully known
  std::vector<int32_t> parent(body_.begin(), body_.end() - n), child(body_.rbegin(), body_.rbegin() + n);
  return EscapeLevel(child, EscapeCap(n), nullptr, &parent) >= 1 &&
         EscapeLevel(parent, EscapeCap(length_ - n), nullptr, &child) >= 1;
}

// After splitting off the last n segments our new tail is body[L-n-1]; the ping opposite the
// facing starts there and runs from body[L-n-2] through it. The child's tail is body[L-n]: it
// hears us only if the body runs straight through the cut.
bool StrategyAgent::HeardSplit(int32_t n) const {
  if (static_cast<int32_t>(body_.size()) < length_ || n < 2 || n > length_ - 2) return false;
  const int32_t i = length_ - n - 2;
  const int32_t a = body_[i], b = body_[i + 1], c = body_[i + 2];
  for (int d = 0; d < 4; ++d)
    if (board_.Step(a, d) == b) return board_.Step(b, d) == c;
  return false;
}

int32_t StrategyAgent::GrowthSplit() const {
  const int32_t half = length_ / 2;
  if (!p_.check_split) return half;
  if (p_.heard_split)
    for (const int32_t n : {half, half + 1, half - 1})
      if (n >= 2 && n <= length_ - 2 && HeardSplit(n) && SplitSafe(n)) return n;
  return SplitSafe(half) ? half : -1;
}

// The next turn's options after stepping onto `to` (body after the step: `body`):
// 2 a free neighbour no other head touches, 1 only free neighbours another head could take
// first, 0 none.
int StrategyAgent::Liberties(int32_t to, const std::vector<int32_t>& body) const {
  int libs = 0;
  for (int d = 0; d < 4; ++d) {
    const int32_t n = board_.Step(to, d);
    if (n == kUnknown) return 2;
    if (n < 0 || std::find(body.begin(), body.end(), n) != body.end()) continue;
    if (board_.Current(n) && board_.at(n).occupant >= 0) continue;
    bool contested = false;
    for (int e = 0; e < 4 && !contested; ++e) {
      const int32_t m = board_.Step(n, e);
      contested = m >= 0 && m != to && board_.Current(m) && board_.at(m).occupant >= 0 && board_.at(m).occupant_head &&
                  board_.at(m).occupant != spawn_.id;
    }
    libs = std::max(libs, contested ? 1 : 2);
  }
  return libs;
}

int StrategyAgent::EscapeLevel(const std::vector<int32_t>& body, int32_t cap, int32_t* count,
                               const std::vector<int32_t>* blocked) const {
  const int32_t n = board_.size(), len = static_cast<int32_t>(body.size());
  std::vector<int32_t> index(n, -1);                   // own segment index, head 0
  if (blocked)
    for (const int32_t t : *blocked) index[t] = 1 << 20;   // never freed in time
  for (int32_t i = len - 1; i >= 0; --i) index[body[i]] = i;
  std::vector<int32_t> when(n, -1);                    // time a tile was entered
  std::vector<int32_t> queue{body[0]};
  when[body[0]] = 0;
  int32_t reached = 1;
  auto done = [&](int level) {
    if (count) *count = std::min(reached, cap);
    return level;
  };
  LS17_COUNT(g_calls);
  for (size_t qi = 0; qi < queue.size(); ++qi) {
    LS17_COUNT(g_escape);
    const int32_t t = queue[qi], m = when[t] + 1;     // the move that would enter a neighbour
    for (int d = 0; d < 4; ++d) {
      const int32_t to = board_.Step(t, d);
      if (to == kUnknown) return done(2);
      if (to < 0 || when[to] >= 0) continue;
      if (const int32_t i = index[to]; i >= 0) {
        if (i < len && i >= len - m + 1 && i > 0) return done(2);   // freed by then: follow the tail
        continue;                                      // still there (maybe later via a longer way)
      }
      if (board_.Current(to) && board_.at(to).occupant >= 0 && board_.at(to).occupant != spawn_.id) continue;
      when[to] = m;
      if (++reached >= cap) return done(1);
      queue.push_back(to);
    }
  }
  return done(0);
}

bool StrategyAgent::CanSplitNow() const {
  if (length_ < 4 || FreeSlots() < 1) return false;
  return length_ >= p_.reserve_length || FreeSlots() > p_.reserve_slots;
}

bool StrategyAgent::CanCountOnSplit() const {
  if (length_ < 4) return false;
  if (!p_.cap_safety) return true;
  const int32_t reserve = length_ >= p_.reserve_length ? 0 : p_.reserve_slots;
  return FreeSlots() >= p_.cap_margin + reserve;
}

const char* StrategyAgent::Bucket(const char* base) {
  const char* len = length_ <= 3 ? "L2-3" : length_ <= 5 ? "L4-5" : length_ <= 9 ? "L6-9" : "L10+";
  const char* cap = FreeSlots() <= 1 ? " @cap" : "";
  std::snprintf(bucket_, sizeof bucket_, "%s %s%s", base, len, cap);
  return bucket_;
}

bot::Action StrategyAgent::SafeStep(int preferred, bool trap_ok) {
  // near the cap a long dragon can't split out of a dead end: plan like a short one
  const bool can_split_out = CanCountOnSplit();
  if (p_.cap_safety && length_ >= 4 && !can_split_out) trap_ok = false;
  const int32_t need = RoomNeeded(), cap = EscapeCap(length_);
  // room to live in: free tiles, not counting those an enemy head can take next turn
  auto open = [&](int32_t t) { return !Occupied(t) && !Exposed(t); };
  std::vector<int32_t> body(body_.begin(), body_.end());
  body.insert(body.begin(), -1);
  if (static_cast<int32_t>(body.size()) > length_) body.pop_back();
  int best = -1, best_level = -1, best_count = -1, best_room = -1, best_libs = -1;
  bool best_exposed = true;
  for (int k = 0; k < 4; ++k) {
    const int d = ((preferred < 0 ? 0 : preferred) + k) % 4;   // the preferred direction first
    const int32_t to = board_.Step(head_, d);
    if (Classify(to) != Cell::kFree) continue;
    const int32_t room = RoomAround(board_, to, open, need);
    const bool exposed = ExposedNear(to);
    body[0] = to;
    int32_t count = 0;
    int level = p_.escape_steps ? EscapeLevel(body, cap, &count) : 1;
    if (level == 0 && p_.turnout_budget > 0) level = TurnOut(body, count + length_ + 2) == 0 ? 0 : 2;
    int libs = p_.liberties ? Liberties(to, body) : 2;
    // through a portal to a tile out of view: someone may be lying there
    const bool blind = p_.avoid_blind && !board_.Current(to);
    if (blind) libs = std::min(libs, 1);
    if (k == 0 && preferred >= 0 && !exposed && libs >= 2 && ((level >= 1 && room >= need) || (trap_ok && level >= 1)))
      return Because(room >= need ? "toward goal" : "into a dead end worth it", bot::Action::Move(d));
    if (k == 0 && preferred >= 0 && !exposed && libs >= 2 && trap_ok && (can_split_out || !p_.trap_small))
      return Because("into a dead end worth it", bot::Action::Move(d));
    // a sure death in a dead end is worse than a tile an enemy head might take (that would be a trade)
    auto key = [&](bool e, int lv, int lb, int c, int r) {
      return std::make_tuple(p_.doom_first ? lv > 0 : true, !e, lv, lb, c, r);
    };
    if (best < 0 || key(exposed, level, libs, count, room) > key(best_exposed, best_level, best_libs, best_count, best_room)) {
      best = d;
      best_level = level;
      best_count = count;
      best_room = room;
      best_libs = libs;
      best_exposed = exposed;
    }
  }
  // every step is a dead end we can't turn round in: split while we still can (the child
  // walks out the way we came)
  if (best >= 0 && best_level == 0 && best_count <= p_.doomed_split_room && CanSplitNow())
    return Because("doomed: split", bot::Action::Split(length_ - 2));
  if (best >= 0) return Because(preferred >= 0 ? "detour (goal step unsafe)" : "no goal: roomiest", bot::Action::Move(best));
  // no free step. Split: the child's head is our tail, which points back the way we came
  // (usually open), so the child walks out with most of the length; the stub stays behind
  if (CanSplitNow()) {
    // a long stub may keep one more segment so that the child hears the champion from it
    const int32_t n = p_.heard_split && length_ >= 8 && !HeardSplit(length_ - 2) && HeardSplit(length_ - 3) ? length_ - 3 : length_ - 2;
    return Because(Bucket("trapped: split"), bot::Action::Split(n));
  }
  // else trade with an adjacent enemy head, else step through an unexplored portal
  for (int d = 0; d < 4; ++d)
    if (Classify(board_.Step(head_, d)) == Cell::kEnemyHead) return Because("trapped: trade", bot::Action::Move(d));
  for (int d = 0; d < 4; ++d)
    if (Classify(board_.Step(head_, d)) == Cell::kUnknown) return Because("trapped: unknown portal", bot::Action::Move(d));
#ifdef LS17_DEBUG
  Dump("trapped: no move");
#endif
  return Because(Bucket("trapped: no move"), bot::Action::Resign());
}

// ---- the lunge (ported from swarm-hunt-2) -------------------------------------------------------

// Breadth-first over free tiles, at most length-1 steps (every step after the first costs a
// tail segment): the nearest enemy head worth a trade. Nothing else moves during our turn.
bool StrategyAgent::Lunge(bot::Action* out) const {
  const int32_t reach = std::min<int32_t>(length_ - 1, p_.lunge_reach);
  if (reach < 2) return false;   // one step is the plain kamikaze
  std::unordered_map<int32_t, std::pair<int32_t, int8_t>> from;
  std::vector<std::pair<int32_t, int32_t>> frontier{{head_, 0}};
  from[head_] = {-1, -1};
  for (size_t q = 0; q < frontier.size() && q < 256; ++q) {
    const auto [t, dist] = frontier[q];
    if (dist >= reach) continue;
    for (int d = 0; d < 4; ++d) {
      const int32_t n = board_.Step(t, d);
      if (n < 0 || from.count(n)) continue;
      const Cell c = Classify(n);
      if (c == Cell::kEnemyHead && WorthTrading(board_.at(n).occupant)) {
        std::vector<int> steps{d};
        for (int32_t u = t; from.at(u).first >= 0; u = from.at(u).first) steps.push_back(from.at(u).second);
        std::reverse(steps.begin(), steps.end());
        *out = bot::Action{bot::Action::kMove, steps, 0, false, {}};
        return true;
      }
      if (c != Cell::kFree || !board_.Current(n)) continue;
      from[n] = {t, static_cast<int8_t>(d)};
      frontier.push_back({n, dist + 1});
    }
  }
  return false;
}

// ---- the turn --------------------------------------------------------------------------------------

bot::Action StrategyAgent::Turn(const rl::Observation& o) {
  bot::Action a = Decide(o);
  // tell the dragons around us who the champion is (a dead dragon's pings are not cast)
  const Report champ = Champion();
  if (champ.id >= 0)
    for (int d = 0; d < 4; ++d) a.sonar[d] = Encode(champ);
  if (obit_until_ > round_ && (round_ & 1))
    for (int d = 0; d < 4; ++d) a.sonar[d] = Encode(obit_);
#ifdef LS17_TRACE
  if (a.kind == bot::Action::kSplit)
    std::fprintf(stderr, "SPLITDBG known%d heard%d %s\n", int(static_cast<int32_t>(body_.size()) >= length_), int(HeardSplit(a.split)), reason_);
  if (round_ == 400 || round_ == 460 || (round_ > 400 && round_ % 10 == 0))
    std::fprintf(stderr, "CHAMPDBG r%d t%d id%d len%d champ%d\n", round_, spawn_.team, spawn_.id, length_, champ.id);
#endif
  // stepping next to an unknown portal: ping only through it, so next turn's echoes say
  // whether a dragon lies past its exit
  if (WillCross() && a.kind == bot::Action::kMove && !a.resign && a.steps.size() == 1) {
    const int d = a.steps[0];
    const int32_t n = board_.Step(head_, d);
    if (n >= 0 && board_.Current(n))
      for (int e = 0; e < 4; ++e)
        if (e != (d + 2) % 4 && UnknownPortal(n, e)) {
          a.sonar = {};
          a.sonar[e] = champ.id >= 0 ? Encode(champ) : 1;
          ping_round_ = round_;
          ping_tile_ = n;
          ping_dir_ = e;
          break;
        }
  }
#ifdef LS17_DEBUG
  ++g_turns;
  if (Looping()) ++g_idle;
  if (length_ > 0 && round_ == last_eat_ && round_ > born_) ++g_eaten;
  if (std::string(reason_) == "portal: explore") ++g_cross;
#endif
  return a;
}

bot::Action StrategyAgent::Decide(const rl::Observation& o) {
  Remember(o);
  UpdateOutnumbered();

  // 1. feeding: late in the game every dragon but the champion walks to it and dies next to
  // it, leaving it pearls (a leader only near the end, and only when it knows where it is)
  const Report champ = Champion();
  const bool feeding = champ.id >= 0 && champ.id != spawn_.id && champ.length >= length_ &&
                       (leader_ ? round_ >= p_.leader_feed_from && round_ - champ.round <= p_.leader_feed_fresh
                                : round_ >= p_.feed_from_round);
  int32_t ally_head = -1;
  bool in_view = false;                              // only die next to it when we see it now
  bool held = false;
  if (feeding) {
    if (const auto it = seen_.find(champ.id); it != seen_.end() && it->second.round == round_ && it->second.head >= 0) {
      ally_head = it->second.head;
      in_view = true;
      // (late-surge-6) only feed a target clearly longer than us, by its length as known this turn
      const auto rit = reports_.find(champ.id);
      const int32_t tl = rit != reports_.end() ? rit->second.length : champ.length;
      if (length_ * 10 >= tl * p_.feed_clear_tenths) {
        ally_head = -1;                              // hold: keep gathering instead
        in_view = false;
        held = true;
      }
    } else if (champ.tile != head_ && !board_.Current(champ.tile)) {
      ally_head = champ.tile;                        // where it was last reported (out of view)
    }
  }
  const Paths paths = Search(board_, head_, [&](int32_t t) {
    return !Occupied(t) || Classify(t) == Cell::kEnemyHead || t == ally_head;
  });
  if (in_view && paths.dist[ally_head] >= 0 && paths.dist[ally_head] <= p_.feed_distance)
    return Because("feed champion", bot::Action::Resign());

  // 2. kamikaze: an enemy head next to ours, and the trade pays off
  for (int d = 0; d < 4; ++d) {
    const int32_t to = board_.Step(head_, d);
    if (Classify(to) == Cell::kEnemyHead && WorthTrading(board_.at(to).occupant))
      return Because("kamikaze", bot::Action::Move(d));
  }

  // 2b. lunge: an enemy head worth a trade within a multi-step MOVE (never the champion: it only grows)
  if (units_ > 1 && !(round_ >= p_.feed_from_round && champ.id == spawn_.id)) {
    bot::Action lunge;
    if (Lunge(&lunge)) return Because("lunge", lunge);
  }

  // 3. survival: our last dragons, outnumbered, keep away from enemy heads (losing them all loses the game)
  if (units_ <= p_.survivor_units && EnemiesSeen() > units_)
    if (const int flee = FleeDirection(); flee >= 0) {
      bot::Action a = SafeStep(flee, false);
      if (std::string(reason_) == "toward goal") reason_ = "survivor: flee";
      return a;
    }

  // 4. growth: split off a worker while there is time for it to gather
  if (length_ >= p_.split_min_length && round_ < (leader_ ? p_.leader_split_until : p_.split_until_round) &&
      FreeSlots() > std::max(p_.split_headroom, p_.reserve_slots) &&
      !(round_ >= p_.champ_freeze && champ.id == spawn_.id) &&
      GrowthSplit() >= 0)
    return Because("split", bot::Action::Split(GrowthSplit()));

  // 5. a goal, and the first safe step towards it
  Goal target = ally_head >= 0 && paths.dist[ally_head] > 0 ? Goal{ally_head, false} : Goal{};
  // late walk (from late-surge-7): the champion is somewhere this long feeder has never
  // explored (no known path to it): walk to the known tile nearest it and explore on from there
  if (feeding && round_ >= p_.converge_from && length_ >= p_.converge_min_length && target.tile < 0 &&
      ally_head >= 0 && ally_head != head_ && paths.dist[ally_head] < 0) {
    auto torus = [&](int32_t a, int32_t b) {
      const int32_t dx = std::abs(board_.X(a) - board_.X(b)), dy = std::abs(board_.Y(a) - board_.Y(b));
      return std::min(dx, board_.width() - dx) + std::min(dy, board_.height() - dy);
    };
    int32_t best_key = torus(head_, ally_head) * 2;
    for (int32_t t = 0; t < board_.size(); ++t) {
      const int32_t d = paths.dist[t];
      if (d <= 0 || Occupied(t)) continue;
      const int32_t key = torus(t, ally_head) * 2 + d;
      if (key < best_key) best_key = key, target = Goal{t, false};
    }
  }
  if (target.tile < 0) target = ChooseGoal(paths);
  const int32_t goal = target.tile;
  // loop breaker: circling without eating: ban this goal for a while
  // (not while waiting beside a bed for its countdown: circling there is the plan)
  const bool waiting = goal >= 0 && paths.dist[goal] > 0 && [&] {
    int32_t ready;
    return TileValue(goal, paths.dist[goal], &ready) > 0.0 && ready > paths.dist[goal];
  }();
  if (p_.loop_break && goal >= 0 && !waiting && Looping() && !(ally_head >= 0 && goal == ally_head)) {
    banned_[goal] = round_ + p_.loop_ban;
    if (last_goal_ >= 0) banned_[last_goal_] = round_ + p_.loop_ban;
#ifdef LS17_DEBUG
    ++g_bans;
#endif
    recent_.clear();
  }
  last_goal_ = goal;
  // exploring beside an unknown portal whose exit our ping found clear: step through it
  if (WillCross() && exit_clear_ && ping_tile_ == head_ && !target.trap && length_ >= 2 && UnknownPortal(head_, ping_dir_) &&
      BodyInView() && (goal < 0 || [&] {
        int32_t ready;
        return TileValue(goal, paths.dist[goal], &ready) == 0.0;
      }())) {
    bool enemy = false;
    for (const auto& [id, w] : seen_) enemy |= w.team != spawn_.team && w.round == round_ && w.head >= 0;
    if (!enemy) return Because("portal: explore", bot::Action::Move(ping_dir_));
  }
  const int preferred = goal >= 0 ? paths.first[goal] : -1;
  bot::Action a = SafeStep(preferred, target.trap);
  // the last step onto an enemy head we chose to hunt is a trade, not a mistake
  if (goal >= 0 && preferred >= 0 && board_.Step(head_, preferred) == goal && Classify(goal) == Cell::kEnemyHead &&
      WorthTrading(board_.at(goal).occupant))
    return Because("hunt: trade", bot::Action::Move(preferred));
  if (held && std::string(reason_) == "toward goal") reason_ = "held: peer not clearly longer";
  return a;
}

}  // namespace ls17
