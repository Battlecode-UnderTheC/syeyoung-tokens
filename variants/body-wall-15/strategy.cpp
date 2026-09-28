#include "strategy.hpp"
#include <string>

#include <algorithm>
#include <cmath>
#include <tuple>
#include <functional>
#ifdef BW15_TUNE
#include <cstdlib>
#include <string>
#endif
#include <unordered_map>
#include <unordered_set>
#ifdef BW15_DEBUG
#include <atomic>
#include <cstdio>
#include <cstdlib>
#endif

namespace bw15 {
#ifdef BW15_DEBUG
static long g_escape = 0, g_turnout = 0, g_calls = 0;
#define BW15_COUNT(x) ++(x)
// idle-loop measurement: turns where the last 12 heads cover <= 6 tiles with nothing eaten
static std::atomic<long> g_turns{0}, g_idle{0}, g_eaten{0}, g_cross{0}, g_bans{0};
static std::atomic<long> g_cross_dead{0}, g_champ_trap{0}, g_champ_dead_end{0};
static struct PortalReport {
  ~PortalReport() {
    std::fprintf(stderr, "BW15 crossings %ld, crossers dead within 2 rounds %ld; frozen champion trapped splits %ld, dead-end turns %ld\n",
                 g_cross.load(), g_cross_dead.load(), g_champ_trap.load(), g_champ_dead_end.load());
  }
} g_portal_report;
static std::atomic<long> g_seal_made{0}, g_seal_dead{0}, g_seal_alive{0}, g_seal_lost{0};
static struct SealReport {
  ~SealReport() {
    if (g_seal_made)
      std::fprintf(stderr, "BW15 multi-seal enemies %ld: gone with head tile in view %ld, alive after 4 %ld, lost sight %ld\n",
                   g_seal_made.load(), g_seal_dead.load(), g_seal_alive.load(), g_seal_lost.load());
  }
} g_seal_report;
static struct IdleReport {
  ~IdleReport() {
    if (g_turns) std::fprintf(stderr, "BW15 turns %ld idle %ld (%.2f%%) eaten %ld crossings %ld bans %ld\n", g_turns.load(),
                              g_idle.load(), 100.0 * g_idle / g_turns, g_eaten.load(), g_cross.load(), g_bans.load());
  }
} g_idle_report;
#else
#define BW15_COUNT(x)
#endif

StrategyAgent::StrategyAgent(const rl::Spawn& spawn, Params params)
    : p_(params), spawn_(spawn), board_(spawn.width, spawn.height), portal_(spawn.width * spawn.height, 0) {
#ifdef BW15_TUNE
  // BW15_P="name=value,..." overrides parameters (tuning builds only)
  if (const char* env = std::getenv("BW15_P")) {
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
#define BW15_SET(name) if (k == #name) p_.name = static_cast<decltype(p_.name)>(v);
      BW15_SET(split_min_length) BW15_SET(split_until_round) BW15_SET(feed_from_round) BW15_SET(wall_until) BW15_SET(trap_cost)
      BW15_SET(room_margin) BW15_SET(room_min) BW15_SET(escape_slack) BW15_SET(escape_min) BW15_SET(check_split)
      BW15_SET(liberties) BW15_SET(share_factor) BW15_SET(patrol) BW15_SET(hunt_radius) BW15_SET(explore_value)
      BW15_SET(escape_steps) BW15_SET(trap_small)
      BW15_SET(split_headroom) BW15_SET(turnout_budget) BW15_SET(doom_first) BW15_SET(ally_memory) BW15_SET(avoid_blind) BW15_SET(doomed_split_room) BW15_SET(leader_split_until) BW15_SET(share_leader) BW15_SET(feed_distance) BW15_SET(big_split_length)
      BW15_SET(portal_cross) BW15_SET(portal_explore) BW15_SET(loop_break) BW15_SET(loop_ban) BW15_SET(known_portal) BW15_SET(blind_fallback) BW15_SET(exit_risk_max) BW15_SET(exit_risk_ping)
      BW15_SET(wait_factor) BW15_SET(cluster_bonus) BW15_SET(cluster_radius) BW15_SET(champ_no_trap) BW15_SET(cross_min_length) BW15_SET(never_cross)
#undef BW15_SET
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
    waller_ = !leader_ && p_.waller_mod > 0 && spawn_.id % p_.waller_mod == 0;
    born_ = o.round;                         // alive at the start: not a split child
    first_turn_ = false;
    for (const rl::Segment& s : o.segments)         // the part of the body in view, head to tail
      if (s.id == spawn_.id) body_.push_back(board_.Tile(s.x, s.y));
  }
  if (body_.empty() || body_.front() != head_) body_.push_front(head_);
  while (static_cast<int32_t>(body_.size()) > length_) body_.pop_back();
  std::unordered_map<int32_t, int32_t> segments;
  bodies_.clear();
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
    bodies_[s.id].push_back(board_.Tile(s.x, s.y));
    Sighting& w = seen_[s.id];
    w.team = s.team;
    w.round = round_;
    if (s.head) w.head = board_.Tile(s.x, s.y);
  }
  for (const auto& [id, n] : segments) seen_[id].length = n;
  // obituaries: an ally >= obit_min_length in view, its head beside ours last turn, is gone
  // now while we moved one plain step (it died): note it dead (length 0)
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
  for (const uint64_t w : o.inbox) Decode(w);
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
    const Report& was = reports_[spawn_.id];
    if (was.id >= 0 && was.length >= p_.obit_champion && length_ * 2 < was.length) {
      obit_ = Report{spawn_.id, head_, length_, round_, false};
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
  exit_dirty_ = false;
  if (ping_round_ == round_ - 1 && ping_tile_ == head_ && o.echo_present) {
    bool any = false;
    for (int k = 1; k < 5; ++k) any |= o.echo[k] != "0";   // ally, ally head, enemy, enemy head
    exit_clear_ = !any;
    exit_dirty_ = any;
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
  const auto it = seen_.find(enemy_id);
  return WorthTradingLength(it == seen_.end() ? 0 : it->second.length);
}

bool StrategyAgent::WorthTradingLength(int32_t theirs) const {
  if (units_ <= 1) return false;
  if (leader_) return theirs > length_;
  // trades are one for one: when they have more dragons, even trades walk us to elimination
  if (p_.outnumbered_factor > 0 && units_ < EnemiesSeen()) return theirs >= p_.outnumbered_factor * length_;
  return theirs >= length_ + p_.trade_margin ||
         (p_.outnumber_factor > 0 && units_ >= p_.hunt_min_units &&
          units_ >= p_.outnumber_factor * std::max(1, EnemiesSeen()));
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

static constexpr uint64_t kThreat = 0x5C;   // body-wall-11: an enemy head (id, tile, length, round)

uint64_t StrategyAgent::Encode(const Report& r, uint64_t magic) const {
  const uint64_t x = board_.X(r.tile) & 63, y = board_.Y(r.tile) & 63;
  return (magic << 56) | (uint64_t(spawn_.team & 1) << 55) | (uint64_t(r.id & 4095) << 43) | (x << 37) | (y << 31) |
         (uint64_t(std::min(r.length, 4095)) << 19) | (uint64_t(r.round & 511) << 10) | uint64_t(r.lower ? 1 : 0);
}

static constexpr uint64_t kNever = 0x3D;   // body-wall-15: a never-cross portal (entry x, y, direction)

void StrategyAgent::MarkNever() {
  if (!p_.never_cross || cross_tile_ < 0 || round_ - cross_round_ > 2) return;
  never_key_ = cross_tile_ * 4 + cross_dir_;
  never_.insert(never_key_);
  never_emit_until_ = round_ + 2;
}

void StrategyAgent::Decode(uint64_t w) {
  if ((w >> 56) == kNever) {
    if (int32_t((w >> 55) & 1) != (spawn_.team & 1)) return;
    const int32_t x = int32_t((w >> 37) & 63), y = int32_t((w >> 31) & 63);
    if (x >= board_.width() || y >= board_.height()) return;
    never_.insert(board_.Tile(x, y) * 4 + int32_t((w >> 29) & 3));
    return;
  }
  const bool threat = (w >> 56) == kThreat;
  if (((w >> 56) != kMagic && !threat) || int32_t((w >> 55) & 1) != (spawn_.team & 1) || (w & 1022) != 0) return;
  Report r;
  r.id = int32_t((w >> 43) & 4095);
  const int32_t x = int32_t((w >> 37) & 63), y = int32_t((w >> 31) & 63);
  if (x >= board_.width() || y >= board_.height()) return;
  r.tile = board_.Tile(x, y);
  r.length = int32_t((w >> 19) & 4095);
  r.round = int32_t((w >> 10) & 511);
  r.lower = (w & 1) != 0;
  if (threat) {
    if (r.round > round_) return;
    Report& have = threats_[r.id];
    if (r.round > have.round || (r.round == have.round && r.length > have.length)) have = r;
    return;
  }
  if (r.round > round_ || r.id == spawn_.id) return;
  Note(r);
}

void StrategyAgent::Note(const Report& r) {
  Report& have = reports_[r.id];
  if (r.round > have.round || (r.round == have.round && r.length > have.length)) {
    if (p_.lower_bound_reports && r.lower && have.id >= 0 && have.length > r.length) {
      have.tile = r.tile;   // it is still at least as long as we knew
      have.round = r.round;
      have.lower = true;
    } else {
      // an exact, newer report cutting a known long ally to less than half its length: a
      // trap-split stub (or a death, length 0) rather than the champion we thought it was;
      // relay the correction so the team stops naming a ghost
      if (r.length * 2 < have.length && !r.lower && r.round > have.round && have.length >= p_.obit_champion) {
        obit_ = r;
        obit_until_ = round_ + p_.obit_rounds;
      }
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
    // late-surge-17: waiting on a bed is charged more than walking
    *ready = p_.wait_factor > 0 ? dist + static_cast<int32_t>(p_.wait_factor * std::max(0, due - dist))
                                 : std::max(dist, due);
    return 1.0;
  }
  return age == 0 ? 1.0 : p_.ripe_bed_value;               // due now (blocked while occupied)
}

int32_t StrategyAgent::RoomNeeded() const { return std::max(length_ + p_.room_margin, p_.room_min); }

StrategyAgent::Goal StrategyAgent::ChooseGoal(const Paths& paths) const {
  // hunting: a worker with dragons to spare goes for enemy heads it just saw
  if (!leader_ && units_ >= p_.hunt_min_units && round_ < p_.feed_from_round) {
    int32_t best = -1;
    for (const auto& [id, w] : seen_)
      if (w.team != spawn_.team && w.round == round_ && w.head >= 0 && paths.dist[w.head] > 0 &&
          paths.dist[w.head] <= p_.hunt_radius && WorthTrading(id) &&
          (best < 0 || paths.dist[w.head] < paths.dist[best]))
        best = w.head;
    if (best >= 0) return {best, false};
  }
  // a waller long enough to wall goes where enemy heads are (seen in the last few rounds)
  const bool at_cap = units_ >= spawn_.unit_limit - p_.split_headroom;
  const bool cap_waller = !leader_ && at_cap && p_.cap_waller_mod > 0 && spawn_.id % p_.cap_waller_mod == 0;
  if ((waller_ || cap_waller) && length_ >= p_.squeeze_length && round_ < p_.wall_until) {
    int32_t best = -1;
    for (const auto& [id, w] : seen_)
      if (w.team != spawn_.team && round_ - w.round <= p_.waller_chase_memory && w.head >= 0 && paths.dist[w.head] > 0 &&
          paths.dist[w.head] <= p_.waller_chase_radius && (best < 0 || paths.dist[w.head] < paths.dist[best]))
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
  // gst49/swh17: many allies in view means beds nearby are already being worked, so stale
  // (long-unseen) ground is worth chasing even if it's far, and patrol (towards what hasn't
  // been seen longest) should compete directly instead of only running as a fallback once
  // nothing else scores.
  int32_t crowd = 0;
  for (const auto& [id, w] : seen_)
    if (w.team == spawn_.team && w.round == round_ && w.head >= 0 && id != spawn_.id) ++crowd;
  const bool crowded = crowd > 3;
  // gst49: which tiles are ours right now, once (Mine() walks the whole body otherwise: an
  // O(length) check per tile adds up fast over a full-board pass)
  std::vector<uint8_t> mine_mask(board_.size(), 0);
  for (const int32_t t : body_)
    if (t >= 0) mine_mask[t] = 1;
  auto occ = [&](int32_t t) { return mine_mask[t] != 0 || (board_.Current(t) && board_.at(t).occupant >= 0); };
  // late-surge-17: each tile's value and rounds until ripe, once (for the cluster bonus below;
  // calling TileValue per neighbour of every tile is slow, so it's precomputed here)
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
      // late-surge-17: a pearl or bed near other pearls/beds is worth more (a bed far from ripe
      // counts little)
      double near = 0;
      const int32_t R = p_.cluster_radius, x = board_.X(t), y = board_.Y(t);
      for (int32_t dy = -R; dy <= R; ++dy)
        for (int32_t dx = -R + std::abs(dy); dx <= R - std::abs(dy); ++dx) {
          if (dx == 0 && dy == 0) continue;
          const int32_t n = board_.Tile(x + dx, y + dy);
          const double v = base[n];
          if (v <= 0) continue;
          const int32_t dn = d + std::abs(dx) + std::abs(dy);
          const int32_t r2 = p_.wait_factor > 0 ? dn + static_cast<int32_t>(p_.wait_factor * std::max(0, due0[n] - dn))
                                                 : std::max(dn, due0[n]);
          near += v * (d + 1.0) / (r2 + 1.0);
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
      if (board_.Step(t, dir) == kUnknown &&
          !(p_.portal_explore && (portal_[t] >> dir & 1) && (!WillCross() || length_ < p_.cross_min_length || Never(t, dir)))) {
        // swh17: crowded means beds are already being fought over; ground unseen for a long
        // time earns a multiplier so it can compete with a bed several dragons pile onto,
        // instead of always losing for being merely far.
        double mult = 1.0;
        if (crowded) {
          const int32_t age = round_ - board_.at(t).seen_round;
          mult = std::min(4.0, std::max(1.0, age / 50.0));
        }
        score = std::max(score, mult * p_.explore_value / (d + 1));
        break;
      }
    // swh17: fold patrol (towards what hasn't been seen for longest) into this same scan
    // when crowded, so it competes with weak bed/pearl goals directly instead of only
    // running as the fallback below once nothing else scores at all.
    if (crowded && p_.patrol && !occ(t) && !Banned(t)) {
      const double patrol_score = (round_ - board_.at(t).seen_round) / (d + 4.0);
      score = std::max(score, patrol_score);
    }
    if (score > 0 && !Banned(t)) candidates.push_back({t, score});
  }
  const size_t k = std::min<size_t>(candidates.size(), p_.trap_candidates);
  std::partial_sort(candidates.begin(), candidates.begin() + k, candidates.end(),
                    [](const Candidate& a, const Candidate& b) { return a.score > b.score; });
  // the best few, checked for dead ends: a dead end is worth its pocket's pearls less the
  // cost of splitting out of it
  const int32_t need = RoomNeeded();
  const bool no_trap = (p_.cap_safety && length_ >= 4 && !CanCountOnSplit()) ||
                       (p_.champ_no_trap && round_ >= p_.champ_freeze && Champion().id == spawn_.id);
  auto open = [&](int32_t t) { return !Occupied(t); };
  Goal best;
  double best_score = 0;
  std::vector<int32_t> pocket;
  for (size_t i = 0; i < k; ++i) {
    const Candidate& c = candidates[i];
    double score = c.score;
    bool trap = false;
    if (RoomAround(board_, c.tile, open, need, &pocket) < need) {
      if (no_trap) continue;                          // near the cap: a dead end is a grave (gst27); a frozen champion coils (bw15)
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

#ifdef BW15_DEBUG
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
      BW15_COUNT(g_turnout);
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
  BW15_COUNT(g_calls);
  for (size_t qi = 0; qi < queue.size(); ++qi) {
    BW15_COUNT(g_escape);
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

bot::Action StrategyAgent::SafeStep(int preferred, bool trap_ok) {
  // near the cap a long dragon can't split out of a dead end: plan like a short one (gst27)
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
    const bool exposed = Exposed(to);
    body[0] = to;
    int32_t count = 0;
    int level = p_.escape_steps ? EscapeLevel(body, cap, &count) : 1;
    if (level == 0 && p_.turnout_budget > 0) level = TurnOut(body, count + length_ + 2) == 0 ? 0 : 2;
    int libs = p_.liberties ? Liberties(to, body) : 2;
    // through a portal to a tile out of view: someone may be lying there
    bool blind = p_.avoid_blind && !board_.Current(to);
    const char* lifted = nullptr;
    // (gst42/gst46) a known portal is not a one-way door: last turn's ping through it came back
    // clean, or its exit was seen empty a round or two ago, or we already refused this goal's
    // blind step twice (unless our ping found a dragon on the far side)
    if (blind && p_.known_portal) {
      const bool here = ping_tile_ == head_ && ping_dir_ == d;
      const TileMemory& m = board_.at(to);
      if (exit_clear_ && here) lifted = "portal: pinged clear";
      else if (m.seen_round >= round_ - 2 && m.seen_round >= 0 && m.occupant < 0) lifted = "portal: exit seen empty";
      else if (p_.blind_fallback > 0 && k == 0 && preferred >= 0 && cur_goal_ >= 0 && blind_goal_ == cur_goal_ &&
               blind_streak_ >= p_.blind_fallback && round_ - blind_round_ <= 30 && !(exit_dirty_ && here))
        lifted = "portal: fallback";
      if (lifted) {
        // (gst46) the exit's neighbours as we know them (not the tile we come from): a clean
        // ping only clears the straight line, so an exit with 3 unknown / occupied sides is
        // where a head beside it steps in as we cross
        int heads = 0, bodies = 0, stale = 0;
        for (int e = 0; e < 4; ++e) {
          const int32_t n = board_.Step(to, e);
          if (n == head_ || n == kBlocked) continue;
          if (n < 0) { ++stale; continue; }
          const TileMemory& nm = board_.at(n);
          if (nm.occupant >= 0 && nm.occupant_head && nm.occupant_team != spawn_.team && nm.seen_round >= round_ - 4) ++heads;
          else if (nm.occupant >= 0 && nm.seen_round >= round_ - 2) ++bodies;
          else if (nm.seen_round < round_ - 2) ++stale;
        }
        const bool risky = p_.exit_risk_max < 4 && (p_.exit_risk_ping || !(exit_clear_ && here)) && (heads > 0 || heads + bodies + stale > p_.exit_risk_max);
        if (risky) lifted = nullptr;
      }
      if (lifted) blind = false;
    }
    if (blind) {
      // the goal step refused only for being blind: remember it (ping it next time we stand here)
      if (p_.known_portal && k == 0 && preferred >= 0 && cur_goal_ >= 0 && !exposed && libs >= 2 && level >= 1) {
        if (blind_goal_ != cur_goal_ || round_ - blind_round_ > 30) blind_streak_ = 0;
        blind_goal_ = cur_goal_;
        ++blind_streak_;
        blind_round_ = round_;
        blind_tile_ = head_;
        blind_dir_ = d;
      }
      libs = std::min(libs, 1);
    }
    if (k == 0 && preferred >= 0 && !exposed && libs >= 2 && ((level >= 1 && room >= need) || (trap_ok && level >= 1))) {
      if (lifted && room >= need) return Because(lifted, bot::Action::Move(d));
      return Because(room >= need ? "toward goal" : "into a dead end worth it", bot::Action::Move(d));
    }
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
  {
    MarkNever();
    return Because("doomed: split", bot::Action::Split(length_ - 2));
  }
  if (best >= 0) return Because(preferred >= 0 ? "detour (goal step unsafe)" : "no goal: roomiest", bot::Action::Move(best));
  // no free step. Split: the child's head is our tail, which points back the way we came
  // (usually open), so the child walks out with most of the length; the stub stays behind
  if (CanSplitNow()) {
    // a long stub may keep one more segment so that the child hears the champion from it
    MarkNever();
#ifdef BW15_DEBUG
    if (round_ >= p_.champ_freeze && Champion().id == spawn_.id) ++g_champ_trap;
#endif
    const int32_t n = p_.heard_split && length_ >= 8 && !HeardSplit(length_ - 2) && HeardSplit(length_ - 3) ? length_ - 3 : length_ - 2;
    return Because("trapped: split", bot::Action::Split(n));
  }
  // else trade with an adjacent enemy head, else step through an unexplored portal
  for (int d = 0; d < 4; ++d)
    if (Classify(board_.Step(head_, d)) == Cell::kEnemyHead) return Because("trapped: trade", bot::Action::Move(d));
  for (int d = 0; d < 4; ++d)
    if (Classify(board_.Step(head_, d)) == Cell::kUnknown) return Because("trapped: unknown portal", bot::Action::Move(d));
#ifdef BW15_DEBUG
  Dump("trapped: no move");
#endif
  return Because("trapped: no move", bot::Action::Resign());
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

// ---- the body wall ---------------------------------------------------------------------------

// The tiles enemy `id` (body head first, as seen) can reach, up to `cap`, with our body
// `ours` (head first, after our move). Timed: its own tail and our tail free tiles as they
// move on, and reaching its freed tail is a way out (returns cap). Static: every body stays.
// Timed: leaving the window is a way out (we can't tell); static: only the room in view counts.
int32_t StrategyAgent::EnemyReach(int32_t id, const std::vector<int32_t>& enemy, const std::vector<int32_t>& ours,
                                  bool timed, int32_t cap) const {
  const int32_t n = board_.size(), elen = static_cast<int32_t>(enemy.size()), olen = static_cast<int32_t>(ours.size());
  std::vector<int32_t> ei(n, -1), oi(n, -1), when(n, -1);
  for (int32_t i = elen - 1; i >= 0; --i) ei[enemy[i]] = i;
  for (int32_t i = olen - 1; i >= 0; --i)
    if (ours[i] >= 0) oi[ours[i]] = i;
  std::vector<int32_t> queue{enemy[0]};
  when[enemy[0]] = 0;
  int32_t reached = 1;
  for (size_t q = 0; q < queue.size(); ++q) {
    const int32_t t = queue[q], m = when[t] + 1;
    for (int d = 0; d < 4; ++d) {
      const int32_t to = board_.Step(t, d);
      if (to == kUnknown) return cap;
      if (to < 0 || when[to] >= 0) continue;
      if (!board_.Current(to)) {                         // out of view: can't tell
        if (timed) return cap;
        continue;                                        // static: count the room in view only
      }
      if (const int32_t i = ei[to]; i >= 0) {
        if (timed && i > 0 && i >= elen - m + 1) return cap;   // follows its own tail out
        continue;
      }
      if (const int32_t j = oi[to]; j >= 0) {
        if (!timed || olen - j > m) continue;            // our segment still there then
      } else if (board_.at(to).occupant >= 0 && board_.at(to).occupant != spawn_.id && board_.at(to).occupant != id) {
        continue;                                        // another dragon (assumed to stay)
      }
      when[to] = m;
      if (++reached >= cap) return cap;
      queue.push_back(to);
    }
  }
  return reached;
}

// A step that seals enemy heads in view into pockets they die in (more enemy length sealed
// than by our other safe steps); failing that, for a long dragon, the step that shrinks the
// room around enemy heads most (closing a pocket behind them).
bool StrategyAgent::Wall(bot::Action* out, bool* squeezed) {
  if (!p_.wall || length_ < p_.wall_min_length) return false;
  if (round_ >= p_.feed_from_round && Champion().id == spawn_.id) return false;
  if (leader_ && round_ >= p_.wall_leader_round) return false;
  if (static_cast<int32_t>(body_.size()) < length_) return false;   // own body not fully known
  std::vector<std::pair<int32_t, const std::vector<int32_t>*>> enemies, allies;
  for (const auto& [id, b] : bodies_) {
    const auto it = seen_.find(id);
    if (it == seen_.end() || it->second.round != round_ || b.empty() || it->second.head != b[0]) continue;
    if (it->second.team == spawn_.team) {
      if (p_.ally_room_cost > 0) allies.push_back({id, &b});
    } else {
      enemies.push_back({id, &b});
    }
  }
  if (enemies.empty()) return false;
  const bool squeeze = p_.squeeze_room > 0 && length_ >= p_.squeeze_length && round_ < p_.wall_until;
  int best = -1;
  int32_t best_seal = -1, best_sq = -1, worst_seal = 1 << 30, worst_sq = 1 << 30;
  std::vector<int32_t> ours(body_.begin(), body_.end());
  // the room each allied head in view has now (our body as it is), to charge the squeeze what it takes
  std::vector<int32_t> ally_room;
  if (squeeze)
    for (const auto& [id, b] : allies) ally_room.push_back(EnemyReach(id, *b, ours, false, p_.squeeze_room));
  ours.insert(ours.begin(), -1);
  ours.pop_back();
  int32_t safe_steps = 0;
  std::vector<int32_t> sealed_by(enemies.size(), 0);   // safe single steps that seal enemy e
  for (int d = 0; d < 4; ++d) {
    const int32_t to = board_.Step(head_, d);
    if (Classify(to) != Cell::kFree || !board_.Current(to) || Exposed(to)) continue;
    ours[0] = to;
    if (EscapeLevel(ours, EscapeCap(length_)) < 1 || Liberties(to, ours) < 1) continue;
    int32_t seal = 0, sq = 0;
    ++safe_steps;
    for (size_t e = 0; e < enemies.size(); ++e) {
      const auto& [id, b] = enemies[e];
      if (EnemyReach(id, *b, ours, true, p_.wall_cap) < p_.wall_cap) {
        ++sealed_by[e];
        seal += static_cast<int32_t>(b->size());
      } else if (squeeze) {
        sq += p_.squeeze_room - EnemyReach(id, *b, ours, false, p_.squeeze_room);
      }
    }
    if (squeeze && seal == 0)
      for (size_t k = 0; k < allies.size(); ++k)
        sq -= static_cast<int32_t>(p_.ally_room_cost *
                                   std::max(0, ally_room[k] - EnemyReach(allies[k].first, *allies[k].second, ours, false, p_.squeeze_room)));
    worst_seal = std::min(worst_seal, seal);
    worst_sq = std::min(worst_sq, sq);
    if (std::make_pair(seal, sq) > std::make_pair(best_seal, best_sq)) {
      best_seal = seal;
      best_sq = sq;
      best = d;
    }
  }
  // bw15 multi-step seal: no single step seals anyone, so try a MOVE of 2..seal_steps steps
  // (each after the first costs a tail segment) that lays our head across a pocket's exit
  // before the enemy moves again
  if (p_.seal_steps >= 2 && length_ >= 3 && best_seal <= std::max(worst_seal, 0) && round_ < p_.wall_until) {
    bot::Action multi;
    // an enemy every safe step seals is doomed anyway: it earns the multi-step nothing
    std::vector<std::pair<int32_t, const std::vector<int32_t>*>> open_enemies;
    for (size_t e = 0; e < enemies.size(); ++e)
      if (safe_steps == 0 || sealed_by[e] < safe_steps) open_enemies.push_back(enemies[e]);
    if (!open_enemies.empty() && MultiSeal(open_enemies, &multi)) {
      *squeezed = false;
      *out = multi;
      return true;
    }
  }
  if (best < 0) return false;
  if (best_seal > worst_seal) {
    *squeezed = false;
  } else if (squeeze && best_sq > worst_sq && best_sq >= p_.squeeze_gain) {
    *squeezed = true;
  } else {
    return false;
  }
  *out = bot::Action::Move(best);
  return true;
}

// A multi-step MOVE after which some enemy head in view is sealed in (timed flood finds no
// way out), worth more enemy length than the segments it costs us.
bool StrategyAgent::MultiSeal(const std::vector<std::pair<int32_t, const std::vector<int32_t>*>>& enemies,
                              bot::Action* out) const {
  const int32_t max_steps = std::min<int32_t>(p_.seal_steps, length_ - 1);
  if (max_steps < 2) return false;
  // tiles next to the room enemies have now (static, in view): a seal must touch one
  std::unordered_set<int32_t> near;
  std::vector<std::unordered_set<int32_t>> near_of;   // per enemy (only these are flooded for a path touching them)
  {
    for (const auto& [id, b] : enemies) {
      near_of.emplace_back();
      std::unordered_set<int32_t> seen{(*b)[0]};
      std::vector<int32_t> q{(*b)[0]};
      for (size_t i = 0; i < q.size() && q.size() < static_cast<size_t>(p_.wall_cap); ++i)
        for (int d = 0; d < 4; ++d) {
          const int32_t to = board_.Step(q[i], d);
          if (to < 0 || !board_.Current(to) || seen.count(to)) continue;
          seen.insert(to);
          near.insert(to);
          near_of.back().insert(to);
          if (Mine(to) || std::find(b->begin(), b->end(), to) != b->end()) continue;   // other bodies move on
          q.push_back(to);
        }
    }
  }
  if (near.empty()) return false;
  int32_t best_gain = 0, evals = 0;
  std::vector<int> best_steps;
  std::vector<int> steps;
  std::vector<int32_t> path;
  // depth-first over paths of free tiles in view
  std::function<void(int32_t)> dfs = [&](int32_t at) {
    if (evals >= p_.seal_evals) return;
    const int32_t k = static_cast<int32_t>(steps.size());
    if (k >= 2) {
      bool touches = false;
      for (const int32_t t : path) touches |= near.count(t) > 0;
      if (touches && !Exposed(at)) {
        // our body after the move: the path's tiles in front, the tail cut by the steps taken
        // (a pearl keeps the tail for that step), one extra segment per step after the first
        std::vector<int32_t> ours(body_.begin(), body_.end());
        int32_t eaten = 0;
        for (int32_t i = 0; i < k; ++i) {
          ours.insert(ours.begin(), path[i]);
          if (board_.at(path[i]).pearl) ++eaten;
          else ours.pop_back();
          if (i > 0) ours.pop_back();
        }
        const int32_t spent = std::max(0, (k - 1) - eaten);
        if (static_cast<int32_t>(ours.size()) >= 2) {
          // enemies first (only those whose room the path touches), our own safety only for a winner
          int32_t sealed = 0;
          for (size_t e = 0; e < enemies.size(); ++e) {
            bool touched = false;
            for (const int32_t t : path) touched |= near_of[e].count(t) > 0;
            if (!touched) continue;
            ++evals;
            if (EnemyReach(enemies[e].first, *enemies[e].second, ours, true, p_.wall_cap) < p_.wall_cap)
              sealed += static_cast<int32_t>(enemies[e].second->size());
          }
          const int32_t gain = sealed - static_cast<int32_t>(p_.seal_cost * spent);
          if (sealed > 0 && gain > best_gain && Liberties(at, ours) >= 1 &&
              EscapeLevel(ours, EscapeCap(static_cast<int32_t>(ours.size()))) >= 1) {
            best_gain = gain;
            best_steps = steps;
#ifdef BW15_DEBUG
            dbg_sealed_.clear();
            for (const auto& [id, b] : enemies)
              if (EnemyReach(id, *b, ours, true, p_.wall_cap) < p_.wall_cap) dbg_sealed_.push_back({id, (*b)[0], round_});
#endif
          }
        }
      }
    }
    if (k >= max_steps) return;
    for (int d = 0; d < 4; ++d) {
      const int32_t to = board_.Step(at, d);
      if (Classify(to) != Cell::kFree || !board_.Current(to)) continue;
      if (std::find(path.begin(), path.end(), to) != path.end()) continue;
      steps.push_back(d);
      path.push_back(to);
      dfs(to);
      steps.pop_back();
      path.pop_back();
    }
  };
  dfs(head_);
  if (best_steps.empty()) return false;
#ifdef BW15_DEBUG
  for (const auto& x : dbg_sealed_) {
    dbg_pending_.push_back(x);
    ++g_seal_made;
  }
#endif
  *out = bot::Action{bot::Action::kMove, best_steps, 0, false, {}};
  return true;
}

// ---- threats heard by sonar (body-wall-11) ------------------------------------------------------

bool StrategyAgent::Threat(bot::Action* out, const Paths& paths) {
  int32_t best = -1, best_dist = 1 << 30;
  for (auto it = threats_.begin(); it != threats_.end();) {
    const Report& r = it->second;
    if (round_ - r.round > p_.threat_age) {
      it = threats_.erase(it);
      continue;
    }
    ++it;
    if (const auto s = seen_.find(r.id); s != seen_.end() && s->second.round == round_) continue;   // in view: the usual logic
    if (board_.Current(r.tile)) continue;                        // it was there, we see the tile now: it moved away
    const bool trade = WorthTradingLength(r.length);
    const bool shorter = p_.threat_hunt_shorter && length_ >= r.length + 2 && length_ >= p_.wall_min_length;
    if (!trade && !shorter) continue;
    const int32_t d = paths.dist[r.tile];
    if (d <= 0 || d > (length_ - 1) + p_.threat_slack) continue;
    if (d < best_dist) {
      best_dist = d;
      best = r.tile;
    }
  }
  if (best < 0) return false;
  bot::Action a = SafeStep(paths.first[best], false);
  if (std::string(reason_) != "toward goal") return false;     // not safe to go: carry on as usual
  reason_ = "threat: approach";
  *out = a;
  return true;
}

#ifdef BW15_DEBUG
static std::atomic<long> g_dead{0}, g_dead_view{0}, g_dead_blind{0}, g_dead_heard{0};

static struct DeathReport {
  ~DeathReport() {
    if (g_dead)
      std::fprintf(stderr, "BW15 deaths not by own act %ld: enemy head in view %ld, none in view %ld (heard a threat %ld)\n",
                   g_dead.load(), g_dead_view.load(), g_dead_blind.load(), g_dead_heard.load());
  }
} g_death_report;

StrategyAgent::~StrategyAgent() {
  if (round_ < 497 && length_ > 0 && round_ - cross_round_ <= 2) ++g_cross_dead;
  if (round_ >= 497 || length_ <= 0) return;
  const std::string r = reason_;
  for (const char* own : {"feed", "trapped", "kamikaze", "lunge", "trade", "split", "wall"})
    if (r.find(own) != std::string::npos) return;
  ++g_dead;
  if (dbg_enemy_view_) ++g_dead_view;
  else {
    ++g_dead_blind;
    if (dbg_heard_) ++g_dead_heard;
  }
}
#endif

// ---- the turn --------------------------------------------------------------------------------------

bot::Action StrategyAgent::Turn(const rl::Observation& o) {
  bot::Action a = Decide(o);
  // tell the dragons around us who the champion is (a dead dragon's pings are not cast)
  const Report champ = Champion();
  if (champ.id >= 0)
    for (int d = 0; d < 4; ++d) a.sonar[d] = Encode(champ);
  // body-wall-11: announce the longest enemy head in view on the two directions (alternating
  // N/S and E/W by round) that don't carry the champion word
  if (p_.threat_ping && round_ < p_.threat_until) {
    Report best;
    for (const auto& [id, w] : seen_)
      if (w.team != spawn_.team && w.round == round_ && w.head >= 0 && w.length > best.length)
        best = Report{id, w.head, w.length, round_, false};
    if (best.id >= 0) {
      const int c = round_ & 1;   // champion on c and c + 2
      a.sonar[c + 1] = Encode(best, kThreat);
      a.sonar[(c + 3) % 4] = Encode(best, kThreat);
      if (champ.id < 0) a.sonar[c] = a.sonar[c + 2] = Encode(best, kThreat);
    }
  }
  // stepping next to an unknown portal: ping only through it, so next turn's echoes say
  // whether a dragon lies past its exit
  bool pinging = false;
  // (gst42/gst46) stepping back onto the tile beside a known portal whose blind step we refused: ping through it
  if (p_.known_portal && a.kind == bot::Action::kMove && !a.resign && a.steps.size() == 1 && round_ - blind_round_ <= 30) {
    const int d = a.steps[0];
    const int32_t n = board_.Step(head_, d);
    if (n >= 0 && n == blind_tile_ && blind_dir_ != (d + 2) % 4 && (portal_[n] >> blind_dir_ & 1)) {
      a.sonar = {};
      a.sonar[blind_dir_] = champ.id >= 0 ? Encode(champ) : 1;
      ping_round_ = round_;
      ping_tile_ = n;
      ping_dir_ = blind_dir_;
      pinging = true;
    }
  }
  if (!pinging && WillCross() && a.kind == bot::Action::kMove && !a.resign && a.steps.size() == 1) {
    const int d = a.steps[0];
    const int32_t n = board_.Step(head_, d);
    if (n >= 0 && board_.Current(n))
      for (int e = 0; e < 4; ++e)
        if (e != (d + 2) % 4 && UnknownPortal(n, e) && !Never(n, e) && length_ >= p_.cross_min_length) {
          a.sonar = {};
          a.sonar[e] = champ.id >= 0 ? Encode(champ) : 1;
          ping_round_ = round_;
          ping_tile_ = n;
          ping_dir_ = e;
          break;
        }
  }
  // obituaries: relay the last correction on odd rounds for a while, overriding the champion
  // ping (an ally that just learned its champion died or trap-split wins the airwaves briefly)
  if (obit_until_ > round_ && (round_ & 1))
    for (int d = 0; d < 4; ++d) a.sonar[d] = Encode(obit_);
  // bw15: a portal that just trapped us: tell whoever the pings reach (the split child first)
  if (never_emit_until_ >= round_ && never_key_ >= 0) {
    const int32_t t = never_key_ / 4;
    const uint64_t w = (kNever << 56) | (uint64_t(spawn_.team & 1) << 55) | (uint64_t(board_.X(t) & 63) << 37) |
                       (uint64_t(board_.Y(t) & 63) << 31) | (uint64_t(never_key_ & 3) << 29);
    for (int d = 0; d < 4; ++d) a.sonar[d] = w;
  }
#ifdef BW15_DEBUG
  dbg_enemy_view_ = false;
  for (const auto& [id, w] : seen_) dbg_enemy_view_ |= w.team != spawn_.team && w.round == round_ && w.head >= 0;
  dbg_heard_ = false;
  for (const auto& [id, r] : threats_) dbg_heard_ |= round_ - r.round <= 1;
  ++g_turns;
  if (Looping()) ++g_idle;
  if (length_ > 0 && round_ == last_eat_ && round_ > born_) ++g_eaten;
  if (std::string(reason_) == "portal: explore") ++g_cross;
  if (std::string(reason_) == "into a dead end worth it" && round_ >= p_.champ_freeze && Champion().id == spawn_.id) ++g_champ_dead_end;
#endif
  return a;
}

bot::Action StrategyAgent::Decide(const rl::Observation& o) {
  Remember(o);
#ifdef BW15_DEBUG
  for (size_t i = 0; i < dbg_pending_.size();) {
    const auto [id, head, r] = dbg_pending_[i];
    const auto it = seen_.find(id);
    const bool visible = it != seen_.end() && it->second.round == round_;
    int verdict = 0;
    if (!visible && board_.Current(head)) verdict = 1;
    else if (visible && round_ - r >= 4) verdict = 2;
    else if (round_ - r >= 4) verdict = 3;
    if (verdict) {
      (verdict == 1 ? g_seal_dead : verdict == 2 ? g_seal_alive : g_seal_lost)++;
      dbg_pending_.erase(dbg_pending_.begin() + i);
    } else {
      ++i;
    }
  }
#endif

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

  // 2c. body wall: seal an enemy head into a pocket it dies in
  {
    bot::Action wall;
    bool squeezed = false;
    if (Wall(&wall, &squeezed))
      return Because(squeezed ? "wall: squeeze" : wall.steps.size() > 1 ? "wall: multi-seal" : "wall: seal", wall);
  }

  // 2d. body-wall-11: an enemy head heard of (not in view) within our lunge reach + slack: walk at it
  if (p_.threat_ping && units_ > 1 && ally_head < 0 && !(round_ >= p_.champ_freeze && champ.id == spawn_.id)) {
    bot::Action hunt;
    if (Threat(&hunt, paths)) return hunt;
  }

  // 3. survival: our last dragons, outnumbered, keep away from enemy heads (losing them all loses the game)
  if (units_ <= p_.survivor_units && EnemiesSeen() > units_)
    if (const int flee = FleeDirection(); flee >= 0) {
      bot::Action a = SafeStep(flee, false);
      if (std::string(reason_) == "toward goal") reason_ = "survivor: flee";
      return a;
    }

  // 4. growth: split off a worker while there is time for it to gather
  if (length_ >= (waller_ ? p_.waller_split : p_.split_min_length) && round_ < (leader_ ? p_.leader_split_until : p_.split_until_round) &&
      FreeSlots() > std::max(p_.split_headroom, p_.reserve_slots) &&
      !(round_ >= p_.champ_freeze && champ.id == spawn_.id) &&
      GrowthSplit() >= 0)
    return Because("split", bot::Action::Split(GrowthSplit()));

  // 5. a goal, and the first safe step towards it
  const Goal target = ally_head >= 0 && paths.dist[ally_head] > 0 ? Goal{ally_head, false} : ChooseGoal(paths);
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
#ifdef BW15_DEBUG
    ++g_bans;
#endif
    recent_.clear();
  }
  last_goal_ = goal;
  // exploring beside an unknown portal whose exit our ping found clear: step through it
  if (WillCross() && exit_clear_ && ping_tile_ == head_ && !target.trap && length_ >= std::max(2, p_.cross_min_length) &&
      (p_.cross_min_length < 4 || CanSplitNow()) && UnknownPortal(head_, ping_dir_) && !Never(head_, ping_dir_) &&
      BodyInView() && (goal < 0 || [&] {
        int32_t ready;
        return TileValue(goal, paths.dist[goal], &ready) == 0.0;
      }())) {
    // (a waller chasing a recently seen enemy head is not exploring: it doesn't cross)
    bool enemy = false;
    for (const auto& [id, w] : seen_)
      enemy |= w.team != spawn_.team && w.head >= 0 && (w.round == round_ || (goal >= 0 && w.head == goal));
    if (!enemy) {
      cross_round_ = round_;
      cross_tile_ = head_;
      cross_dir_ = ping_dir_;
      return Because("portal: explore", bot::Action::Move(ping_dir_));
    }
  }
  const int preferred = goal >= 0 ? paths.first[goal] : -1;
  cur_goal_ = goal;
  bot::Action a = SafeStep(preferred, target.trap);
  cur_goal_ = -1;
  // the last step onto an enemy head we chose to hunt is a trade, not a mistake
  if (goal >= 0 && preferred >= 0 && board_.Step(head_, preferred) == goal && Classify(goal) == Cell::kEnemyHead &&
      WorthTrading(board_.at(goal).occupant))
    return Because("hunt: trade", bot::Action::Move(preferred));
  if (held && std::string(reason_) == "toward goal") reason_ = "held: peer not clearly longer";
  return a;
}

}  // namespace bw15
