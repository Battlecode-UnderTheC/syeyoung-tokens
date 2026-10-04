#include "strategy.hpp"


#include <algorithm>
#include <cmath>
#include <tuple>
#include <functional>
#include <unordered_map>
#include <unordered_set>
#if defined(GM2_DEBUG) || defined(GM2_TRACE)
#include <atomic>
#include <cstdio>
#include <cstdlib>
#endif

namespace gm2 {
// body-wall-21 (ported via gst70): slot give-up at the cap (see 4b in Turn)
static constexpr int kGiveUpMaxLen = 3, kGiveUpFree = 1, kGiveUpMinUnits = 32, kGiveUpIdleDist = 6;
static constexpr int kGiveUpAllyDist = 5, kGiveUpAllyLen = 6, kGiveUpHeads = 3;
// g38-4: near the cap (UNIT_COUNT >= UNIT_LIMIT-4) no worker of length 4+ makes a growth split;
// the give-up-slot resign below already applies only to length <= kGiveUpMaxLen (3).
static constexpr int kCapGrowMargin = 4;
static_assert(kGiveUpMaxLen <= 3, "g38-4: only length <= 3 may give up a slot");
// g39-5 (ring-noburn-1): no mass consolidation. A stub resigns only with its head directly next
// to the head of the dragon that will eat its pearls (feed and give-up alike), only dragons shorter
// than kFeedMaxLen feed, and from kNoBurnFrom, once the team has reached the cap, workers grow to
// kParallelGrowLen in parallel before they split again (released if the team falls kCapRelease
// below the cap).
static constexpr int kFeedMaxLen = 8, kNoBurnFrom = 250, kParallelGrowLen = 16, kCapRelease = 12;
#ifdef GM2_DEBUG
static long g_escape = 0, g_turnout = 0, g_calls = 0;
#define GM2_COUNT(x) ++(x)
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
#define GM2_COUNT(x)
#endif

StrategyAgent::StrategyAgent(const rl::Spawn& spawn, Params params)
    : p_(params), spawn_(spawn), board_(spawn.width, spawn.height), portal_(spawn.width * spawn.height, 0),
      cd_last_(spawn.width * spawn.height, -1), cd_round_(spawn.width * spawn.height, -1), hot_(spawn.width * spawn.height, 0) {
  sym_.Init(spawn.width, spawn.height);
}

// ---- what this dragon sees ------------------------------------------------------------------------

void StrategyAgent::Remember(const rl::Observation& o) {
  board_.Update(o);
  head_ = board_.Tile(o.head_x(), o.head_y());
  length_ = o.length;
  units_ = o.unit_count;
  // mass-swarm-2: the consolidation schedule follows the team's size, turn by turn. A big swarm
  // (food-rich or big map) keeps eating until kBigFinal; a small one consolidates from kFinal.
  {
    if (big_team_ < 0 && round_ >= kLatchRound) big_team_ = units_ >= kBigTeam ? 1 : 0;
    const bool big = big_team_ >= 0 ? big_team_ == 1 : units_ >= kBigTeam;
    const int32_t shift = big ? kBigFinal - kFinal : 0;
    p_.split_until_round = kFinal + shift;
    p_.leader_split_until = kFinal + shift;
    p_.feed_from_round = kFinal + shift;
    p_.no_trap_from_round = kFinal + shift;
    p_.leader_feed_from = kFinal + shift;
    p_.merge_round = kFinal + 30 + shift;
    p_.champ_freeze = shift ? kBigChampFreeze : kChampFreeze;
  }
  round_ = o.round;
  if (round_ >= kNoBurnFrom && units_ >= spawn_.unit_limit - kCapGrowMargin) cap_latched_ = true;
  if (units_ < spawn_.unit_limit - kCapRelease) cap_latched_ = false;
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
  std::unordered_map<int32_t, bool> cut, boundary;
  auto on_border = [&](const rl::Segment& s) {
    int32_t dx = std::abs(s.x - o.head_x()), dy = std::abs(s.y - o.head_y());
    dx = std::min(dx, board_.width() - dx), dy = std::min(dy, board_.height() - dy);
    return dx >= 3 || dy >= 3;
  };
  for (const rl::Segment& s : o.segments) {
    if (s.id == spawn_.id) continue;
    ++segments[s.id];
    cut[s.id] = on_border(s);
    boundary[s.id] = boundary[s.id] || on_border(s);
    Sighting& w = seen_[s.id];
    w.team = s.team;
    w.round = round_;
    if (s.head) w.head = board_.Tile(s.x, s.y);
  }
  for (const auto& [id, n] : segments) { seen_[id].length = n; seen_[id].partial = boundary[id] || n < 2; }
  for (const uint64_t w : o.inbox) Decode(w);
  // g39-sym: infer the map's symmetry and mirror what we have seen into board_
  sym_.Observe(o, board_);
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
#ifdef GM2_TRACE
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
  if (IsQueen()) return false;
  if (units_ <= 1) return false;
  const auto it = seen_.find(enemy_id);
  const int32_t theirs = it == seen_.end() ? 0 : it->second.length;
  if (kTradeValue && it != seen_.end() && it->second.head >= 0) {
    // ms8 (from arena-search-6/7, the search team's adversarial trade): a head-to-head kills both;
    // it pays when what they lose, less what we lose, plus our expected share of the remains
    // (ceil(L/2) pearls from each corpse, eaten by whoever has more heads nearby) is positive
    const int32_t at = it->second.head;
    int32_t ours = 0, all = 0;
    for (const auto& [id, w] : seen_) {
      if (id == enemy_id || id == spawn_.id || w.round != round_ || w.head < 0) continue;
      const int32_t dx = std::abs(board_.X(w.head) - board_.X(at)), dy = std::abs(board_.Y(w.head) - board_.Y(at));
      if (std::min(dx, board_.width() - dx) + std::min(dy, board_.height() - dy) > kShareRadius) continue;
      ++all;
      if (w.team == spawn_.team) ++ours;
    }
    const double share = (ours + 0.5) / (all + 1.0);
    const double remains = (theirs + 1) / 2 + (length_ + 1) / 2;
    return theirs - length_ + remains * (2.0 * share - 1.0) > kTradeMargin;
  }
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
    if (const int32_t n = board_.Step(t, d); n >= 0 && Classify(n) == Cell::kEnemyHead) {
      const int32_t id = board_.at(n).occupant;
      if (!WorthTrading(id)) return true;
      // gm2 (the Schooltime server-seed trace): the enemy, not us, decides whether a trade beside
      // its head happens, and a gst-style enemy takes it whenever we are the longer. A short dragon
      // (the ones that split next) keeps gst's caution: beside a shorter enemy head is exposed.
      if (length_ <= kCautiousMax) {
        const auto it = seen_.find(id);
        if (it != seen_.end() && it->second.length < length_) return true;
      }
    }
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
         (uint64_t(std::min(r.length, 4095)) << 19) | (uint64_t(r.round & 511) << 10) | (uint64_t(sym_.Usable()) << 1) | uint64_t(r.lower ? 1 : 0);
}

// ---- gst-merge-1 (from gst-hunter-1): hot beds -----------------------------------------------------
static constexpr uint64_t kClaimMagic = 0xA8;

void StrategyAgent::UpdateHot(const rl::Observation& o) {
  if (!p_.hot_farm) return;
  for (int k = 0; k < rl::kWindowTiles; ++k) {
    const int32_t c = o.countdown[k];
    if (c < 0) continue;
    const int32_t t = board_.Tile(o.tile_x[k], o.tile_y[k]);
    if (cd_round_[t] >= 0 && cd_round_[t] < round_) {
      const int32_t expected = cd_last_[t] - (round_ - cd_round_[t]);
      if (c > expected) hot_[t] = c <= p_.hot_gap ? 1 : 0;   // the countdown was reset: c is (about) the new gap
    }
    cd_last_[t] = c;
    cd_round_[t] = round_;
  }
}

bool StrategyAgent::ClaimedByOther(int32_t t) const {
  if (!p_.hot_claim) return false;
  for (const auto& [bed, who] : claims_) {
    if (who.first == spawn_.id || round_ - who.second > p_.hot_claim_memory) continue;
    int32_t dx = std::abs(board_.X(bed) - board_.X(t)), dy = std::abs(board_.Y(bed) - board_.Y(t));
    dx = std::min(dx, board_.width() - dx), dy = std::min(dy, board_.height() - dy);
    if (dx + dy <= p_.hot_claim_radius) return true;
  }
  return false;
}

void StrategyAgent::Decode(uint64_t w) {
  if ((w >> 56) == kClaimMagic) {
    if (int32_t((w >> 55) & 1) != (spawn_.team & 1)) return;
    const int32_t id = int32_t((w >> 43) & 4095), x = int32_t((w >> 37) & 63), y = int32_t((w >> 31) & 63),
                  r = int32_t((w >> 10) & 511);
    if (x >= board_.width() || y >= board_.height() || r > round_) return;
    auto& c = claims_[board_.Tile(x, y)];
    if (r >= c.second) c = {id, r};
    return;
  }
  if ((w >> 56) != kMagic || int32_t((w >> 55) & 1) != (spawn_.team & 1) || (w & 1008) != 0) return;
  sym_.Hint(uint8_t((w >> 1) & 7));   // g39-sym: bits 1-3 carry the sender's usable symmetry candidates
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
#ifdef GM2_TRACE
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
  if (IsQueen()) return Report{spawn_.id, head_, length_, round_, false};
  if (queen_id_ >= 0) {
    const auto q = reports_.find(queen_id_);
    if (q != reports_.end() && q->second.length > 0 && round_ - q->second.round <= 80)
      return q->second;
  }
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
  return TileValue(t, dist, ready, Occupied(t));
}

double StrategyAgent::TileValue(int32_t t, int32_t dist, int32_t* ready, bool occupied) const {
  const TileMemory& m = board_.at(t);
  *ready = dist;
  if (m.seen_round < 0 || occupied) return 0.0;
  const int32_t age = round_ - m.seen_round;
  if (m.pearl) return std::pow(p_.pearl_decay, age);
  if (m.countdown < 0) return 0.0;                         // not a bed
  if (p_.hot_farm && hot_[t] && ClaimedByOther(t)) return 0.0;   // another farmer's corridor
  const int32_t due = m.countdown - age;                   // rounds until it grows a pearl
  if (p_.hot_farm && hot_[t] && due <= 0) return 1.0;      // a hot bed refills every few rounds
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
  if (!leader_ && units_ >= p_.hunt_min_units && round_ < p_.feed_from_round) {
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
  const bool champion = round_ >= p_.feed_from_round && Champion().id == spawn_.id;
  if (p_.share_factor < 1.0 && (!leader_ || p_.share_leader) && !champion)
    for (const auto& [id, w] : seen_)
      if (w.team == spawn_.team && w.round == round_ && w.head >= 0) allies.push_back(w.head);
  // ms4: allies out of view that reported themselves by sonar a few rounds ago count too
  // (targets nearer to them are theirs): coordination at no protocol cost
  if (kSonarShare > 0 && p_.share_factor < 1.0 && (!leader_ || p_.share_leader) && !champion)
    for (const auto& [id, r] : reports_)
      if (id != spawn_.id && r.tile >= 0 && r.length > 0 && round_ - r.round <= kSonarShare) {
        const auto it = seen_.find(id);
        if (it == seen_.end() || it->second.round != round_) allies.push_back(r.tile);
      }
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
  // ring-explore-1 (explore-3's multiplier, hunger part): before r150 a worker (never the
  // queen) values unexplored ground more the longer it has gone without eating, up to x2
  // after kHungerRounds rounds.
  double hunger_mult = 1.0;
  if (EXPLORE_HUNGER && !IsQueen() && round_ < kHungerUntil) {
    const int32_t hungry = round_ - std::max(last_eat_, born_);
    hunger_mult = 1.0 + std::min(1.0, hungry / double(kHungerRounds));
  }
  // ring-explore-2 (explore-3's unseen-share term): before r150 a worker's explore score
  // also rises by the share of the board the team has never seen (+1 at the start, 0 once
  // the whole map is known). Additive with the hunger part, as in explore-3.
  if (EXPLORE_UNSEEN && !IsQueen() && round_ < kHungerUntil) {
    int32_t unseen = 0;
    for (int32_t t = 0; t < board_.size(); ++t) unseen += board_.at(t).seen_round < 0;
    hunger_mult += unseen / double(board_.size());
  }
  // gst115/ls17: which tiles are ours right now, once (Mine() walks the whole body otherwise:
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
    base[t] = (RING5_MODE==3&&Ring5Reserved(t))?0:TileValue(t, 0, &r, occ(t));
    const TileMemory& m = board_.at(t);
    if (base[t] > 0 && !m.pearl) due0[t] = std::max(0, m.countdown - (round_ - m.seen_round));
  }
  auto park_dist = [&](int32_t t) {
    const int32_t dx = std::abs(board_.X(t) - board_.X(anchor_)), dy = std::abs(board_.Y(t) - board_.Y(anchor_));
    return std::min(dx, board_.width() - dx) + std::min(dy, board_.height() - dy);
  };
  if (anchor_ >= 0)
    for (int32_t t = 0; t < board_.size(); ++t)
      if (base[t] > 0 && park_dist(t) > kPark) base[t] = 0.0;
  // ms6 "reserved ground": in the feeding phase the feeders' remains pile up around the champion,
  // and any other ally that eats them only has to die again later (halving them once more). So
  // every dragon but the champion leaves the pearls within kReserve of the champion's head alone.
  if (kReserve > 0 && round_ >= p_.feed_from_round) {
    const Report champ = Champion();
    if (champ.id >= 0 && champ.id != spawn_.id && round_ - champ.round <= kReserveFresh) {
      int32_t c = champ.tile;
      if (const auto it = seen_.find(champ.id); it != seen_.end() && it->second.round == round_ && it->second.head >= 0)
        c = it->second.head;
      const int32_t cx = board_.X(c), cy = board_.Y(c), w = board_.width(), h = board_.height();
      for (int dy = -kReserve; dy <= kReserve; ++dy)
        for (int dx = -kReserve + std::abs(dy); dx <= kReserve - std::abs(dy); ++dx) {
          const int32_t t = board_.Tile(((cx + dx) % w + w) % w, ((cy + dy) % h + h) % h);
          if (board_.at(t).pearl) base[t] = 0.0;
        }
    }
  }
  // g38-sched: a deterministic arrival-time assignment of the food near us among the dragons
  // that see each other. Every worker in view (the queen excluded) and we ourselves get a
  // breadth-first arrival time to each valued tile within kSchedHorizon rounds (a bed counts
  // from when it ripens). Pairs are matched greedily by (arrival, dragon id, tile), so every
  // dragon that sees the same allies computes the same matching without messages. A tile
  // matched to another dragon is that dragon's: we discount it like a shared tile. A tile
  // matched to us is ours: no crowd or share discount. One dragon takes one tile, so a
  // dragon nearest to two pearls no longer scares everyone off both, and the dragons left
  // over (more dragons than food) look further or explore.
  std::vector<int32_t> sched_owner;
  int32_t sched_mine = 1 << 20;   // the arrival time of the tile matched to us
  if (SCHED_MODE && round_ < p_.feed_from_round && !champion) {
    std::vector<std::pair<int32_t, int32_t>> parts{{spawn_.id, head_}};   // (id, head)
    for (const auto& [id, w] : seen_)
      if (w.team == spawn_.team && w.round == round_ && w.head >= 0 && id != spawn_.id && id > 1 && w.head != head_)
        parts.push_back({id, w.head});
    if (parts.size() >= 2 || SCHED_MODE >= 2) {
      std::sort(parts.begin(), parts.end());
      if (static_cast<int32_t>(parts.size()) > kSchedMaxParts) parts.resize(kSchedMaxParts);
      struct Pair { int32_t ready, id, tile; };
      std::vector<Pair> pairs;
      auto ready_of = [&](int32_t t, int32_t d) { return std::max(d, due0[t]); };
      for (const auto& [id, h] : parts) {
        if (id == spawn_.id) {
          for (int32_t t = 0; t < board_.size(); ++t) {
            const int32_t d = paths.dist[t];
            if (d > 0 && base[t] > 0 && ready_of(t, d) <= kSchedHorizon) pairs.push_back({ready_of(t, d), id, t});
          }
          continue;
        }
        // an ally's arrival times: breadth-first from its head over the free tiles we know
        sched_seen_.Begin(board_.size());
        sched_queue_.clear();
        sched_queue_.push_back(h);
        sched_seen_.Set(h, 0);
        for (size_t qi = 0; qi < sched_queue_.size(); ++qi) {
          const int32_t t = sched_queue_[qi], d = sched_seen_.Get(t);
          ++work_;
          if (d > 0 && base[t] > 0 && ready_of(t, d) <= kSchedHorizon) pairs.push_back({ready_of(t, d), id, t});
          if (d >= kSchedHorizon) continue;
          for (int dir = 0; dir < 4; ++dir) {
            const int32_t n = board_.Step(t, dir);
            if (n < 0 || sched_seen_.Has(n) || occ(n)) continue;
            sched_seen_.Set(n, d + 1);
            sched_queue_.push_back(n);
          }
        }
      }
      if (!pairs.empty()) {
        std::sort(pairs.begin(), pairs.end(), [](const Pair& a, const Pair& b) {
          return std::tie(a.ready, a.id, a.tile) < std::tie(b.ready, b.id, b.tile);
        });
        sched_owner.assign(board_.size(), -1);
        std::vector<int32_t> done;
        for (const Pair& p : pairs) {
          if (sched_owner[p.tile] >= 0 || std::find(done.begin(), done.end(), p.id) != done.end()) continue;
          sched_owner[p.tile] = p.id;
          done.push_back(p.id);
          if (p.id == spawn_.id) sched_mine = p.ready;
          if (done.size() == parts.size()) break;
        }
      }
    }
  }
  // (SCHED_MODE 2) roles from the matching: a worker matched to food it reaches within
  // kSchedHarvest rounds is a harvester and drops the hunger/unseen explore multiplier;
  // the unmatched ones keep it and explore
  if (SCHED_MODE >= 2 && sched_mine <= kSchedHarvest) hunger_mult = 1.0;
  auto sched_other = [&](int32_t t) { return !sched_owner.empty() && sched_owner[t] >= 0 && sched_owner[t] != spawn_.id; };
  auto sched_any = [&](int32_t t) { return !sched_owner.empty() && sched_owner[t] >= 0; };
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
  // (gst115, from gst83/gst98/g16-26) seen-but-unreachable valued tiles: counted alongside the
  // reachable pass so the poor check below can ask whether more is walled off from us than is
  // actually open
  int32_t unreachable = 0;
  // g22-1 (strategist-g P1): the nearest reachable pearl, tracked in this pass already over
  // every tile (a second full-board scan just for the opening fallback below was 5x slower on
  // dense maps: `check` showed default's slowest turn jump from 0.4ms to 2.1ms).
  int32_t nearest_pearl = -1, nearest_pearl_d = p_.opening_pearl_radius + 1;
  double nearest_pearl_score = 0.0;
  const bool want_pearl_fallback = round_ < p_.opening_end;
  for (int32_t t = 0; t < board_.size(); ++t) {
    const int32_t d = paths.dist[t];
    if (d <= 0) {
      if (base[t] > 0) ++unreachable;
      continue;
    }
    if (want_pearl_fallback && d < nearest_pearl_d && board_.at(t).pearl && !Banned(t) && !sched_other(t)) {
      nearest_pearl = t;
      nearest_pearl_d = d;
      nearest_pearl_score = base[t] / (d + 1.0);
    }
    const double value0 = base[t];
    int32_t ready = d;
    if (due0[t] > 0)
      ready = p_.wait_factor > 0 ? d + static_cast<int32_t>(p_.wait_factor * std::max(0, due0[t] - d))
                                  : std::max(d, due0[t]);
    double score = value0 / (ready + 1);
    if (score > 0) ranked.push_back({t, score});
  }
  // (gst115, from gst83/gst98/g16-26) a poor home region: few known reachable pearls/beds
  // relative to allies actually seen near us this round (not the whole team, so a dragon whose
  // own corner is fine isn't diluted by teammates elsewhere on the map), nothing eaten for a
  // while, and more seen-but-unreachable valued tiles than reachable ones (so a merely
  // spread-out dragon with open food nearby doesn't cross, but one genuinely locked off does).
  // See the explore scan below and the cross decision in Decide() for where poor_ is used.
  {
    int32_t allies_here = 1;  // ourselves
    for (const auto& [id, w] : seen_)
      if (w.team == spawn_.team && w.round == round_ && id != spawn_.id) ++allies_here;
    const int32_t reachable = static_cast<int32_t>(ranked.size());
    poor_ = round_ < p_.cross_until &&
            (reachable < std::max(p_.poor_min, p_.poor_per_unit * allies_here) ||
             (round_ - std::max(last_eat_, born_) >= p_.starve_rounds)) &&
            unreachable > reachable;
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
    if (score > 0 && value0 > 0 && sched_any(t)) {
      // g38-sched: matched tiles replace the crowd and share discounts
      if (sched_other(t)) score *= kSchedOther;
    } else {
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
        double pm = 1.0;
        if (poor_ && WillCross())
          for (int e = 0; e < 4; ++e)
            if (UnknownPortal(t, e)) pm = p_.poor_portal_mult;
        score = std::max(score, pull * mult * pm * hunger_mult * p_.explore_value / (d + 1));
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
                       (p_.champ_no_trap && round_ >= p_.champ_freeze && Champion().id == spawn_.id) ||
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
      int32_t hot = 0;
      for (const int32_t t : pocket) {
        int32_t ready;
        const double v = TileValue(t, paths.dist[t] < 0 ? paths.dist[c.tile] : paths.dist[t], &ready);
        gain += v;
        if (v >= 0.5) ++pearls;
        if (p_.hot_farm && hot_[t] && v > 0) ++hot;
      }
      gain += hot * p_.hot_pocket_bonus;
      if (hot > 0 && length_ < 4 && length_ + pearls >= 4 && FreeSlots() >= 1) grow = true;
      // g22-1 (strategist-g P3): the opening is a throughput race, not a farming problem;
      // pay dead-end costs lightly while it still matters to move fast.
      double cost = round_ < p_.opening_end ? p_.trap_opening_cost : p_.trap_cost;
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
  // g22-1 (strategist-g P1): in the opening, nothing beat the dead-end/crowd/contest scoring
  // above (or every candidate got trapped and no_trap refused it) -- ts8's plain greedy
  // nearest-pearl search out-throughputs that caution. Fall back to the nearest reachable
  // pearl, unscored, rather than idling into patrol or roomiest-step stalling.
  // g22-1 (round 2): don't wait for best.tile<0 -- Devil's open centre means the scored scan
  // almost always finds *something*, so "nothing scored" never fires there and the fallback
  // was dead code (orchestrator's finding: r50 numbers unchanged from baseline). Prefer the
  // nearest reachable pearl whenever the best scored candidate is weak relative to it: either
  // there's no candidate at all, or the pearl's own plain score (ignoring crowd/share/contest/
  // cluster discounts, same as ts8's greedy search) beats what the careful scan settled on.
  if (want_pearl_fallback && nearest_pearl >= 0 &&
      (best.tile < 0 || best_score < nearest_pearl_score * p_.opening_pearl_factor))
    best = {nearest_pearl, false};
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
  // ms6 parking: never leave the parking ground for a goal; head home instead
  if (anchor_ >= 0 && (best.tile < 0 || park_dist(best.tile) > kPark) && paths.dist[anchor_] > 0)
    best = {anchor_, false};
  return best;
}

// ---- how to step ----------------------------------------------------------------------------------

#ifdef GM2_DEBUG
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
      GM2_COUNT(g_turnout);
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
  // g37-3 (legend-bud, from queen-ring17-first-then-grow-5): before r360 a long dragon buds off a
  // 2-long child instead of halving, so it keeps its mass.
  if (round_ < 360 && length_ >= 6 && SplitSafe(2)) return 2;
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
  // (ring-cpu-1) epoch scratch in place of two board-sized arrays per call; same marks
  Scratch& index = esc_index_;                         // own segment index, head 0
  index.Begin(n);
  if (blocked)
    for (const int32_t t : *blocked) index.Set(t, 1 << 20);   // never freed in time
  for (int32_t i = len - 1; i >= 0; --i) index.Set(body[i], i);
  Scratch& when = esc_when_;                           // time a tile was entered
  when.Begin(n);
  std::vector<int32_t>& queue = esc_queue_;
  queue.assign(1, body[0]);
  when.Set(body[0], 0);
  int32_t reached = 1;
  auto done = [&](int level) {
    if (count) *count = std::min(reached, cap);
    return level;
  };
  GM2_COUNT(g_calls);
  for (size_t qi = 0; qi < queue.size(); ++qi) {
    GM2_COUNT(g_escape);
    ++work_;
    const int32_t t = queue[qi], m = when.Get(t) + 1;  // the move that would enter a neighbour
    for (int d = 0; d < 4; ++d) {
      const int32_t to = board_.Step(t, d);
      if (to == kUnknown) {
        // ms4 (territory team's finding): an unseen edge is not a guaranteed exit; count it
        // as kUnknownWorth tiles of room (0: the old rule, an escape)
        if (kUnknownWorth <= 0) return done(2);
        reached += kUnknownWorth;
        if (reached >= cap) return done(1);
        continue;
      }
      if (to < 0 || when.Has(to)) continue;
      if (index.Has(to)) {
        const int32_t i = index.Get(to);
        if (i < len && i >= len - m + 1 && i > 0) return done(2);   // freed by then: follow the tail
        continue;                                      // still there (maybe later via a longer way)
      }
      if (board_.Current(to) && board_.at(to).occupant >= 0 && board_.at(to).occupant != spawn_.id) continue;
      when.Set(to, m);
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
    // (gst115) a known portal is not a one-way door: last turn's ping through it came back
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
#ifdef GM2_DEBUG
      if (lifted) ++g_lifted;
#endif
      if (lifted && room >= need) return Because(lifted, bot::Action::Move(d));
      return Because(room >= need ? "toward goal" : "into a dead end worth it", bot::Action::Move(d));
    }
#ifdef GM2_DEBUG
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
#ifdef GM2_DEBUG
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
#ifdef GM2_DEBUG
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

// A feeder's head must be reachable from the queen without crossing another body.
// The first pearl of its corpse is at that head; distant body segments are not a delivery.
bool StrategyAgent::FeedAhead(const Report& queen) const {
  const auto q = seen_.find(queen.id);
  if (q == seen_.end() || q->second.round != round_ || q->second.head < 0) return false;
  for (int d = 0; d < 4; ++d) {
    int t = q->second.head;
    for (int n = 1; n <= 2; ++n) {
      t = board_.Step(t, d);
      if (t < 0 || !board_.Current(t)) break;
      if (t == head_) return true;
      if (Occupied(t)) break;
    }
  }
  return false;
}

bool StrategyAgent::Corridor(int32_t t) const {
  int kelp=0;
  for(int d=0;d<4;++d) if(board_.Step(t,d)==kBlocked) ++kelp;
  return kelp>=2;
}

// Tiles reachable from t without crossing the given body or any occupied tile, up to cap.
// An unseen edge counts as open ground.
int StrategyAgent::OpenRoom(int32_t t, const std::vector<int32_t>& body, int cap) const {
  std::vector<uint8_t> seen(board_.size(),0);
  for(const int32_t b:body) if(b>=0) seen[b]=1;
  std::vector<int32_t> queue{t};seen[t]=1;
  int reached=1;
  for(size_t qi=0;qi<queue.size();++qi) {
    for(int d=0;d<4;++d) {
      const int32_t to=board_.Step(queue[qi],d);
      if(to==kUnknown) return cap;
      if(to<0 || seen[to]) continue;
      seen[to]=1;
      if(board_.Current(to) && board_.at(to).occupant>=0) continue;
      if(++reached>=cap) return cap;
      queue.push_back(to);
    }
  }
  return reached;
}

bot::Action StrategyAgent::QueenMove() {
  if (queen_home_ < 0) queen_home_ = head_;
  queen_id_ = spawn_.id;
  // Latch only the real initial population, never a late-game collapse.
  // Before R430 the existing growth flag can only be set by this policy.
  if(EARLY15_MODE && round_==0 && units_<=((EARLY15_MODE>=4 && EARLY15_MODE!=8) ? 1 : 2)) {
    queen_growing_=true;ring_.clear();
  }
  // g35-1: gate on the queen's observed starting length and the team's size, never the map.
  if(FORAGE35_MODE && round_==0 && units_<=2 && length_<=3) {
    queen_forage_=true;queen_growing_=true;ring_.clear();
  }
  if(queen_forage_ && !queen_growing_) queen_forage_=false;
  bool early_roaming=EARLY15_MODE && queen_growing_ && round_<430;
  if(queen_forage_ && early_roaming && (units_>=kForageSettleUnits || round_>=kForageSettleRound)) {
    // Settle into the ring: split off a worker when that is safe, else just stop roaming.
    if(length_>=4 && FreeSlots()>0 && int(body_.size())==length_ && SplitSafe(length_-2)) {
      queen_growing_=false;queen_forage_=false;ring_.clear();
      return Because("forage35: settle split",bot::Action::Split(length_-2));
    }
    queen_growing_=false;queen_forage_=false;ring_.clear();early_roaming=false;
  }
  // Solo-only bootstrap: first successful child production ends harvesting.
  // Population is only used for the fallback when another existing split rule
  // made the first child; the main path splits at the legal minimum length4.
  if(EARLY15_MODE==4 && early_roaming) {
    if(units_>=2) {queen_growing_=false;ring_.clear();early_roaming=false;}
    else if(length_>=4 && FreeSlots()>0 && int(body_.size())==length_ && SplitSafe(length_-2)) {
      queen_growing_=false;ring_.clear();
      return Because("queen16: first child",bot::Action::Split(length_-2));
    }
  }
  const bool settle=!queen_forage_ && early_roaming && ((EARLY15_MODE==1 && round_>=100) ||
    ((EARLY15_MODE==2 || EARLY15_MODE==5 || EARLY15_MODE>=6) && (units_>=4 || round_>=200)));
  if(settle) {
    if(length_>=4 && FreeSlots()>0 && int(body_.size())==length_ && SplitSafe(length_-2)) {
      queen_growing_=false;ring_.clear();
      return Because("queen15: settle split",bot::Action::Split(length_-2));
    }
    if(length_<=3) {queen_growing_=false;ring_.clear();early_roaming=false;}
    // If shortening is unsafe, defer settling rather than force a lethal split.
  }
  if (RING4_STAGE >= 1 && round_ >= 430 && !queen_growing_) {
    auto exit = Ring4Exit();
    if (!exit.steps.empty()) return exit;
  }
  if (false && !queen_growing_ && round_ >= 440 && GrowthReady()) {
    queen_growing_ = true;
    ring_.clear();
  }
  auto ring_action = queen_growing_ ? bot::Action{} : RingQueen();
  if (ring_action.kind == bot::Action::kSplit || !ring_action.steps.empty()) return ring_action;
  auto distance = [&](int a, int b) {
    int dx = std::abs(board_.X(a) - board_.X(b));
    int dy = std::abs(board_.Y(a) - board_.Y(b));
    return std::min(dx, board_.width() - dx) + std::min(dy, board_.height() - dy);
  };
  std::vector<uint8_t> threat(board_.size(), 0);
  for (const auto& [id, e] : seen_) {
    if (e.team == spawn_.team || e.round != round_ || e.head < 0 || !board_.Current(e.head) || board_.at(e.head).occupant != id || !board_.at(e.head).occupant_head) continue;
    // Visible lengths can be lower bounds; use at least a three-step danger zone.
    const int reach = QUEEN_ENDGAME && e.partial ? 8 : std::min(8, std::max(3, (e.length + 3) / 4 + e.length - 2));
    const auto paths = Search(board_, e.head, [&](int t) {
      return t == head_ || !Occupied(t);
    }, reach);
    for (int t = 0; t < board_.size(); ++t)
      if (paths.dist[t] >= 0 && paths.dist[t] <= reach) threat[t] = 1;
  }
  // Harvesting continues after an early split. Mode6 bootstraps/replaces
  // the first worker at length4, then uses the existing retain-six splits.
  // Modes7/8 repeatedly split down to length2 until the colony4 settle rule.
  if(EARLY15_MODE>=6 && early_roaming && (EARLY15_MODE!=6 || units_<=1 || queen_forage_) &&
      length_>=4 && FreeSlots()>0 && !threat[head_] &&
      !(queen_forage_ && Corridor(head_)) &&
      int(body_.size())==length_ && SplitSafe(length_-2))
    return Because(EARLY15_MODE==6 ? "queen17: bootstrap and harvest" : "queen17: fast split and harvest",
                   bot::Action::Split(length_-2));
  if (QUEEN_SHORT && round_ < kPantryFrom && length_ >= 8 && FreeSlots() > 1 &&
      !threat[head_] && static_cast<int>(body_.size()) == length_ && SplitSafe(length_ - 6))
    return Because("queen: shed to six", bot::Action::Split(length_ - 6));

  bool closed_branch[4]={};
  if(RING11_BRANCH_MODE && round_>=430 && round_<499) {
    for(int first=0;first<4;++first) {
      int previous=head_,tile=board_.Step(head_,first);
      std::vector<int> visited;
      for(int depth=0;tile>=0 && depth<64;++depth) {
        if(std::find(visited.begin(),visited.end(),tile)!=visited.end())break;
        visited.push_back(tile);
        bool unknown=false;std::vector<int> forward;
        for(int d=0;d<4;++d) {
          int next=board_.Step(tile,d);
          if(next==kUnknown){unknown=true;break;}
          if(next>=0 && next!=previous && std::find(forward.begin(),forward.end(),next)==forward.end())forward.push_back(next);
        }
        if(unknown)break;
        if(forward.empty()){closed_branch[first]=true;break;}
        if(forward.size()>1)break;
        previous=tile;tile=forward[0];
      }
    }
  }
  struct Route {
    std::vector<int32_t> body;
    std::vector<int> steps;
    std::vector<int> eaten;
    double gain = 0, value = 0;
    int transit_risk = 0;
    bool corridor = false;
  };
  Route root;
  root.body.assign(body_.begin(), body_.end());
  std::vector<Route> beam{root};
  Route best;
  std::vector<Route> guard_candidates;
  double best_value = -1e30;
  const bool exact_guard = RING8_MODE && round_>=430 && int(body_.size())==length_;
  const int engine_free_steps = (length_ + 3) / 4;
  const bool rescue_search=RING11_RESCUE_MODE && !RING8_MODE && round_>=430 && int(body_.size())==length_;
  const int original_limit=exact_guard ? 12 : std::min(6,engine_free_steps);
  int max_steps=original_limit,free_future=0;
  const int rescue_horizon=std::min(4,499-round_);
  const int rescue_threshold=RING11_RESCUE_MODE==2 ? std::min(1,rescue_horizon) : rescue_horizon;
  // (ring-cpu-1) body_ is fixed during the beam: Mine() as a mark set (TileValue's Occupied)
  q_mine_.Begin(board_.size());
  for (const int32_t t : body_) if (t >= 0) q_mine_.Set(t);
  bool over_budget = false;
  for (int depth = 0; depth < max_steps && !over_budget; ++depth) {
    std::vector<Route> next;
    for (const auto& route : beam) {
    // (ring-cpu-1) CPU safety net: past kQueenWorkCap the beam stops and keeps the best route
    // found so far (the first depth always runs in full, so a legal step is never lost)
    if (depth > 0 && work_ > kQueenWorkCap) { over_budget = true; break; }
    for (int d = 0; d < 4; ++d) {
      const bool paid = depth >= engine_free_steps;
      if(paid && route.body.size()<=2)continue;
      int t = board_.Step(route.body.front(), d);
      if (t < 0 || !board_.Current(t) ||
          std::find(route.body.begin(), route.body.end(), t) != route.body.end()) continue;
      const auto& cell = board_.at(t);
      if (cell.occupant >= 0 && cell.occupant != spawn_.id) continue;
      // A visible own segment missing from our partial memory is still occupied.
      // Only a segment present in the original body may become free during this sprint.
      if (cell.occupant == spawn_.id &&
          std::find(body_.begin(), body_.end(), t) == body_.end()) continue;
      work_ += 16 + int(route.body.size()) / 4;   // the copies below
      Route candidate = route;
      if (QUEEN_ENDGAME && round_ >= QUEEN_SAFETY_FROM)
        candidate.transit_risk = std::max(candidate.transit_risk, QueenRisk(t));
      candidate.steps.push_back(d);
      candidate.body.insert(candidate.body.begin(), t);
      if (cell.pearl && std::find(candidate.eaten.begin(), candidate.eaten.end(), t) == candidate.eaten.end()) {
        candidate.eaten.push_back(t);
        candidate.gain += 12;
      } else if (exact_guard || rescue_search) {
        candidate.body.pop_back();
      } else if (static_cast<int>(candidate.body.size()) > length_ + static_cast<int>(candidate.eaten.size())) {
        candidate.body.pop_back();
      }
      if(paid){candidate.body.pop_back();candidate.gain-=12;}
      int room = 0;
      const int escape = EscapeLevel(candidate.body, std::max(18, 2 * int(candidate.body.size())), &room);
      // (ring-cpu-1) the candidate's body as a mark set: same tests as std::find over it
      q_body_.Begin(board_.size());
      for (const int32_t b : candidate.body) if (b >= 0) q_body_.Set(b);
      int liberties = 0;
      for (int direction = 0; direction < 4; ++direction) {
        const int n = board_.Step(t, direction);
        if (n >= 0 && !q_body_.Has(n) &&
            !(board_.Current(n) && board_.at(n).occupant >= 0 && board_.at(n).occupant != spawn_.id)) ++liberties;
      }
      // (ring-cpu-1) Search(board_, t, enter, 12) without board-sized allocations; the food
      // term is a max over the tiles it reached, so only those are visited
      double food = 0;
      {
        q_dist_.Begin(board_.size());
        q_queue_.assign(1, t);
        q_dist_.Set(t, 0);
        for (size_t qi = 0; qi < q_queue_.size(); ++qi) {
          const int32_t u = q_queue_[qi], du = q_dist_.Get(u);
          ++work_;
          if (du >= 12) continue;
          for (int dir = 0; dir < 4; ++dir) {
            const int32_t n = board_.Step(u, dir);
            if (n < 0 || q_dist_.Has(n)) continue;
            if (q_body_.Has(n) || (board_.Current(n) && board_.at(n).occupant >= 0 && board_.at(n).occupant != spawn_.id)) continue;
            q_dist_.Set(n, du + 1);
            q_queue_.push_back(n);
          }
        }
        for (size_t qi = 1; qi < q_queue_.size(); ++qi) {
          const int32_t n = q_queue_[qi], dn = q_dist_.Get(n);
          ++work_;
          int ready;
          const bool occupied = q_mine_.Has(n) || (board_.Current(n) && board_.at(n).occupant >= 0);
          const double value = TileValue(n, dn, &ready, occupied);
          food = std::max(food, 6 * value / (1 + std::max(ready, dn)));
        }
      }
      // Compare safe destinations first, but retain a legal escape when every route is exposed.
      candidate.value = candidate.gain + (QUEEN_ENDGAME && round_ == 499 ? 0 : food) + 0.03 * room + 0.3 * liberties
        - (QUEEN_ENDGAME && round_ >= QUEEN_SAFETY_FROM ? 10000.0 * QueenRisk(t) + 1000.0 * candidate.transit_risk : 0)
        - (escape == 0 && !(QUEEN_ENDGAME && round_ == 499) ? (QUEEN_ENDGAME ? 1000000 : 1000) : 0)
        - (liberties == 0 && !(QUEEN_ENDGAME && round_ == 499) ? (QUEEN_ENDGAME ? 1000000 : 1000) : 0)
        - (threat[t] ? 100 : 0)
        - ((EARLY15_MODE==3 || queen_forage_) && early_roaming ? 0.0 : (round_ < kPantryFrom ? 0.6 : 0.1)) * std::max(0, distance(t, queen_home_) - 6)
        - 0.03 * candidate.steps.size();
      if(queen_forage_ && early_roaming) {
        // The queen plays safe while foraging: no corridor tiles on the way, no low-room endpoints.
        if(Corridor(t)) candidate.corridor=true;
        if(candidate.corridor) candidate.value-=20.0;
        const int open=OpenRoom(t,candidate.body,kForageRoom);
        if(open<kForageRoom) candidate.value-=2.0*(kForageRoom-open);
      }
      if(EARLY15_MODE==3 && early_roaming) {
        const int repeats=std::count(recent_.begin(),recent_.end(),t);
        candidate.value-=0.4*repeats; // Break food-free local cycles without changing danger penalties.
      }
      // Experiment: prefer a destination outside visible enemy reach even when
      // the static continuation heuristic dislikes it. Only applies after R430.
      if(closed_branch[candidate.steps.front()])candidate.value-=4000000.0;
      if(RING10_DANGER_MODE && round_>=430 && QueenRisk(t)>0)
        candidate.value-=4000000.0;
      if(RING6_MODE>=2&&round_>=430&&round_<499){
        int branches=0;int future=QueenContinuation(candidate.body,candidate.eaten,&branches);
        const int horizon=std::min(4,499-round_);
        if(rescue_search && depth<original_limit)free_future=std::max(free_future,future);
        candidate.value-=200.0*(horizon-future);
        if(branches<2)candidate.value-=18.0;
        // Ally heads can occupy currently free continuations before the queen acts again.
        int exposed=0;
        for(int direction=0;direction<4;++direction){int n=board_.Step(t,direction);if(n<0||!board_.Current(n)||board_.at(n).occupant>=0||std::find(candidate.body.begin(),candidate.body.end(),n)!=candidate.body.end())continue;
          bool risk=false;for(const auto&[id,e]:seen_)if(id!=spawn_.id&&e.team==spawn_.team&&e.round==round_&&e.head>=0&&board_.Current(e.head)&&board_.at(e.head).occupant==id&&board_.at(e.head).occupant_head)
            for(int d=0;d<4;++d)risk|=board_.Step(e.head,d)==n;
          exposed+=risk;
        }
        candidate.value-=8.0*exposed;
      }
      if((RING5_MODE==1||RING5_MODE==2)&&!queen_growing_&&round_<430&&round_<=ring5_recover_until_) {
        candidate.value-=candidate.gain; // recovery is not a growth phase
        candidate.value-=2.0*candidate.eaten.size();
        if(RING5_MODE==2&&t==ring5_next_&&escape>0&&liberties>0)candidate.value+=8;
      }
      if (candidate.value > best_value) { best_value = candidate.value; best = candidate; }
      if(exact_guard)guard_candidates.push_back(candidate);
      next.push_back(std::move(candidate));
    }
    }
    std::sort(next.begin(), next.end(), [](const Route& a, const Route& b) { return a.value > b.value; });
    if (next.size() > 8) next.resize(8);
    beam = std::move(next);
    // Try at most two extra sprint steps only if no generated ordinary route
    // has an observed continuation. This is a bounded heuristic, not a death proof.
    if(rescue_search && depth+1==original_limit && free_future<rescue_threshold)
      max_steps=std::min(12,original_limit+2);
    if (beam.empty()) break;
  }
  const char* guard_reason=nullptr;
  if(exact_guard&&!guard_candidates.empty()) {
    std::stable_sort(guard_candidates.begin(),guard_candidates.end(),[](const Route&a,const Route&b){return a.value>b.value;});
    int budget=3000,expanded=0,checked=0,rejected=0;auto status=queen_survival::Result::Dead;
    bool have_unknown=false;Route unknown_choice;
    for(const auto&candidate:guard_candidates){
      ++checked;auto verdict=QueenFuture(candidate.body,candidate.eaten,budget,expanded);
      if(verdict==queen_survival::Result::Dead){++rejected;continue;}
      if(verdict==queen_survival::Result::Alive){best=candidate;status=verdict;break;}
      if(!have_unknown){unknown_choice=candidate;have_unknown=true;}
      if(budget<=0)break;
    }
    if(status!=queen_survival::Result::Alive&&have_unknown){best=unknown_choice;status=queen_survival::Result::Unknown;}
    const char* tag=status==queen_survival::Result::Alive?"alive":status==queen_survival::Result::Unknown?"unknown":"emergency";
    queen_guard_reason_=std::string("queen8: harvest status=")+tag+" checked="+std::to_string(checked)+" rejected="+std::to_string(rejected)+" expanded="+std::to_string(expanded);
    guard_reason=queen_guard_reason_.c_str();
    // If every generated route fails, retain the last-resort legal move rather
    // than interpreting bounded forecast failure as permission to resign.
  }
  if (!best.steps.empty()) {
    if(RING11_BRANCH_MODE>=2 && closed_branch[best.steps.front()] && length_>=4 && FreeSlots()>0)
      return Because("queen11: wait at closed corridor",bot::Action::Split(2));
    // The base remembers one head per turn. Store every sprint step so later self checks stay correct.
    body_.assign(best.body.begin(), best.body.end());
    bot::Action a;
    a.steps = best.steps;
    return Because(early_roaming?"queen15: early harvest":guard_reason?guard_reason:(rescue_search && int(best.steps.size())>original_limit ? "queen11: rescue sprint" : (round_ < kPantryFrom ? "queen: safe habitat" : "queen: harvest")), a);
  }
  // Attempt an unseen endpoint only when there is no currently observed legal move.
  if((RING6_MODE==1||RING6_MODE>=3)&&round_>=430){
    for(int d=0;d<4;++d){int t=board_.Step(head_,d);
      // A tile outside current vision can still be occupied by our known body.
      // Do not erase this evidence and turn a legal split fallback into a self-hit.
      if(t>=0 && std::find(body_.begin(),body_.end(),t)!=body_.end())continue;
      if(t==kUnknown || (t>=0 && !board_.Current(t))) {
        // A legal split buys a turn without risking an unobserved portal landing.
        // Preserve the existing split selection below, including its legality checks.
        if(length_>=4 && FreeSlots()>0)continue;
        body_.clear();ring_.clear();queen_growing_=true;
        return Because("ring6: last portal escape",bot::Action::Move(d));
      }
    }
  }
  // If already boxed in, preserve at least four queen segments when an escape split exists.
  if (length_ >= 6 && FreeSlots() > 0 && static_cast<int>(body_.size()) == length_)
    for (int n = 2; n <= length_ - 4; ++n) if (SplitSafe(n))
      return Because("queen: escape split", bot::Action::Split(n));
  // At any round, a legal split keeps a boxed queen alive this turn. Do not require
  // the child to survive when the alternative is immediate queen death.
  if(length_>=4 && FreeSlots()>0) {
    // Split legality depends only on length and capacity, not observed tail coordinates.
    int n=2;
    if(RING10_SPLIT_MODE==2)n=std::max(2,length_/2);
    if(RING10_SPLIT_MODE==3)n=length_-2;
    return Because("queen10: boxed legal split",bot::Action::Split(n));
  }
  return Because("queen: trapped", bot::Action::Resign());
}

// ---- the turn --------------------------------------------------------------------------------------

bot::Action StrategyAgent::Turn(const rl::Observation& o) {
  rescue_committed_ = false;
  walking_to_ally_ = false;   // (gst-search-4)
#ifdef GM2_DEBUG
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
  if (!IsQueen()) {
    const auto observed_body = body_;
    if (QUEEN_CLEARANCE_MODE || QUEEN_GUARD) a = RescueQueen(a, o);
    const auto rescue_body = body_;
    body_ = observed_body;
    a = RingWorker(a, o);
    if (!ring_worker_override_) body_ = rescue_body;
  }
  if(!IsQueen()) a=Supply14Yield(a);
  // tell the dragons around us who the champion is (a dead dragon's pings are not cast)
  const Report champ = Champion();
  if (champ.id >= 0)
    for (int d = 0; d < 4; ++d) a.sonar[d] = Encode(champ);
  if (obit_until_ > round_ && (round_ & 1))
    for (int d = 0; d < 4; ++d) a.sonar[d] = Encode(obit_);
  if (claim_tile_ >= 0 && a.kind == bot::Action::kMove && !a.resign && !a.steps.empty()) {
    // the claim goes out on both sides across our heading (allies in the corridor lie ahead or behind)
    const uint64_t w = (kClaimMagic << 56) | (uint64_t(spawn_.team & 1) << 55) | (uint64_t(spawn_.id & 4095) << 43) |
                       (uint64_t(board_.X(claim_tile_) & 63) << 37) | (uint64_t(board_.Y(claim_tile_) & 63) << 31) |
                       (uint64_t(round_ & 511) << 10);
    const int f = a.steps.back();
    a.sonar[(f + 2) % 4] = w;
    if (round_ & 1) a.sonar[f] = w;
  }
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
#ifdef GM2_TRACE
  if (a.kind == bot::Action::kSplit)
    std::fprintf(stderr, "SPLITDBG known%d heard%d %s\n", int(static_cast<int32_t>(body_.size()) >= length_), int(HeardSplit(a.split)), reason_);
  if (round_ == 450 && spawn_.team == 0) std::fprintf(stderr, "C450 id%d len%d champ%d\n", spawn_.id, length_, champ.id);
  if (round_ == 495 && spawn_.team == 0) {
    const int dx = std::abs(board_.X(champ.tile) - board_.X(head_)), dy = std::abs(board_.Y(champ.tile) - board_.Y(head_));
    std::fprintf(stderr, "END id%d len%d leader%d champ%d clen%d cage%d cdist%d start%d reason=%s\n", spawn_.id, length_, int(leader_), champ.id, champ.length, round_ - champ.round,
                 std::min(dx, board_.width() - dx) + std::min(dy, board_.height() - dy), FeedStart(champ), reason_);
  }
#endif
  // stepping next to an unknown portal: ping only through it, so next turn's echoes say
  // whether a dragon lies past its exit
  bool pinging = false;
  // (gst115) stepping back onto the tile beside a known portal whose blind step we refused: ping through it
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
#ifdef GM2_DEBUG
  ++g_turns;
  if (Looping()) ++g_idle;
  if (length_ > 0 && round_ == last_eat_ && round_ > born_) ++g_eaten;
  if (std::string(reason_) == "portal: explore") ++g_cross;
  if (a.kind == bot::Action::kMove && !a.resign && !a.steps.empty() && (portal_[head_] >> a.steps[0] & 1)) ++g_kcross;
#endif
  if (!ring_.empty() && round_ - ring_seen_ <= 12)
    for (int d = 0; d < 4; ++d) a.sonar[d] = RingWord();
  if (QUEEN_ENDGAME >= 2 && IsQueen() && round_ >= kPantryFrom)
    for (int d = 0; d < 4; ++d) a.sonar[d] = DeliveryWord();
  return a;
}

bot::Action StrategyAgent::Decide(const rl::Observation& o) {
  work_ = 0;
  Remember(o);
  ring4_observed_body_.assign(body_.begin(),body_.end());
  if(RING5_MODE==3)ReadRing5(o);
  UpdateHot(o);
  if (QUEEN_ENDGAME >= 2 && !IsQueen()) ReadDelivery(o);
  claim_tile_ = -1;   // (gm2) set again below only when this turn heads for a hot bed
  for (int q = 0; q < 2; ++q) {
    const auto s = seen_.find(q);
    if (s != seen_.end()) queen_id_ = s->second.team == spawn_.team ? q : 1 - q;
    if (reports_.count(q)) queen_id_ = q;
  }
  if (IsQueen()) return QueenMove();
  if(SUPPLY14_MODE==3 && round_>=450 && round_<499) {
    // No omniscient alive bit. Require a recent positive report or sighting.
    bool known_alive=false;
    const auto q=seen_.find(queen_id_);if(q!=seen_.end() && q->second.team==spawn_.team && q->second.length>=2 && round_-q->second.round<=24)known_alive=true;
    const auto report=reports_.find(queen_id_);if(report!=reports_.end() && report->second.length>=2 && round_-report->second.round<=24)known_alive=true;
    if(q!=seen_.end() && q->second.length==0)known_alive=false;
    if(report!=reports_.end() && report->second.length==0)known_alive=false;
    if(known_alive){rescue_committed_=true;return Because("feed14: mass sacrifice",bot::Action::Resign());}
  }
  if (RING4_STAGE >= 2) {auto supply=Ring4Supply();if(supply.resign)return supply;}
  if (QUEEN_ENDGAME >= 2 && round_ >= kPantryFrom) {
    auto delivery = CourierMove();
    if (delivery.resign || !delivery.steps.empty()) return delivery;
  }
  p_.feed_from_round = p_.leader_feed_from = kPantryFrom;
  if (QUEEN_FEED_MODE == 2 && round_ >= 450)
    return Because("pantry: mass sacrifice", bot::Action::Resign());

  // 1. feeding: late in the game every dragon but the champion walks to it and dies next to
  // it, leaving it pearls (a leader only near the end, and only when it knows where it is)
  const Report champ = Champion();
  // ms6 parking: from the feed round the champion stays near the tile it held then (anchor_), so
  // its reported position stays true and the feeders, who walk no faster than it, can reach it
  if (kPark > 0 && champ.id == spawn_.id && round_ >= p_.feed_from_round) {
    if (anchor_ < 0) anchor_ = head_;
  } else {
    anchor_ = -1;
  }
  const bool queen_target = champ.id >= 0 && champ.id <= 1;
  const bool feeding = champ.id >= 0 && champ.id != spawn_.id &&
                       (!queen_target) &&
                       (queen_target || champ.length >= length_) && length_ >= kFeedMin && length_ < kFeedMaxLen &&
                       (leader_ ? round_ >= p_.leader_feed_from && round_ - champ.round <= p_.leader_feed_fresh
                                : round_ >= FeedStart(champ));
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
      if (!queen_target && !merge_ready && length_ * 10 >= tl * p_.feed_clear_tenths) {
        ally_head = -1;                              // hold: keep gathering instead
        in_view = false;
        held = true;
      }
    } else if (kSeekBody) {
      // ms6 delivery: its head is out of view. Walk to the nearest piece of its body we see; else
      // to where it was last reported, even when that tile is in view and empty (it moved, but
      // not far: the report is fresh and relayed every round)
      const int32_t hx = board_.X(head_), hy = board_.Y(head_);
      int32_t best_d = 1 << 20;
      for (int dy = -3; dy <= 3; ++dy)
        for (int dx = -3; dx <= 3; ++dx) {
          const int32_t t = board_.Tile(hx + dx, hy + dy);
          if (board_.Current(t) && board_.at(t).occupant == champ.id && std::abs(dx) + std::abs(dy) < best_d) {
            best_d = std::abs(dx) + std::abs(dy);
            ally_head = t;
          }
        }
      if (ally_head < 0 && champ.tile != head_) ally_head = champ.tile;
    } else if (champ.tile != head_ && !board_.Current(champ.tile)) {
      ally_head = champ.tile;                        // where it was last reported (out of view)
    }
  }
  const Paths paths = Search(board_, head_, [&](int32_t t) {
    return (RING5_MODE!=3||!Ring5Reserved(t))&&(!Occupied(t) || Classify(t) == Cell::kEnemyHead || t == ally_head);
  });
  // ms6 delivery: die beside any part of the champion's body, not only its head (a long champion
  // that keeps moving is much easier to meet along its body; it comes back for the pearls, which
  // the reserved ground keeps for it)
  bool at_body = false;
  if (in_view && kBodyReach > 0) {
    const int32_t hx = board_.X(head_), hy = board_.Y(head_);
    for (int dy = -kBodyReach; dy <= kBodyReach && !at_body; ++dy)
      for (int dx = -kBodyReach + std::abs(dy); dx <= kBodyReach - std::abs(dy) && !at_body; ++dx) {
        const int32_t t = board_.Tile(hx + dx, hy + dy);
        at_body = board_.Current(t) && board_.at(t).occupant == champ.id;
      }
  }
  if (queen_target ? (false && in_view && FeedAhead(champ)) :
      (in_view && paths.dist[ally_head] == 1))   // g39-5: directly next to its head only
  {
    // ms5 "chop feed": a death leaves a pearl on every second segment (head first), so a body of
    // length L feeds ceil(L/2). Cut a long feeder into 3-long pieces first (each dies for 2
    // pearls, 67%) instead of dying whole (50% for even L): the tail piece is a new dragon beside
    // the champion that feeds on its next turn, and we come back to this line next round.
    if (kChopFeed && length_ >= kChopMin && FreeSlots() >= 1)
      return Because("feed: chop", bot::Action::Split(3));
    return Because("feed champion", bot::Action::Resign());
  }

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
      !(round_ >= p_.champ_freeze && champ.id == spawn_.id) &&
      heads_near < p_.split_crowd_limit &&
      !EarlyHold() &&
      // g38-4 cap-aware growth: within 4 of UNIT_LIMIT a worker of length 4+ keeps its length
      // (grows toward a champion) instead of filling the last slots with short stubs
      !(units_ >= spawn_.unit_limit - kCapGrowMargin && length_ >= 4) &&
      // g39-5: once the team has been at the cap from r250, workers grow in parallel to 15
      !(cap_latched_ && !leader_ && length_ < kParallelGrowLen) &&
      GrowthSplit() >= 0)
    return Because("split", bot::Action::Split(GrowthSplit()));

  // 5. a goal, and the first safe step towards it
  poor_ = false;
  const Goal target = ally_head >= 0 && paths.dist[ally_head] > 0 ? Goal{ally_head, false} : ChooseGoal(paths);
  walking_to_ally_ = ally_head >= 0 && paths.dist[ally_head] > 0;   // (gst-search-4)
  const int32_t goal = target.tile;
  // gst-merge-1 (from gst-hunter-1): a short farmer heading for a hot bed claims it over sonar
  claim_tile_ = -1;
  if (p_.hot_claim && goal >= 0 && hot_[goal] && length_ <= p_.hot_claim_max_len && round_ < p_.feed_from_round &&
      paths.dist[goal] >= 0 && paths.dist[goal] <= p_.hot_claim_dist) {
    claim_tile_ = goal;
    claims_[goal] = {spawn_.id, round_};
  }
  // 4b. body-wall-21 (ported via gst70): free a slot at the cap. Every trapped death of length >= 4
  // happens at the unit cap with no split possible (league/KNOWLEDGE.md): an idle 2-3-long stub near
  // a long ally or a crowd of heads gives up its slot so a long dragon can escape-split. UNIT_COUNT
  // is live within a round, so stubs acting after one resigned see the freed slot (throttle).
  if (round_ < kGiveUpUntil && length_ >= 2 && length_ <= kGiveUpMaxLen && !leader_ && FreeSlots() <= kGiveUpFree && units_ > kGiveUpMinUnits &&
      champ.id != spawn_.id && ally_head < 0 && (goal < 0 || paths.dist[goal] > kGiveUpIdleDist)) {
    bool long_ally = false;
    int heads = 0;
    const int hx = board_.X(head_), hy = board_.Y(head_);
    for (const auto& [id, w] : seen_) {
      if (w.round != round_ || w.head < 0 || id == spawn_.id) continue;
      const int d = std::abs(board_.X(w.head) - hx) + std::abs(board_.Y(w.head) - hy);
      ++heads;
      if (w.team == spawn_.team && d <= 1) {   // g39-5: directly next to its head (was kGiveUpAllyDist)
        int32_t len = w.length;
        if (const auto r = reports_.find(id); r != reports_.end()) len = std::max(len, r->second.length);
        long_ally |= len >= kGiveUpAllyLen;
      }
    }
    (void)heads;   // g39-5: no crowd give-up (nobody beside us eats the pearls)
    if (long_ally) return Because("give up slot: long ally", bot::Action::Resign());
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
#ifdef GM2_DEBUG
    ++g_bans;
#endif
    recent_.clear();
  }
  last_goal_ = goal;
  // exploring beside an unknown portal whose exit our ping found clear: step through it
  if (WillCross() && exit_clear_ && ping_tile_ == head_ && !target.trap && length_ >= 2 && UnknownPortal(head_, ping_dir_) &&
      BodyInView() && (goal < 0 || (poor_ && p_.poor_cross > 0 && paths.dist[goal] >= p_.poor_cross) || [&] {
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

}  // namespace gm2

namespace gm2 {
// ms6 delivery: a worker far from the champion leaves early enough to arrive before the end.
int32_t StrategyAgent::FeedStart(const Report& champ) const {
  if (kWalkRate <= 0 || champ.tile < 0) return p_.feed_from_round;
  const int32_t dx = std::abs(board_.X(champ.tile) - board_.X(head_)), dy = std::abs(board_.Y(champ.tile) - board_.Y(head_));
  const int32_t dist = std::min(dx, board_.width() - dx) + std::min(dy, board_.height() - dy);
  return std::min(p_.feed_from_round, kWalkEnd - kWalkRate * dist);
}
}  // namespace gm2

namespace gm2 {
// gst-merge-1, from gst-free-1 (free team; strategist w P3): before kEarlyHoldRound a dragon of
// length >= kEarlyHoldLength does not split while an enemy head is within kEarlyHoldRadius (torus
// Manhattan): the long opener is not shattered into 2-long pieces inside a contested lane.
bool StrategyAgent::EarlyHold() const {
  if (round_ >= kEarlyHoldRound || length_ < kEarlyHoldLength) return false;
  for (const auto& [id, w] : seen_) {
    if (w.team == spawn_.team || w.round != round_ || w.head < 0) continue;
    const int32_t dx = std::abs(board_.X(w.head) - board_.X(head_)), dy = std::abs(board_.Y(w.head) - board_.Y(head_));
    if (std::min(dx, board_.width() - dx) + std::min(dy, board_.height() - dy) <= kEarlyHoldRadius) return true;
  }
  return false;
}
}  // namespace gm2
