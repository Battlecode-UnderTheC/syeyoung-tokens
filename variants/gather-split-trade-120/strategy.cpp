#include "strategy.hpp"


#include <algorithm>
#include <cmath>
#include <tuple>
#include <functional>
#ifdef GST120_TUNE
#include <cstdlib>
#include <string>
#endif
#include <unordered_map>
#include <unordered_set>
#if defined(GST120_DEBUG) || defined(GST120_TRACE)
#include <atomic>
#include <cstdio>
#include <cstdlib>
#endif

namespace gst120 {
// body-wall-21 (ported via gst70): slot give-up at the cap (see 4b in Turn)
static constexpr int kGiveUpMaxLen = 3, kGiveUpFree = 1, kGiveUpMinUnits = 32, kGiveUpIdleDist = 6;
static constexpr int kGiveUpAllyDist = 5, kGiveUpAllyLen = 6, kGiveUpHeads = 3;
#ifdef GST120_DEBUG
static long g_escape = 0, g_turnout = 0, g_calls = 0;
#define GST120_COUNT(x) ++(x)
// idle-loop measurement: turns where the last 12 heads cover <= 6 tiles with nothing eaten
// g15-6 exposure probe: goal steps refused for being next to an enemy head, and what that head did
static std::atomic<long> g_exp{0}, g_exp_pearl{0}, g_exp_hit{0}, g_exp_away{0}, g_exp_unseen{0}, g_exp_hit_p{0}, g_exp_away_p{0};
static std::atomic<long> g_turns{0}, g_idle{0}, g_eaten{0}, g_cross{0}, g_bans{0}, g_kcross{0}, g_pdetour{0}, g_lifted{0};
static struct IdleReport {
  ~IdleReport() {
    if (g_turns) std::fprintf(stderr, "GST51 turns %ld idle %ld (%.2f%%) eaten %ld crossings %ld bans %ld portal-steps %ld portal-detours %ld lifted %ld\n", g_turns.load(),
                              g_idle.load(), 100.0 * g_idle / g_turns, g_eaten.load(), g_cross.load(), g_bans.load(),
                              g_kcross.load(), g_pdetour.load(), g_lifted.load());
    if (g_exp) std::fprintf(stderr, "EXPOSED refused %ld (pearl %ld): enemy stepped on it %ld (pearl %ld), stayed off %ld (pearl %ld), unseen %ld\n",
                            g_exp.load(), g_exp_pearl.load(), g_exp_hit.load(), g_exp_hit_p.load(), g_exp_away.load(), g_exp_away_p.load(), g_exp_unseen.load());
  }
} g_idle_report;
#else
#define GST120_COUNT(x)
#endif

StrategyAgent::StrategyAgent(const rl::Spawn& spawn, Params params)
    : p_(params), spawn_(spawn), board_(spawn.width, spawn.height), portal_(spawn.width * spawn.height, 0) {
#ifdef GST120_TUNE
  // GST120_P="name=value,..." overrides parameters (tuning builds only)
  if (const char* env = std::getenv("GST120_P")) {
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
#define GST120_SET(name) if (k == #name) p_.name = static_cast<decltype(p_.name)>(v);
      GST120_SET(split_min_length) GST120_SET(split_until_round) GST120_SET(feed_from_round) GST120_SET(trap_cost)
      GST120_SET(room_margin) GST120_SET(room_min) GST120_SET(escape_slack) GST120_SET(escape_min) GST120_SET(check_split)
      GST120_SET(liberties) GST120_SET(share_factor) GST120_SET(patrol) GST120_SET(hunt_radius) GST120_SET(explore_value)
      GST120_SET(escape_steps) GST120_SET(trap_small)
      GST120_SET(split_headroom) GST120_SET(turnout_budget) GST120_SET(doom_first) GST120_SET(ally_memory) GST120_SET(avoid_blind) GST120_SET(doomed_split_room) GST120_SET(leader_split_until) GST120_SET(share_leader) GST120_SET(feed_distance) GST120_SET(big_split_length) GST120_SET(champ_no_trap)
      GST120_SET(portal_cross) GST120_SET(portal_explore) GST120_SET(loop_break) GST120_SET(loop_ban) GST120_SET(known_portal) GST120_SET(blind_fallback)
      GST120_SET(wait_factor) GST120_SET(cluster_bonus) GST120_SET(cluster_radius) GST120_SET(cluster_pool)
      GST120_SET(crowd_threshold) GST120_SET(stale_divisor) GST120_SET(stale_cap)
      GST120_SET(centre_bonus) GST120_SET(centre_until) GST120_SET(centre_max_tiles)
      GST120_SET(fan_out_until) GST120_SET(fan_out_radius) GST120_SET(safe_step_fan_radius) GST120_SET(safe_step_fan_floor) GST120_SET(safe_step_fan_decay)
#undef GST120_SET
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
#ifdef GST120_TRACE
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

// g22-7 (cs40's g21-6 ported): how packed with our own dragons the tile's neighbourhood is,
// counting occupied-body density (not just other heads' goal claims, unlike ChooseGoal's
// share_factor above) so SafeStep can break ties away from the crowd. Diamond radius, same
// shape as the cluster_bonus scan; cheap (2*radius^2+2*radius+1 tiles, radius=2 -> 13).
int32_t StrategyAgent::AllyDensity(int32_t t, int32_t radius) const {
  int32_t n = 0;
  const int32_t x = board_.X(t), y = board_.Y(t);
  for (int32_t dy = -radius; dy <= radius; ++dy)
    for (int32_t dx = -radius + std::abs(dy); dx <= radius - std::abs(dy); ++dx) {
      const int32_t nb = board_.Tile(x + dx, y + dy);
      if (nb == t) continue;
      if (Mine(nb)) { ++n; continue; }
      if (board_.Current(nb) && board_.at(nb).occupant >= 0 && board_.at(nb).occupant_team == spawn_.team) ++n;
    }
  return n;
}

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

uint64_t StrategyAgent::Encode(const Report& r) const {
  const uint64_t x = board_.X(r.tile) & 63, y = board_.Y(r.tile) & 63;
  return (kMagic << 56) | (uint64_t(spawn_.team & 1) << 55) | (uint64_t(r.id & 4095) << 43) | (x << 37) | (y << 31) |
         (uint64_t(std::min(r.length, 4095)) << 19) | (uint64_t(r.round & 511) << 10) | uint64_t(r.lower ? 1 : 0);
}

void StrategyAgent::Decode(uint64_t w) {
  if ((w >> 56) != kMagic || int32_t((w >> 55) & 1) != (spawn_.team & 1) || (w & 1022) != 0) return;
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
#ifdef GST120_TRACE
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

bool StrategyAgent::Consolidate() const {
  // g21-1 (strategist-e P1): the round-based backstop always fires eventually ...
  if (round_ >= p_.feed_from_round) return true;
  // ... but before it, react to the opponent's own state instead of waiting on a clock.
  if (round_ < p_.reactive_min_round) return false;
  if (p_.reactive_count_margin > 0 && EnemiesSeen() >= units_ + p_.reactive_count_margin) return true;
  if (p_.reactive_longest_margin > 0) {
    const int32_t champ_len = Champion().length;
    int32_t enemy_longest = 0;
    for (const auto& [id, w] : seen_)
      if (w.team != spawn_.team && round_ - w.round <= p_.enemy_memory) enemy_longest = std::max(enemy_longest, w.length);
    if (enemy_longest >= champ_len + p_.reactive_longest_margin) return true;
  }
  return false;
}

// ---- where to go --------------------------------------------------------------------------------

// A pearl seen there is worth 1, discounted by how long ago (someone may have eaten it);
// a bed is worth 1 once its countdown runs out (rounds until then: *ready), a bed that
// ran out while out of view a bit less. Occupied tiles and unseen ones are worth nothing.
double StrategyAgent::TileValue(int32_t t, int32_t dist, int32_t* ready) const {
  return TileValue(t, dist, ready, Occupied(t));
}

double StrategyAgent::TileValue(int32_t t, int32_t dist, int32_t* ready, bool occupied) const {
  const TileMemory& m = board_.at(t);
  *ready = dist;
  if (m.seen_round < 0 || occupied) return 0.0;
  const int32_t age = round_ - m.seen_round;
  if (m.pearl) return std::pow(p_.pearl_decay, age);
  if (m.countdown < 0) return 0.0;                         // not a bed
  const int32_t due = m.countdown - age;                   // rounds until it grows a pearl
  if (due > 0) {
    // late-surge-17: waiting is charged double (dist steps are free, then due-dist waiting rounds cost wait_factor each)
    *ready = p_.wait_factor > 0 ? dist + static_cast<int32_t>(p_.wait_factor * std::max(0, due - dist))
                                 : std::max(dist, due);
    return 1.0;
  }
  return age == 0 ? 1.0 : p_.ripe_bed_value;               // due now (blocked while occupied)
}

int32_t StrategyAgent::RoomNeeded() const { return std::max(length_ + p_.room_margin, p_.room_min); }

StrategyAgent::Goal StrategyAgent::ChooseGoal(const Paths& paths) const {
  // hunting: a worker with dragons to spare goes for enemy heads it just saw
  if (!leader_ && units_ >= p_.hunt_min_units && !Consolidate()) {
    int32_t best = -1;
    for (const auto& [id, w] : seen_)
      if (w.team != spawn_.team && w.round == round_ && w.head >= 0 && paths.dist[w.head] > 0 &&
          paths.dist[w.head] <= p_.hunt_radius && WorthTrading(id) &&
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
  const bool champion = Consolidate() && Champion().id == spawn_.id;
  if (p_.share_factor < 1.0 && (!leader_ || p_.share_leader) && !champion)
    for (const auto& [id, w] : seen_)
      if (w.team == spawn_.team && w.round == round_ && w.head >= 0) allies.push_back(w.head);
  // g16-27: allies in view, for fan-out only (opening rounds, independent of share_factor/
  // leader gating above: even a leader should give way on a bare frontier tile)
  const bool fan_out = round_ < p_.fan_out_until;
  std::vector<int32_t> fan_allies;
  if (fan_out)
    for (const auto& [id, w] : seen_)
      if (w.team == spawn_.team && w.round == round_ && w.head >= 0) fan_allies.push_back(w.head);
  // g17-11: the same ally-head list, but unconditional on fan_out's opening window, for the
  // bed/pearl crowd discount below (pattern #35: bed-room scrums below the cap).
  std::vector<int32_t> crowd_allies;
  for (const auto& [id, w] : seen_)
    if (w.team == spawn_.team && w.round == round_ && w.head >= 0 && id != spawn_.id) crowd_allies.push_back(w.head);
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
  // body-wall-17 (ported g15-20): small-map opening centre pull
  const bool centre_pull = p_.centre_bonus > 0 && round_ < p_.centre_until &&
                           board_.width() * board_.height() <= p_.centre_max_tiles;
  const int32_t centre = board_.Tile(board_.width() / 2, board_.height() / 2);
  const double centre_far = std::max(1, board_.width() / 2 + board_.height() / 2);
  // swh17: many allies in view means beds nearby are already being worked, so stale
  // (long-unseen) ground is worth chasing even if it's far, and patrol (towards what
  // hasn't been seen longest) should compete directly instead of only running as a
  // fallback once nothing else scores.
  int32_t crowd = 0;
  for (const auto& [id, w] : seen_)
    if (w.team == spawn_.team && w.round == round_ && w.head >= 0 && id != spawn_.id) ++crowd;
  const bool crowded = crowd > p_.crowd_threshold;
  // gst96/ls17: which tiles are ours right now, once (Mine() walks the whole body otherwise:
  // O(length) per tile check adds up fast over a full-board pass)
  std::vector<uint8_t> mine_mask(board_.size(), 0);
  for (const int32_t t : body_)
    if (t >= 0) mine_mask[t] = 1;
  auto occ = [&](int32_t t) { return mine_mask[t] != 0 || (board_.Current(t) && board_.at(t).occupant >= 0); };
  // late-surge-17: each tile's value and its due (rounds until a bed ripens), computed once:
  // pearl/bed value does not depend on dist, so this doubles as the cluster-bonus lookup table
  // and removes the second full TileValue pass the naive version needed (calling TileValue per
  // neighbour of every tile, or twice per tile, was slow).
  std::vector<double> base(board_.size(), 0.0);
  std::vector<int32_t> due0(board_.size(), 0);  // > 0: a bed not yet ripe, due in this many rounds
  for (int32_t t = 0; t < board_.size(); ++t) {
    int32_t r;
    base[t] = TileValue(t, 0, &r, occ(t));
    const TileMemory& m = board_.at(t);
    if (base[t] > 0 && !m.pearl) due0[t] = std::max(0, m.countdown - (round_ - m.seen_round));
  }
  // late-surge-17's cluster bonus (0.5x nearby pearls/beds) needs a radius-2 scan per source
  // tile; scanning every one of them on every turn is too slow on a dense/large map (this
  // showed up as multi-ms turns on official maps in `check`). Score without it first (fast,
  // same cost as the parent), then only pay for the scan on the handful of tiles that are
  // actually in the running: the best `cluster_pool` candidates plus their own neighbours
  // (a lone lower-scoring bed next to a field should still catch the bonus).
  std::vector<Candidate> ranked;
  // g15-6: each enemy head in view claims only its enemy_claims nearest valuable tiles
  std::vector<std::unordered_set<int32_t>> claims;
  if (p_.enemy_claims > 0 && !enemies.empty()) {
    claims.resize(enemies.size());
    for (size_t i = 0; i < enemies.size(); ++i) {
      std::vector<std::pair<int32_t, int32_t>> near;  // (dist, tile)
      const int32_t e = enemies[i].first, ex = board_.X(e), ey = board_.Y(e), R = 8;
      for (int32_t dy = -R; dy <= R; ++dy)
        for (int32_t dx = -R + std::abs(dy); dx <= R - std::abs(dy); ++dx) {
          const int32_t n = board_.Tile(ex + dx, ey + dy);
          if (base[n] > 0) near.push_back({torus(e, n), n});
        }
      std::sort(near.begin(), near.end());
      // a tile at a tied distance is claimed too
      int32_t cut = -1;
      for (size_t k = 0; k < near.size(); ++k) {
        if (static_cast<int32_t>(k) >= p_.enemy_claims && near[k].first > cut) break;
        cut = near[k].first;
        claims[i].insert(near[k].second);
      }
    }
  }
  for (int32_t t = 0; t < board_.size(); ++t) {
    const int32_t d = paths.dist[t];
    if (d <= 0) continue;
    const double value0 = base[t];
    int32_t ready = d;
    if (due0[t] > 0)
      ready = p_.wait_factor > 0 ? d + static_cast<int32_t>(p_.wait_factor * std::max(0, due0[t] - d))
                                  : std::max(d, due0[t]);
    double score = value0 / (ready + 1);
    if (score > 0) ranked.push_back({t, score});
  }
  std::vector<uint8_t> cluster_at;  // tiles worth the radius-2 scan: top-scoring + their neighbours
  if (p_.cluster_bonus > 0 && !ranked.empty()) {
    cluster_at.assign(board_.size(), 0);
    const size_t m = std::min<size_t>(ranked.size(), p_.cluster_pool);
    std::partial_sort(ranked.begin(), ranked.begin() + m, ranked.end(),
                      [](const Candidate& a, const Candidate& b) { return a.score > b.score; });
    const int32_t R = p_.cluster_radius;
    for (size_t i = 0; i < m; ++i) {
      const int32_t t = ranked[i].tile, x = board_.X(t), y = board_.Y(t);
      for (int32_t dy = -R; dy <= R; ++dy)
        for (int32_t dx = -R + std::abs(dy); dx <= R - std::abs(dy); ++dx)
          cluster_at[board_.Tile(x + dx, y + dy)] = 1;
    }
  }
  for (int32_t t = 0; t < board_.size(); ++t) {
    const int32_t d = paths.dist[t];
    if (d <= 0) continue;
    const double value0 = base[t];
    int32_t ready = d;
    if (due0[t] > 0)
      ready = p_.wait_factor > 0 ? d + static_cast<int32_t>(p_.wait_factor * std::max(0, due0[t] - d))
                                  : std::max(d, due0[t]);
    double value = value0;
    if (value > 0 && p_.cluster_bonus > 0 && !cluster_at.empty() && cluster_at[t]) {
      // late-surge-17: a pearl or bed in a field is worth more than a lone one; a bed far
      // from ripe counts little
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
          near += v * (d + 1.0) / (r2 + 1.0);
        }
      value += p_.cluster_bonus * near;
    }
    double score = value / (ready + 1);
    const double pull = centre_pull ? 1.0 + p_.centre_bonus * std::max(0.0, 1.0 - torus(t, centre) / centre_far) : 1.0;
    score *= pull;
    // g17-11: a bed/pearl goal already crowded by ally heads is discounted so a dragon looks
    // elsewhere instead of piling into the same small room (pattern #35).
    if (score > 0 && value0 > 0 && !crowd_allies.empty()) {
      int32_t near_heads = 0;
      for (const int32_t a : crowd_allies)
        if (torus(a, t) <= p_.crowd_discount_radius) ++near_heads;
      if (near_heads > 0) score *= std::pow(p_.crowd_discount_factor, near_heads);
    }
    if (score > 0)
      for (const int32_t a : allies)
        if (torus(a, t) < d) {
          score *= p_.share_factor;
          break;
        }
    if (score > 0 && !enemies.empty()) {
      const TileMemory& m = board_.at(t);
      const int32_t due = m.pearl ? 0 : std::max(0, m.countdown - (round_ - m.seen_round));
      for (size_t i = 0; i < enemies.size(); ++i) {
        const auto& [e, id] = enemies[i];
        // g15-6: an enemy head eats one pearl at a time: it only claims its nearest few
        // (a trade's corpse line beside it is not all theirs)
        if (!claims.empty() && !claims[i].count(t)) continue;
        const int32_t theirs = std::max(torus(e, t), due);
        if (theirs < ready || (theirs == ready && due > 0 && ready == due && id < spawn_.id)) {
          score *= p_.contest_factor;
          break;
        }
      }
    }
    for (int dir = 0; dir < 4; ++dir)
      if (board_.Step(t, dir) == kUnknown && !(p_.portal_explore && (portal_[t] >> dir & 1) && !WillCross())) {
        // g16-27: replay pattern #33 - in the opening, a frontier tile close to an ally who
        // is already nearer to it than we are is probably that ally's target; give it no
        // explore credit so the scan lands on a different frontier tile instead of a
        // discounted copy of the same contested one.
        bool contested = false;
        if (fan_out)
          for (const int32_t a : fan_allies)
            if (torus(a, t) <= p_.fan_out_radius && torus(a, t) < d) {
              contested = true;
              break;
            }
        if (contested) break;
        // swh17: crowded means beds are already being fought over; ground unseen for a
        // long time earns a multiplier so it can compete with a bed several dragons pile
        // onto, instead of always losing for being merely far.
        double mult = 1.0;
        if (crowded) {
          const int32_t age = round_ - board_.at(t).seen_round;
          mult = std::min(p_.stale_cap, std::max(1.0, age / p_.stale_divisor));
        }
        score = std::max(score, pull * mult * p_.explore_value / (d + 1));
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
                       (p_.champ_no_trap && Consolidate() && Champion().id == spawn_.id) ||
                       (round_ >= p_.no_trap_from_round && length_ >= p_.no_trap_min_length);
  auto open = [&](int32_t t) { return !Occupied(t); };
  Goal best;
  double best_score = 0;
  std::vector<int32_t> pocket;
  for (size_t i = 0; i < k; ++i) {
    const Candidate& c = candidates[i];
    double score = c.score;
    bool trap = false, grow = false;
    if (RoomAround(board_, c.tile, open, need, &pocket) < need) {
      if (no_trap) continue;                          // near the cap: a dead end is a grave
      double gain = 0;
      int32_t pearls = 0;
      for (const int32_t t : pocket) {
        int32_t ready;
        const double v = TileValue(t, paths.dist[t] < 0 ? paths.dist[c.tile] : paths.dist[t], &ready);
        gain += v;
        if (v >= 0.5) ++pearls;
      }
      double cost = p_.trap_cost;
      if (p_.trap_loss) {
        const int32_t grown = length_ + pearls;
        if (static_cast<int32_t>(pocket.size()) >= grown + 1) cost = 0.0;   // room to follow our tail out
        else if (length_ >= 4) cost = p_.trap_stub;                        // split out, the stub stays
        else if (grown >= 4 && FreeSlots() >= 1) cost = p_.trap_stub, grow = true;
      }
      score = (gain - cost) / (paths.dist[c.tile] + 1);
      trap = true;
    }
    if (score > best_score) {
      best_score = score;
      best = {c.tile, trap, grow};
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

#ifdef GST120_DEBUG
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
      GST120_COUNT(g_turnout);
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
  GST120_COUNT(g_calls);
  for (size_t qi = 0; qi < queue.size(); ++qi) {
    GST120_COUNT(g_escape);
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

bot::Action StrategyAgent::SafeStep(int preferred, bool trap_ok, bool trap_grow) {
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
  int32_t best_density = 1 << 29;
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
    // (gst96) a known portal is not a one-way door: last turn's ping through it came back
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
#ifdef GST120_DEBUG
      if (lifted) ++g_lifted;
#endif
      if (lifted && room >= need) return Because(lifted, bot::Action::Move(d));
      return Because(room >= need ? "toward goal" : "into a dead end worth it", bot::Action::Move(d));
    }
#ifdef GST120_DEBUG
    if (k == 0 && preferred >= 0 && exposed) {
      exp_tile_ = to;
      exp_round_ = round_;
      exp_pearl_ = board_.Current(to) && board_.at(to).pearl;
      exp_ids_.clear();
      for (int e = 0; e < 4; ++e)
        if (const int32_t n = board_.Step(to, e); n >= 0 && Classify(n) == Cell::kEnemyHead) exp_ids_.push_back(board_.at(n).occupant);
      ++g_exp;
      if (exp_pearl_) ++g_exp_pearl;
    }
#endif
    if (k == 0 && preferred >= 0 && !exposed && libs >= 2 && trap_ok && (can_split_out || !p_.trap_small || trap_grow))
      return Because("into a dead end worth it", bot::Action::Move(d));
    // g22-7 (cs40's g21-6 ported): among otherwise-equal candidates (the tuple below ties),
    // prefer the tile with fewer of our own dragons nearby -- SafeStep's own contribution to
    // spreading the team out, on top of ChooseGoal's share_factor/fan_out discounts. Bucketed
    // by a round-decaying weight (safe_step_fan_floor..1.0) so the resolution narrows over the
    // game (matters most while the opening pack is forming) without ever fully disabling it.
    const double fan_weight = std::max(p_.safe_step_fan_floor, 1.0 - round_ / p_.safe_step_fan_decay);
    const int32_t density = p_.safe_step_fan_radius > 0
                                 ? static_cast<int32_t>(AllyDensity(to, p_.safe_step_fan_radius) * fan_weight)
                                 : 0;
    // a sure death in a dead end is worse than a tile an enemy head might take (that would be a trade)
    auto key = [&](bool e, int lv, int lb, int c, int r, int32_t dens) {
      return std::make_tuple(p_.doom_first ? lv > 0 : true, !e, lv, lb, c, r, -dens);
    };
    if (best < 0 || key(exposed, level, libs, count, room, density) >
                        key(best_exposed, best_level, best_libs, best_count, best_room, best_density)) {
      best = d;
      best_level = level;
      best_count = count;
      best_room = room;
      best_libs = libs;
      best_exposed = exposed;
      best_density = density;
    }
  }
  // every step is a dead end we can't turn round in: split while we still can (the child
  // walks out the way we came)
  if (best >= 0 && best_level == 0 && best_count <= p_.doomed_split_room && CanSplitNow())
    return Because("doomed: split", bot::Action::Split(length_ - 2));
#ifdef GST120_DEBUG
  if (best >= 0 && preferred >= 0 && (portal_[head_] >> preferred & 1)) ++g_pdetour;
#endif
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
#ifdef GST120_DEBUG
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
#ifdef GST120_DEBUG
  // the exposure probe: last turn's refused tile; did an adjacent enemy head move onto it?
  if (exp_round_ >= 0 && exp_round_ == static_cast<int32_t>(o.round) - 1) {
    int hit = 0, seen = 0;
    for (const int32_t id : exp_ids_) {
      for (const auto& b : o.segments)
        if (b.id == id && b.head) {
          ++seen;
          if (board_.Tile(b.x, b.y) == exp_tile_) hit = 1;
        }
    }
    if (hit) { ++g_exp_hit; if (exp_pearl_) ++g_exp_hit_p; }
    else if (seen == static_cast<int>(exp_ids_.size())) { ++g_exp_away; if (exp_pearl_) ++g_exp_away_p; }
    else ++g_exp_unseen;
  }
  exp_round_ = -1;
#endif
  bot::Action a = Decide(o);
  // tell the dragons around us who the champion is (a dead dragon's pings are not cast)
  const Report champ = Champion();
  if (champ.id >= 0)
    for (int d = 0; d < 4; ++d) a.sonar[d] = Encode(champ);
  if (obit_until_ > round_ && (round_ & 1))
    for (int d = 0; d < 4; ++d) a.sonar[d] = Encode(obit_);
  // (gst64) splitting most of our length off: say so this very turn. Our pings still named us at
  // the old length, the child heard that first (it was the ghost's seed: the child and everyone
  // near kept naming our soon-dead stub); now the child hears our stub length and names itself
  // (a trapped or doomed split only: growth splits are routine and their obituaries crowd the sonar)
  if (p_.split_announce && a.kind == bot::Action::kSplit && !a.resign && 2 * a.split >= length_ &&
      (reason_[0] == 't' || reason_[0] == 'd') &&
      reports_[spawn_.id].length >= p_.obit_champion) {
    const Report self{spawn_.id, head_, length_ - a.split, round_};
    reports_[spawn_.id] = self;
    obit_ = self;
    obit_until_ = round_ + p_.obit_rounds;
    for (int d = 0; d < 4; ++d) a.sonar[d] = Encode(self);
  }
#ifdef GST120_TRACE
  if (a.kind == bot::Action::kSplit)
    std::fprintf(stderr, "SPLITDBG known%d heard%d %s\n", int(static_cast<int32_t>(body_.size()) >= length_), int(HeardSplit(a.split)), reason_);
  if (round_ == 400 || round_ == 460 || (round_ > 400 && round_ % 10 == 0))
    std::fprintf(stderr, "CHAMPDBG r%d t%d id%d len%d champ%d\n", round_, spawn_.team, spawn_.id, length_, champ.id);
#endif
  // stepping next to an unknown portal: ping only through it, so next turn's echoes say
  // whether a dragon lies past its exit
  bool pinging = false;
  // (gst96) stepping back onto the tile beside a known portal whose blind step we refused: ping through it
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
        if (e != (d + 2) % 4 && UnknownPortal(n, e)) {
          a.sonar = {};
          a.sonar[e] = champ.id >= 0 ? Encode(champ) : 1;
          ping_round_ = round_;
          ping_tile_ = n;
          ping_dir_ = e;
          break;
        }
  }
#ifdef GST120_DEBUG
  ++g_turns;
  if (Looping()) ++g_idle;
  if (length_ > 0 && round_ == last_eat_ && round_ > born_) ++g_eaten;
  if (std::string(reason_) == "portal: explore") ++g_cross;
  if (a.kind == bot::Action::kMove && !a.resign && !a.steps.empty() && (portal_[head_] >> a.steps[0] & 1)) ++g_kcross;
#endif
  return a;
}

bot::Action StrategyAgent::Decide(const rl::Observation& o) {
  Remember(o);

  // 1. feeding: late in the game every dragon but the champion walks to it and dies next to
  // it, leaving it pearls (a leader only near the end, and only when it knows where it is)
  const Report champ = Champion();
  const bool feeding = champ.id >= 0 && champ.id != spawn_.id && champ.length >= length_ &&
                       (leader_ ? round_ >= p_.leader_feed_from && round_ - champ.round <= p_.leader_feed_fresh
                                : Consolidate());
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
      // g16-6: late, a long dragon merges into the champion on any margin instead of holding
      // for the usual clearance, so the team doesn't finish with two separate giants.
      const bool merge_ready = round_ >= p_.merge_round && length_ >= p_.merge_min_length;
      if (!merge_ready && length_ * 10 >= tl * p_.feed_clear_tenths) {
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
  if (units_ > 1 && !(Consolidate() && champ.id == spawn_.id)) {
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
  // g17-11: pattern #35 - don't split into a crowd; a child born where 3+ ally heads already
  // are just adds another short stub to the bed-room scrum that traps them all.
  int32_t heads_near = 0;
  for (const auto& [id, w] : seen_)
    if (w.team == spawn_.team && w.round == round_ && w.head >= 0 && id != spawn_.id &&
        std::abs(board_.X(w.head) - board_.X(head_)) + std::abs(board_.Y(w.head) - board_.Y(head_)) <=
            p_.crowd_discount_radius)
      ++heads_near;
  if (length_ >= p_.split_min_length && round_ < (leader_ ? p_.leader_split_until : p_.split_until_round) &&
      FreeSlots() > std::max(p_.split_headroom, p_.reserve_slots) &&
      !(Consolidate() && champ.id == spawn_.id) &&
      heads_near < p_.split_crowd_limit &&
      GrowthSplit() >= 0)
    return Because("split", bot::Action::Split(GrowthSplit()));

  // 5. a goal, and the first safe step towards it
  const Goal target = ally_head >= 0 && paths.dist[ally_head] > 0 ? Goal{ally_head, false} : ChooseGoal(paths);
  const int32_t goal = target.tile;
  // 4b. body-wall-21 (ported via gst70): free a slot at the cap. Every trapped death of length >= 4
  // happens at the unit cap with no split possible (league/KNOWLEDGE.md): an idle 2-3-long stub near
  // a long ally or a crowd of heads gives up its slot so a long dragon can escape-split. UNIT_COUNT
  // is live within a round, so stubs acting after one resigned see the freed slot (throttle).
  if (length_ >= 2 && length_ <= kGiveUpMaxLen && !leader_ && FreeSlots() <= kGiveUpFree && units_ > kGiveUpMinUnits &&
      champ.id != spawn_.id && ally_head < 0 && (goal < 0 || paths.dist[goal] > kGiveUpIdleDist)) {
    bool long_ally = false;
    int heads = 0;
    const int hx = board_.X(head_), hy = board_.Y(head_);
    for (const auto& [id, w] : seen_) {
      if (w.round != round_ || w.head < 0 || id == spawn_.id) continue;
      const int d = std::abs(board_.X(w.head) - hx) + std::abs(board_.Y(w.head) - hy);
      ++heads;
      if (w.team == spawn_.team && d <= kGiveUpAllyDist) {
        int32_t len = w.length;
        if (const auto r = reports_.find(id); r != reports_.end()) len = std::max(len, r->second.length);
        long_ally |= len >= kGiveUpAllyLen;
      }
    }
    if (long_ally || heads >= kGiveUpHeads)
      return Because(long_ally ? "give up slot: long ally" : "give up slot: crowd", bot::Action::Resign());
  }
  // loop breaker: circling without eating: ban this goal for a while
  // (not while waiting beside a bed for its countdown: circling there is the plan)
  const bool waiting = goal >= 0 && paths.dist[goal] > 0 && [&] {
    int32_t ready;
    return TileValue(goal, paths.dist[goal], &ready) > 0.0 && ready > paths.dist[goal];
  }();
  if (p_.loop_break && goal >= 0 && !waiting && Looping() && !(ally_head >= 0 && goal == ally_head)) {
    banned_[goal] = round_ + p_.loop_ban;
    if (last_goal_ >= 0) banned_[last_goal_] = round_ + p_.loop_ban;
#ifdef GST120_DEBUG
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
  cur_goal_ = goal;
  bot::Action a = SafeStep(preferred, target.trap, target.grow);
  cur_goal_ = -1;
  // the last step onto an enemy head we chose to hunt is a trade, not a mistake
  if (goal >= 0 && preferred >= 0 && board_.Step(head_, preferred) == goal && Classify(goal) == Cell::kEnemyHead &&
      WorthTrading(board_.at(goal).occupant))
    return Because("hunt: trade", bot::Action::Move(preferred));
  if (held && std::string(reason_) == "toward goal") reason_ = "held: peer not clearly longer";
  return a;
}

}  // namespace gst120
