#include "strategy.hpp"


#include <algorithm>
#include <limits>
#include <cmath>
#include <tuple>
#include <functional>
#include <unordered_map>
#include <unordered_set>
#if defined(GM2_DEBUG) || defined(GM2_TRACE)
#include <atomic>
#include <cstdlib>
#endif
#include <cstring>

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
// g40-3 (ring-lateconc-1): late concentration. Observed cause (arena-r14 #3): in longest-dragon
// losses our longest/total is 0.14 v 0.27, because no-burn keeps the material spread over many
// mid-length workers to the end. From kLateConcFrom, when our queen is not known alive (the
// longest-dragon tiebreak only matters once queen lengths tie, i.e. ours is dead), every non-queen
// dragon agrees on one champion (the longest non-queen ally known via sonar, ties by lower id; it
// stays champion until it dies) and feeders (kFeedMin..kFeedMaxLen-1) resign within kBodyReach of
// ANY segment of it, not only beside its head. kLateConcFrom = 430: after every growth-split
// cutoff (<= r400), late enough that resigned material can't be regrown elsewhere (feeding from
// r300 starved the team in g38-7), early enough that feeders within ~15 tiles arrive by r480.
// The queen counts as alive while a report of her (length > 0) is at most kQueenAliveFresh rounds
// old: the same 80-round window in which Champion() keeps her as the team's champion, so nothing
// changes while the team still treats her as alive (a 30-round window lost queen games in a duel,
// 14-21 decided by the longer queen, queen alive at the end 54% v 65%: groups that hadn't heard her
// for 30 rounds stopped relaying her and stopped reserving her pearls). A dead queen is reported
// dead (length 0) by the dragons that saw her die.
static constexpr int32_t kLateConcFrom = 430, kQueenAliveFresh = 80;
// Iteration 2, ported from gst-giant-2 (ends games with one dragon of 37-90; its giant grows by
// its own dragons dying next to its HEAD from r380): in late concentration a feeder of ANY length
// (still held while it is 80% or more of the champion) resigns within kLateFeedDistance path steps
// of the champion's head, gst-giant-2's feed_distance. Iteration 1 let feeders die beside any
// segment: total fell 19% in queen-dead games and the longest gained 0.2, because the champion
// rarely came back along its body for the pearls.
static constexpr int32_t kLateFeedDistance = 3;
// g43-7 v2: end tiles a lunge escape run may evaluate (room flood + escape level) in one turn
static constexpr int kLungeRunEvals = 16;
#ifdef LUNGE_DBG
std::atomic<long> g_lunge_goal_veto{0}, g_lf_enemy{0}, g_lf_worth{0}, g_lf_partial{0}, g_lf_middle{0};
#endif
#ifdef GM2_DEBUG
static long g_escape = 0, g_turnout = 0, g_calls = 0;
#define GM2_COUNT(x) ++(x)
// idle-loop measurement: turns where the last 12 heads cover <= 6 tiles with nothing eaten
// g15-6 exposure probe: goal steps refused for being next to an enemy head, and what that head did
static std::atomic<long> g_exp{0}, g_exp_pearl{0}, g_exp_hit{0}, g_exp_away{0}, g_exp_unseen{0}, g_exp_hit_p{0}, g_exp_away_p{0};
static std::atomic<long> g_bed_early{0}, g_bed_wait{0}, g_bed_skip{0};   // g40-6: steps onto a bed whose tick our body would block; steps aside
static std::atomic<long> g_turns{0}, g_idle{0}, g_eaten{0}, g_cross{0}, g_bans{0}, g_kcross{0}, g_pdetour{0}, g_lifted{0};
static struct IdleReport {
  ~IdleReport() {
    if (g_turns) std::fprintf(stderr, "GST51 turns %ld idle %ld (%.2f%%) eaten %ld crossings %ld bans %ld portal-steps %ld portal-detours %ld lifted %ld\n", g_turns.load(),
                              g_idle.load(), 100.0 * g_idle / g_turns, g_eaten.load(), g_cross.load(), g_bans.load(),
                              g_kcross.load(), g_pdetour.load(), g_lifted.load());
    if (g_turns) std::fprintf(stderr, "BEDTIME early-steps %ld waits %ld step-arounds %ld\n", g_bed_early.load(), g_bed_wait.load(), g_bed_skip.load());
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
  // g43-5: after a multi-step move the old head is not next to the new one: rebuild the body's
  // front from the segments in view (head to tail) up to the old head, so the tiles stepped
  // through are remembered as ours (the base remembers one head per turn)
  if (SPRINT_MODE && !body_.empty() && body_.front() != head_) {
    bool adjacent = false;
    for (int d = 0; d < 4; ++d) adjacent |= board_.Step(head_, d) == body_.front();
    if (!adjacent) {
      std::vector<int32_t> run;
      for (const rl::Segment& s : o.segments) {
        if (s.id != spawn_.id) continue;
        const int32_t t = board_.Tile(s.x, s.y);
        if (run.empty()) { if (t != head_) break; run.push_back(t); continue; }
        bool next = false;
        for (int d = 0; d < 4; ++d) next |= board_.Step(run.back(), d) == t;
        if (!next) break;
        run.push_back(t);
        if (t == body_.front()) break;
      }
      if (run.size() >= 2 && run.back() == body_.front()) {
        body_.pop_front();
        for (auto it = run.rbegin(); it != run.rend(); ++it) body_.push_front(*it);
      }
    }
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
    // g42-4: likewise the enemy queen, seen beside our head last turn and wholly gone now, is dead
    if (plain)
      for (int32_t q = 0; q < 2; ++q)
        if (const auto it = seen_.find(q); it != seen_.end() && it->second.team != spawn_.team && it->second.round == round_ - 1 &&
            it->second.head >= 0 && near(it->second.head, head_, 1) && !segments.count(q))
          enemy_queen_dead_ = round_;
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
  // g39-4: in the middle early, a 1-for-1 trade pays while our heads nearby outnumber theirs
  if (InMiddle()) {
    int32_t ours, theirs;
    HeadsNear(head_, 4, &ours, &theirs);
    if (ours > theirs) return true;
  }
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

// g43-7 (lungeguard): server-r17 ring-contest-1 losses: our 8+ dragons died to enemy heads 3.97
// times a loss, 3.72 of them to stubs of length 5 or less that were in view in 90 of 108 cases.
// Exposed() only looks one step from an enemy head; a stub reaches further with paid steps. An
// enemy of length Le moves ceil(Le/4) free steps plus one paid step per segment above 2, so it
// can put its head on any tile within ceil(Le/4)+Le-2 free steps (the rules' cost, not a tuned
// value). Only heads seen this turn, only free tiles in this turn's view, and only enemies we
// won't trade with. 6+ is the length the brief's evidence names (619 of 989 deaths of 6+ dragons
// at r200+ were to killers of length 3 or less); shorter workers keep today's rule.
void StrategyAgent::BuildLungeField() const {
  lunge_round_ = round_;
  lunge_mark_.Begin(board_.size());
  lunge_end_mark_.Begin(board_.size());
  if (IsQueen() || length_ < 6) return;
  for (const auto& [id, w] : seen_) {
    if (w.team == spawn_.team || w.round != round_ || w.head < 0) continue;
    // a head whose body touches the view edge is at least one segment longer than what we see
    // (its seen length is a lower bound): count that segment
    const int32_t le = std::max<int32_t>(2, w.length + (w.partial ? 1 : 0));
    const int32_t reach = (le + 3) / 4 + le - 2;
    // g45-1 v2: the end of a multi-step move of ours (escape run, sprint) must be clear of the reach
    // of the enemy's TRUE length. A body that leaves our view has unknown length, so its reach to
    // that end tile is unbounded within view. In ring-sprint-10's arena games 74 of 169 deaths of our
    // 6+ workers right after our own multi-step move were to an enemy head in view whose body left
    // the view: every one ended inside the reach of its true length, none inside the seen one.
    const int32_t end_reach = w.partial ? std::numeric_limits<int32_t>::max() : reach;
#ifdef LUNGE_DBG
    ++g_lf_enemy;
    if (w.partial) ++g_lf_partial;
    if (WorthTrading(id)) { ++g_lf_worth; if (InMiddle()) ++g_lf_middle; }
#endif
    if (WorthTrading(id)) continue;
    lunge_dist_.Begin(board_.size());
    lunge_queue_.clear();
    lunge_queue_.push_back(w.head);
    lunge_dist_.Set(w.head, 0);
    for (size_t qi = 0; qi < lunge_queue_.size(); ++qi) {
      const int32_t t = lunge_queue_[qi], dt = lunge_dist_.Get(t);
      if (dt >= end_reach) continue;
      for (int d = 0; d < 4; ++d) {
        const int32_t n = board_.Step(t, d);
        if (n < 0 || lunge_dist_.Has(n) || !board_.Current(n) || Classify(n) != Cell::kFree) continue;
        lunge_dist_.Set(n, dt + 1);
        lunge_end_mark_.Set(n);
        if (dt + 1 <= reach) lunge_mark_.Set(n);
        lunge_queue_.push_back(n);
      }
    }
  }
}

// g43-7 v2 (the escape run of gst-search-5): ring-contest-5's arena games against its parent showed
// 123 of 383 deaths of our 6+ workers to enemy heads (server suite) came after a step with no clear
// single step left while a free run (2..ceil(L/4) steps, no segment paid) reached a clear tile. A
// run moves the head several tiles before any enemy acts, so only its last tile must be clear;
// the tiles on the way must be free. Shortest run first (it leaves the most body behind
// unchanged), then the most room.
std::vector<int> StrategyAgent::LungeEscapeRun(const std::function<bool(int32_t)>& open, int32_t need) const {
  std::vector<int> best;
  if (IsQueen() || length_ < 6) return best;
  const int free_steps = (length_ + 3) / 4;
  if (free_steps < 2) return best;
  const int32_t cap = EscapeCap(length_);
  int32_t best_room = -1;
  int evals = 0;
  std::vector<int> path;
  std::vector<int32_t> tiles;
  std::function<void(int32_t)> dfs = [&](int32_t at) {
    const int k = static_cast<int>(path.size());
    if (k >= free_steps || evals >= kLungeRunEvals) return;
    for (int d = 0; d < 4; ++d) {
      const int32_t n = board_.Step(at, d);
      if (n < 0 || !board_.Current(n) || Classify(n) != Cell::kFree || (avoid_tile_ >= 0 && n == avoid_tile_)) continue;
      if (std::find(tiles.begin(), tiles.end(), n) != tiles.end()) continue;
      // g45-1 v3: each tile is entered only by its shortest runs (the run prefers the shortest anyway).
      // With end tiles held to the unknown-length reach, few ends pass and the evaluation cap no longer
      // stopped the search: every self-avoiding walk of up to ceil(L/4) steps in view was tried (a
      // 94M-point turn in ring-sprint-15's arena games, v 52M for ring-sprint-10)
      if (lunge_dist_.Has(n) && lunge_dist_.Get(n) < k + 1) continue;
      lunge_dist_.Set(n, k + 1);
      path.push_back(d);
      tiles.push_back(n);
      if (k + 1 >= 2 && !Exposed(n) && !LungeEndExposed(n) && (best.empty() || static_cast<int>(path.size()) <= static_cast<int>(best.size()))) {
        ++evals;
        const int32_t room = RoomAround(board_, n, open, need);
        if (room >= need && (best.empty() || path.size() < best.size() || room > best_room)) {
          // the body after the run: the run's tiles (newest first), then the old body less as many tail tiles
          std::vector<int32_t> body(tiles.rbegin(), tiles.rend());
          for (int32_t t : body_) {
            if (static_cast<int32_t>(body.size()) >= length_) break;
            body.push_back(t);
          }
          if (EscapeLevel(body, cap) >= 1) {
            best = path;
            best_room = room;
          }
        }
      }
      if (best.empty() || static_cast<int>(path.size()) < static_cast<int>(best.size())) dfs(n);
      path.pop_back();
      tiles.pop_back();
    }
  };
  lunge_dist_.Begin(board_.size());   // free again: the lunge field is built (this turn's BFS is done)
  dfs(head_);
  return best;
}

bool StrategyAgent::LungeAvoid(int d) const {
  if (IsQueen() || length_ < 6 || d < 0 || d > 3) return false;
  if (lunge_round_ != round_) BuildLungeField();
  if (!LungeExposed(board_.Step(head_, d))) return false;
  for (int e = 0; e < 4; ++e) {
    const int32_t to = board_.Step(head_, e);
    if (Classify(to) == Cell::kFree && !(avoid_tile_ >= 0 && to == avoid_tile_) && !Exposed(to) && !LungeExposed(to)) return true;
  }
  return false;
}

bool StrategyAgent::Exposed(int32_t t) const {
  if (!leader_ && !p_.avoid_heads) return false;
  for (int d = 0; d < 4; ++d)
    if (const int32_t n = board_.Step(t, d); n >= 0 && Classify(n) == Cell::kEnemyHead) {
      // g39-4: in the middle early, at local head parity a tile in enemy reach is still ours to take
      if (InMiddle()) {
        int32_t ours, theirs;
        HeadsNear(head_, 4, &ours, &theirs);
        if (ours >= theirs) continue;
      }
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
         (uint64_t(std::min(r.length, 4095)) << 19) | (uint64_t(r.round & 511) << 10) | (uint64_t(sym_.Usable()) << 1) | uint64_t(r.lower ? 1 : 0) | (uint64_t(EnemyQueenField(r.round)) << 4);
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
  if ((w >> 56) != kMagic || int32_t((w >> 55) & 1) != (spawn_.team & 1) || (w & 512) != 0) return;
  sym_.Hint(uint8_t((w >> 1) & 7));   // g39-sym: bits 1-3 carry the sender's usable symmetry candidates
  Report r;
  r.id = int32_t((w >> 43) & 4095);
  const int32_t x = int32_t((w >> 37) & 63), y = int32_t((w >> 31) & 63);
  if (x >= board_.width() || y >= board_.height()) return;
  r.tile = board_.Tile(x, y);
  r.length = int32_t((w >> 19) & 4095);
  r.round = int32_t((w >> 10) & 511);
  r.lower = (w & 1) != 0;
  if (r.round > round_) return;
  // g40-4: bits 4-8: 0 unknown, else 1 + rounds between the enemy queen's last sighting and r.round
  if (const int32_t f = int32_t((w >> 4) & 31); f > 0) enemy_queen_seen_ = std::max(enemy_queen_seen_, r.round - (f - 1));
  if (r.id == spawn_.id) return;
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
  if (die_near_ && late_champ_ >= 0)
    if (const auto it = reports_.find(late_champ_); it != reports_.end()) return it->second;
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
    // late-surge-17: waiting is charged double (dist steps are free, then waiting rounds cost wait_factor each)
    // g40-6: arrival counted against the exact spawn tick (BedReady)
    *ready = BedReady(dist, due, p_.wait_factor);
    return 1.0;
  }
  if (age == 0) return 1.0;                                // due now (blocked while occupied)
  // g40-6: it grew its pearl exactly -due rounds ago (the tick is certain once seen); from then on
  // it is a pearl unseen for that long, with the same per-round chance of still being there
  return BEDTIME_MODE ? std::pow(p_.pearl_decay, -due) : p_.ripe_bed_value;
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
  // g42-6 (ring-contest-1): the opening food contest. arena-r16 cluster A: in r30-59 we ate 6
  // middle pearls to their 28, and they ate 14 pearls on our side to our 2.4 on theirs; deaths
  // were level, so the food was ceded, not fought for. Before kContestUntil a worker (never the
  // queen) counts the enemy heads it saw in the last kRivalMemory rounds (RivalDist: where it was
  // seen, up to kRivalDrift steps nearer). Food we reach no later than every
  // such head, while one of them could reach it within kSchedHorizon rounds (it is coming for
  // it), is worth kContestDeny times as much: taking it is one pearl for us AND one pearl the
  // enemy won't eat, a two-pearl swing in a length race.
  std::vector<std::pair<int32_t, int32_t>> rivals;   // (head tile, rounds since seen)
  if (CONTEST_MODE && round_ < kContestUntil && !IsQueen())
    for (const auto& [id, w] : seen_)
      if (w.team != spawn_.team && w.head >= 0 && round_ - w.round <= kRivalMemory) rivals.push_back({w.head, round_ - w.round});
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
  // g42-3 (ring-champfood-1): in late concentration, before kLastFeedFrom, a worker leaves to the
  // champion every food tile the champion reaches no later than we do (its speed: free sprint steps
  // over tiles in view, min(ceil(L/4), kChampSight) a turn), and that it reaches before the end
  if (late_conc_ && round_ < kLastFeedFrom && !champion) {
    const Report champ = Champion();
    if (champ.id >= 0 && champ.id != spawn_.id && round_ - champ.round <= kReserveFresh) {
      int32_t c = champ.tile;
      if (const auto it = seen_.find(champ.id); it != seen_.end() && it->second.round == round_ && it->second.head >= 0)
        c = it->second.head;
      const int32_t speed = std::max(1, std::min((champ.length + 3) / 4, kChampSight));
      if (c >= 0)
        for (int32_t t = 0; t < board_.size(); ++t) {
          if (base[t] <= 0) continue;
          const int32_t arrive = (torus(c, t) + speed - 1) / speed;
          if (round_ + arrive > 499) continue;
          if (paths.dist[t] < 0 || arrive <= paths.dist[t]) base[t] = 0.0;
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
      auto ready_of = [&](int32_t t, int32_t d) { return BedReady(d, due0[t], 0.0); };   // g40-6: exact tick
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
    const int32_t ready = BedReady(d, due0[t], p_.wait_factor);   // g40-6: exact tick
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
  // g42-9: the call this worker answers (call.cpp), as a detour bonus on food goals that bring
  // us nearer the cluster: eating t and then walking on to the cluster is worth both per the
  // rounds the whole trip takes (the walk on a torus-distance lower bound, as CallGoal's eta).
  // A food goal scores the better of that and its own score, so the answerer keeps eating and
  // splitting on the way instead of walking straight there (ring-clustercall-1: r40 dragons
  // 8.7 v 10.0 on trophy/devil).
  double call_value = 0;
#ifdef CLAIMMID_DBG
  dbg_detour_.clear();
#endif
  const int32_t call_at = CallAnswered(&call_value);
  const int32_t call_here = call_at >= 0 ? Torus(head_, call_at) : 0;
  for (int32_t t = 0; t < board_.size(); ++t) {
    const int32_t d = paths.dist[t];
    if (d <= 0) continue;
    const double value0 = base[t];
    const int32_t ready = BedReady(d, due0[t], p_.wait_factor);   // g40-6: exact tick
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
          const int32_t r2 = BedReady(dn, due0[n], p_.wait_factor);
          near += v * (d + 1.0) / (r2 + 1.0);
        }
      value += p_.cluster_bonus * near;
    }
    double score = value / (ready + 1);
    // g42-6: food we reach no later than an approaching enemy head (the contest boost below)
    bool contested = false;
    if (value0 > 0 && !rivals.empty()) {
      const TileMemory& m = board_.at(t);
      const int32_t due = m.pearl ? 0 : std::max(0, m.countdown - (round_ - m.seen_round));
      int32_t theirs = 1 << 20;
      for (const auto& [e, age] : rivals) theirs = std::min(theirs, BedReady(RivalDist(torus(e, t), age), due, 0.0));
      contested = ready <= theirs && theirs <= kSchedHorizon;
    }
    // g42-m: contested food carries no call bonus: its contest boost decides it (the review of
    // -27/-29: the two otherwise fight over the same r<60 workers)
    if (call_at >= 0 && value0 > 0 && !contested) {
      const int32_t on = Torus(t, call_at);
      if (on < call_here) {
#ifdef CALL_DBG
        if ((value + call_value) / (ready + on + 1.0) > score && round_ < 30)
          std::fprintf(stderr, "DETOUR r%d team%d id%d head(%d,%d) food(%d,%d) call(%d,%d) plain%.3f bonus%.3f\n", round_, spawn_.team, spawn_.id,
                       board_.X(head_), board_.Y(head_), board_.X(t), board_.Y(t), board_.X(call_at), board_.Y(call_at), score,
                       (value + call_value) / (ready + on + 1.0));
#endif
#ifdef CLAIMMID_DBG
        if ((value + call_value) / (ready + on + 1.0) > score) dbg_detour_.push_back(t);
#endif
        score = std::max(score, (value + call_value) / (ready + on + 1.0));
      }
    }
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
        // g40-6: their arrival against the exact tick too (both waiting beside it: lower id eats)
        const int32_t theirs = BedReady(torus(e, t), due, 0.0), tick = BEDTIME_MODE ? due + 1 : due;
        if (theirs < ready || (theirs == ready && due > 0 && ready == tick && id < spawn_.id)) {
          score *= p_.contest_factor;
          break;
        }
      }
    }
    if (score > 0 && contested) score *= kContestDeny;
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
    // g39-4 (g40-5: now a contested find): an explorer holding it stays near until r60
    if (score > 0 && rush_state_ == 2 && round_ < 60 && Torus(t, rush_target_) > 8) score *= 0.5;
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
  // g42-1: a worker answers a teammate's cluster call worth more per round than the goal it
  // chose (call.cpp)
  call_goal_ = false;
  {
    const double chosen = best.tile >= 0 && best.tile == nearest_pearl ? std::max(best_score, nearest_pearl_score) : best_score;
    // g42-9: a food goal already carries the call as a detour bonus; walk straight there only
    // when our goal is not food (exploring, or nothing)
    if (best.tile < 0 || base[best.tile] <= 0)
      if (const int32_t call = CallGoal(paths, best.tile >= 0 ? chosen : 0.0); call >= 0) best = {call, false}, call_goal_ = true;
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
  // g43-7 (lungeguard): a long worker prefers steps out of every untradeable enemy's lunge reach;
  // if every step that today's rule allows is in reach, today's rule decides alone
  BuildLungeField();
  bool lunge_guard = false;
#ifndef LUNGE_OFF   // debug builds only: the base's behaviour with the same counters
  for (int d = 0; d < 4; ++d) {
    const int32_t to = board_.Step(head_, d);
    if (Classify(to) != Cell::kFree || (avoid_tile_ >= 0 && to == avoid_tile_) || Exposed(to)) continue;
    if (!LungeExposed(to)) lunge_guard = true;
  }
  if (lunge_guard) {
    lunge_guard = false;
    for (int d = 0; d < 4; ++d) if (LungeExposed(board_.Step(head_, d))) lunge_guard = true;
  }
#endif
  // g43-7 v2: no single step is clear of the lunge reach, but some step is in it: run out with free steps
  if (!lunge_guard) {
    bool threatened = false, clear = false;
    for (int d = 0; d < 4; ++d) {
      const int32_t to = board_.Step(head_, d);
      if (Classify(to) != Cell::kFree) continue;
      if (LungeExposed(to)) threatened = true;
      else if (!Exposed(to)) clear = true;
    }
    if (threatened && !clear) {
      const std::vector<int> run = LungeEscapeRun(open, need);
      if (!run.empty()) {
        bot::Action a = bot::Action::Move(run[0]);
        a.steps = run;
        return Because("lungeguard: escape run", a);
      }
    }
  }
  for (int k = 0; k < 4; ++k) {
    const int d = ((preferred < 0 ? 0 : preferred) + k) % 4;   // the preferred direction first
    const int32_t to = board_.Step(head_, d);
    if (Classify(to) != Cell::kFree || (avoid_tile_ >= 0 && to == avoid_tile_)) continue;
    const int32_t room = RoomAround(board_, to, open, need);
    const bool exposed = Exposed(to) || (lunge_guard && LungeExposed(to));
#ifdef LUNGE_DBG
    if (lunge_guard && k == 0 && preferred >= 0 && LungeExposed(to) && !Exposed(to)) ++g_lunge_goal_veto;
#endif
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
    // g40-6 v2: among steps equally safe and roomy enough, not onto a bed whose tick our body would block
    auto blocks = [&](int32_t t) {
      if (!BEDTIME_MODE || t < 0) return false;
      const TileMemory& m = board_.at(t);
      if (m.pearl || m.countdown < 0 || m.seen_round < 0) return false;
      const int32_t due = m.countdown - (round_ - m.seen_round);
      return due >= 1 && due <= length_;
    };
    auto key = [&](bool e, int lv, int lb, int c, int r, int32_t t) {
      return std::make_tuple(p_.doom_first ? lv > 0 : true, !e, lv, lb, r >= need, !blocks(t), c, r);
    };
    if (best < 0 || key(exposed, level, libs, count, room, to) > key(best_exposed, best_level, best_libs, best_count, best_room, board_.Step(head_, best))) {
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

// g43-6: whether a worker whose head is at `from` reaches fewer than kBoxRoom tiles once this
// dragon's body is `body` (our current tiles not in it are free; other dragons and kelp block;
// an unseen edge counts as open ground)
bool StrategyAgent::BoxedBy(int32_t from, const std::vector<int32_t>& body) const {
  q_body_.Begin(board_.size());
  for (const int32_t b : body) if (b >= 0) q_body_.Set(b);
  q_dist_.Begin(board_.size());
  q_queue_.assign(1, from);
  q_dist_.Set(from);
  for (size_t qi = 0; qi < q_queue_.size(); ++qi) {
    ++work_;
    for (int d = 0; d < 4; ++d) {
      const int32_t n = board_.Step(q_queue_[qi], d);
      if (n == kUnknown) return false;
      if (n < 0 || q_dist_.Has(n) || q_body_.Has(n)) continue;
      q_dist_.Set(n);
      if (board_.Current(n) && board_.at(n).occupant >= 0 && board_.at(n).occupant != spawn_.id) continue;
      q_queue_.push_back(n);
      if (static_cast<int32_t>(q_queue_.size()) >= kBoxRoom) return false;
    }
  }
  return true;
}

// g44-10: free tiles the queen reaches from t (q_body_ is the route's body, which blocks; her tiles
// outside it are free; other bodies and kelp block), up to cap. An unseen edge is worth
// kQueenUnknownSpace tiles; a portal whose far end she hasn't found is worth none.
int StrategyAgent::QueenSpace(int32_t t, int cap) const {
  qs_dist_.Begin(board_.size());
  qs_queue_.assign(1, t);
  qs_dist_.Set(t);
  int reached = 1;
  for (size_t qi = 0; qi < qs_queue_.size(); ++qi) {
    const int32_t u = qs_queue_[qi];
    ++work_;
    for (int d = 0; d < 4; ++d) {
      const int32_t n = board_.Step(u, d);
      if (n == kUnknown && (portal_[u] >> d & 1)) continue;   // an unfound portal end: no room
      if (n == kUnknown) {   // an unseen edge: some room, not open ground
        if ((reached += kQueenUnknownSpace) >= cap) return cap;
        continue;
      }
      if (n < 0 || qs_dist_.Has(n) || q_body_.Has(n)) continue;
      if (board_.Current(n) && board_.at(n).occupant >= 0 && board_.at(n).occupant != spawn_.id) continue;
      qs_dist_.Set(n);
      if (++reached >= cap) return cap;
      qs_queue_.push_back(n);
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
  if(FORAGE35_MODE && round_==0 && length_<4) {
    bool producer=false;
    for(const auto& [id,w]:seen_)
      if(id!=spawn_.id && w.team==spawn_.team && w.round==round_ && w.length>=4) producer=true;
    if(!producer && units_<kForageSettleUnits) {queen_forage_=true;queen_growing_=true;ring_.clear();}
  }
  // g49-1: a rich start (her own r0 view holds a spawn field): she forages and splits at 4 through
  // the opening, settling at kRichUntil or at max(6, 3 x the starting count) dragons. Not when an
  // enemy head is already in her view at r0: contact from the first turn is the threat her forage
  // split waits out, so she would hold at 4 unsplit while the ring's bootstrap split adds a dragon
  // (eph-0be1606b, fixed-tiny 0-4: she held at 4 among 3 enemy heads and was lunged by r7-25).
  bool enemy_at_spawn=false;
  for(const auto& [id,w]:seen_)
    if(w.team!=spawn_.team && w.round==round_ && w.head>=0) enemy_at_spawn=true;
  if(RICH_MODE && round_==0 && rich_start_ && !queen_forage_ && !enemy_at_spawn) {
    queen_forage_=true;queen_growing_=true;rich_forage_=true;ring_.clear();
    forage_settle_units_=std::max(kForageSettleUnits,kQForageGrowth*units_);
  }
  if(queen_forage_ && !queen_growing_) queen_forage_=false;
  if(!queen_forage_) rich_forage_=false;
  if(FOUNDER_MODE && round_==0 && units_<=2) small_start_=true;   // g43-6: the start size, known at r0
  const bool founder_home=FOUNDER_MODE && small_start_ && round_<kFounderUntil;   // g43-6: she stays home
  bool early_roaming=EARLY15_MODE && queen_growing_ && round_<430;
  if(queen_forage_ && early_roaming && (units_>=forage_settle_units_ || round_>=(rich_forage_ ? kRichUntil : kForageSettleRound))) {
    // Settle into the ring: split off a worker when that is safe, else just stop roaming.
    if(length_>=4 && FreeSlots()>0 && int(body_.size())==length_ && SplitSafe(length_-2)) {
      const char* why=rich_forage_ ? "rich: settle split" : "forage35: settle split";
      queen_growing_=false;queen_forage_=false;rich_forage_=false;ring_.clear();
      return Because(why,bot::Action::Split(length_-2));
    }
    queen_growing_=false;queen_forage_=false;rich_forage_=false;ring_.clear();early_roaming=false;
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
#ifdef QGATE_DBG
  if (round_ == 430) std::fprintf(stderr, "QGATE team%d race%d seen%d dead%d growing%d len%d\n", spawn_.team, int(EnemyQueenRace()), enemy_queen_seen_, enemy_queen_dead_, int(queen_growing_), length_);
#endif
  if (RING4_STAGE >= 1 && round_ >= 430 && !queen_growing_ && EnemyQueenRace()) {   // g42-4 gate
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
      // g49-1: a rich-start queen splits with food in view (RichSplit) and room (qforage-1's rule)
      !(rich_forage_ && (!RichSplit() || OpenRoom(head_,std::vector<int32_t>(body_.begin(),body_.end()),kForageRoom)<kForageRoom)) &&
      int(body_.size())==length_ && SplitSafe(length_-2))
    return Because(rich_forage_ ? "rich: queen split at 4" : EARLY15_MODE==6 ? "queen17: bootstrap and harvest" : "queen17: fast split and harvest",
                   bot::Action::Split(length_-2));
  // g43-6: on a small start, in open ground (no corridor tile, kForageRoom tiles of room), the queen
  // splits whenever she reaches 4 in the opening, child or no child (Colosseum: she sat at 4 for
  // ~20 rounds while one 2-long child lived, the base's bootstrap waiting for units <= 1)
  if(founder_home && early_roaming && length_>=4 && FreeSlots()>0 && !threat[head_] && !Corridor(head_) &&
      int(body_.size())==length_ && OpenRoom(head_,std::vector<int32_t>(body_.begin(),body_.end()),kForageRoom)>=kForageRoom &&
      SplitSafe(length_-2))
    return Because("founder: queen split at 4",bot::Action::Split(length_-2));
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
    double space_cost = 0;   // g44-10: this route's room penalty (diagnostics: the label)
    int transit_risk = 0;
    bool corridor = false;
  };
  Route root;
  root.body.assign(body_.begin(), body_.end());
  std::vector<Route> beam{root};
  Route best;
  std::vector<Route> guard_candidates;
  double best_value = -1e30;
  double raw_best_value = -1e30;   // g44-10: the best route without the room penalty (label only)
  std::vector<int> raw_best_steps;
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
  // g44-10: the queen keeps room as her space shrinks (kQueenSpacePerLength), outside the opening forage
  const bool keep_space = !(queen_forage_ && early_roaming) && !(QUEEN_ENDGAME && round_ == 499);
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
        - ((EARLY15_MODE==3 || (queen_forage_ && !(FOUNDER_HOME && founder_home))) && early_roaming ? 0.0 : (round_ < kPantryFrom ? 0.6 : 0.1)) * std::max(0, distance(t, queen_home_) - 6)
        - 0.03 * candidate.steps.size();
      if(queen_forage_ && early_roaming) {
        // The queen plays safe while foraging: no corridor tiles on the way, no low-room endpoints.
        if(Corridor(t)) candidate.corridor=true;
        if(candidate.corridor) candidate.value-=20.0;
        const int open=OpenRoom(t,candidate.body,kForageRoom);
        if(open<kForageRoom) candidate.value-=2.0*(kForageRoom-open);
      }
      // g44-10 (server-r19 S2): she leaves a shrinking space before it closes, not once boxed
      if (keep_space) {
        const int need = std::clamp(kQueenSpacePerLength * int(candidate.body.size()), kForageRoom, kQueenSpaceCap);
        const int space = QueenSpace(t, need);
        // ... and she doesn't grow in it: the share of the space she lacks is taken off what the
        // route eats and the food it heads for (kQueenSpaceCost note)
        candidate.space_cost = space < need ? kQueenSpaceCost * (need - space) +
            double(need - space) / need * (12.0 * candidate.eaten.size() + food) : 0.0;
        // a pocket she can't fit in is a sure bleed: the same weight as no escape at all
        if (space < int(candidate.body.size()) + 2) candidate.space_cost += 1000000.0;
        candidate.value -= candidate.space_cost;
#ifdef QROOM_DBG
        std::fprintf(stderr, "QS r%d team%d depth%d steps%d to%d,%d space%d need%d esc%d room%d risk%d value%.1f\n", round_, spawn_.team, depth, int(candidate.steps.size()), board_.X(t), board_.Y(t), space, need, escape, room, QueenRisk(t), candidate.value);
#endif
      }
      // g43-6 (follow-up): in the small-start opening the queen doesn't shut a worker in. Arena
      // Colosseum games: she foraged r4-r19 in the 3-wide mouth of the home pocket and the founder
      // left it only at r10-r32 (ring-contest-11: 184515, 184321; ring-contest-14: 186414). A flood
      // from a worker's head that her body (where she'd stop) cuts to under kBoxRoom tiles costs her
      // kBoxCost.
      if(FOUNDER_MODE && founder_home && early_roaming)
        for(const auto& [id,w]:seen_)
          if(id!=spawn_.id && w.team==spawn_.team && w.round==round_ && w.head>=0 &&
             Torus(t,w.head)<=kBoxReach && BoxedBy(w.head,candidate.body))
            candidate.value-=kBoxCost;
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
      if (candidate.value + candidate.space_cost > raw_best_value) { raw_best_value = candidate.value + candidate.space_cost; raw_best_steps = candidate.steps; }
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
    return Because(early_roaming?"queen15: early harvest":guard_reason?guard_reason:(rescue_search && int(best.steps.size())>original_limit ? "queen11: rescue sprint" : (keep_space && raw_best_steps != best.steps ? "queen: keep room" : round_ < kPantryFrom ? "queen: safe habitat" : "queen: harvest")), a);
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
#ifdef QROOM_DBG
  if (IsQueen()) std::fprintf(stderr, "QR r%d team%d len%d head%d,%d kind%d split%d resign%d steps%d why=%s\n", round_, spawn_.team, length_, board_.X(head_), board_.Y(head_), int(a.kind), a.split, int(a.resign), int(a.steps.size()), reason_);
#endif
#ifdef QRACE_DBG
  if (IsQueen() && round_ >= 480) std::fprintf(stderr, "QQ r%d team%d len%d kind%d split%d resign%d steps%d why=%s\n", round_, spawn_.team, length_, int(a.kind), a.split, int(a.resign), int(a.steps.size()), reason_);
#endif
  // tell the dragons around us who the champion is (a dead dragon's pings are not cast)
  const Report champ = Champion();
  if (champ.id >= 0)
    for (int d = 0; d < 4; ++d) a.sonar[d] = Encode(champ);
  if (obit_until_ > round_ && (round_ & 1))
    for (int d = 0; d < 4; ++d) a.sonar[d] = Encode(obit_);
  // g42-1: our find (on all four), or the best fresh call we heard on two opposite directions (alternating
  // each round), so the champion word keeps the other two (call.cpp)
  {
    int32_t ct = -1;
    Call cc;
    if (CallToSend(&ct, &cc)) {
      // the finder itself calls on all four (it is the only one who knows; a relay can wait)
      const int d0 = round_ & 1;
      a.sonar[d0] = a.sonar[d0 + 2] = CallWord(ct, cc);
      if (own_call_ >= 0) a.sonar[1 - d0] = a.sonar[3 - d0] = a.sonar[d0];
    }
  }
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
  if (a.kind == bot::Action::kMove && !a.resign && !a.steps.empty()) {
    const int32_t to = board_.Step(head_, a.steps[0]);
    if (to >= 0 && board_.Current(to) && !board_.at(to).pearl && board_.at(to).countdown >= 1 && board_.at(to).countdown <= length_) ++g_bed_early;
  }
  if (std::string(reason_) == "bed: wait for its tick") ++g_bed_wait;
  if (std::string(reason_) == "bed: step around its tick") ++g_bed_skip;
#endif
  if (!ring_.empty() && round_ - ring_seen_ <= 12)
    for (int d = 0; d < 4; ++d) a.sonar[d] = RingWord();
  if (QUEEN_ENDGAME >= 2 && IsQueen() && round_ >= kPantryFrom)
    for (int d = 0; d < 4; ++d) a.sonar[d] = DeliveryWord();
  // g42-1: every dragon that knows the queen's ring relays the ring word on all four directions,
  // so the call rides in its spare bits 36-55 (the ring readers use bits 0-25, qgate 26-35)
  {
    int32_t ct = -1;
    Call cc;
    if (CallToSend(&ct, &cc))
      for (int d = 0; d < 4; ++d)
        if (a.sonar[d] && (*a.sonar[d] >> 56) == 0xBA) a.sonar[d] = CallInRing(*a.sonar[d], ct, cc);
  }
  return a;
}

bot::Action StrategyAgent::Decide(const rl::Observation& o) {
  work_ = 0;
  Remember(o);
  ring4_observed_body_.assign(body_.begin(),body_.end());
  if(RING5_MODE==3)ReadRing5(o);
  UpdateHot(o);
  if (QUEEN_ENDGAME >= 2 && !IsQueen()) ReadDelivery(o);
  // g42-4: the last enemy-queen sighting relayed in our ring words (see enemy_queen_heard_)
  for (const uint64_t w : o.inbox)
    if ((w >> 56) == 0xBA && int32_t((w >> 25) & 1) == (spawn_.team & 1) && ((w >> 35) & 1) && int32_t((w >> 26) & 511) <= round_)
      enemy_queen_heard_ = std::max(enemy_queen_heard_, int32_t((w >> 26) & 511));
  // g40-4: the enemy queen (dragon 0 or 1 of the other team) in view this turn
  for (int q = 0; q < 2; ++q)
    if (const auto s = seen_.find(q); s != seen_.end() && s->second.team != spawn_.team && s->second.round == round_ && s->second.length > 0)
      enemy_queen_seen_ = round_;
  claim_tile_ = -1;   // (gm2) set again below only when this turn heads for a hot bed
  for (int q = 0; q < 2; ++q) {
    const auto s = seen_.find(q);
    if (s != seen_.end()) queen_id_ = s->second.team == spawn_.team ? q : 1 - q;
    if (reports_.count(q)) queen_id_ = q;
  }
  UpdateLateChampion();
  UpdateCall(o);   // g42-1: the early cluster call (call.cpp)
  UpdateRichStart();   // g49-1 (field.cpp)
  if (IsQueen()) return QueenMove();
  UpdateField(o);   // g46-1: the committed field-first opening (field.cpp)
  UpdateRush();   // g39-4 (g46-1: not for a dragon the field mode took)
  // g43-6: a child that finds the team at most kFounderUnits strong on its first turn (we started
  // with <= 2 dragons) is a founder (founder.cpp). A child split off in round 0 also acts in round 0
  // (leader_ is set for it too). Its tail is the segment that was cut from its parent's new tail
  // (docs/game.md, SPLIT), so on that first turn the two tails touch, which starters' don't.
  if (FOUNDER_MODE && founder_ == 0 && round_ == born_ && round_ < kFounderUntil && units_ <= kFounderUnits) {
    bool child = !leader_;
    if (!child) {
      std::unordered_map<int32_t, int32_t> tails;   // dragon id -> its last segment in view
      for (const rl::Segment& s : o.segments)
        if (s.team == spawn_.team) tails[s.id] = board_.Tile(s.x, s.y);
      const auto mine = tails.find(spawn_.id);
      if (mine != tails.end())
        for (const auto& [id, t] : tails)
          for (int d = 0; d < 4 && !child && id != spawn_.id; ++d) child = board_.Step(mine->second, d) == t;
    }
    founder_ = child ? 1 : 3;
  }
  if(SUPPLY14_MODE==3 && round_>=450 && round_<499) {
    // No omniscient alive bit. Require a recent positive report or sighting.
    bool known_alive=false;
    const auto q=seen_.find(queen_id_);if(q!=seen_.end() && q->second.team==spawn_.team && q->second.length>=2 && round_-q->second.round<=24)known_alive=true;
    const auto report=reports_.find(queen_id_);if(report!=reports_.end() && report->second.length>=2 && round_-report->second.round<=24)known_alive=true;
    if(q!=seen_.end() && q->second.length==0)known_alive=false;
    if(report!=reports_.end() && report->second.length==0)known_alive=false;
    if(known_alive){rescue_committed_=true;return Because("feed14: mass sacrifice",bot::Action::Resign());}
  }
  if (CONVEYOR_MODE) {   // g44-7: one feeder at a time (conveyor.cpp) instead of feed14's wave and QueenRace
    const bot::Action c = Conveyor();
    if (c.resign || !c.steps.empty()) return c;
  }
  if (!CONVEYOR_MODE && RING4_STAGE >= 2 && EnemyQueenRace()) {auto supply=Ring4Supply();if(supply.resign)return supply;}   // g42-4 gate
  if (QRACE_MODE && !CONVEYOR_MODE) {
    const bot::Action race = QueenRace(Champion());
    if (race.resign || !race.steps.empty() || race.kind != bot::Action::kMove) return race;
  }
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
  // g42-3: in late concentration nobody resigns for the champion before kLastFeedFrom (they forage
  // and leave it its food instead, see ChooseGoal), and the champion sprints for pearls
  // (iteration 3) a feeder far from the champion sets out early enough to arrive by kLastFeedFrom:
  // it walks one tile a round, so it leaves at kLastFeedFrom less its torus distance to the champion
  int32_t walk = 0;
  if (late_conc_ && champ.id >= 0 && champ.id != spawn_.id && champ.tile >= 0) {
    const int32_t dx = std::abs(board_.X(champ.tile) - board_.X(head_)), dy = std::abs(board_.Y(champ.tile) - board_.Y(head_));
    walk = std::min(dx, board_.width() - dx) + std::min(dy, board_.height() - dy);
  }
  // g42-8: a doomed stub (length <= kDoomedLen that has eaten nothing for p_.starve_rounds, the
  // team's existing "starving" mark) is not held back: it walks to the agreed champion and dies
  // beside its head now, where the champion eats its drop, instead of dying trapped wherever it
  // is before kLastFeedFrom (see kDoomedLen). A stub that still eats keeps foraging.
  const bool doomed_stub = die_near_ && length_ <= kDoomedLen && round_ - std::max(last_eat_, born_) >= p_.starve_rounds;
  const bool champ_food = late_conc_ && round_ < kLastFeedFrom - walk && !doomed_stub;
  const bool feeding = !champ_food && champ.id >= 0 && champ.id != spawn_.id &&
                       (!queen_target) &&
                       (queen_target || champ.length >= length_) && length_ >= kFeedMin && (length_ < kFeedMaxLen || late_conc_) &&
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
  // g43-6: the founder's way to its region (or the frontier). In the region it farms as any worker
  // and splits at 4; that split ends its dispatch (founder_ 2), so a larger region seen before then
  // still draws it on.
  int32_t founder_goal = -1;
  bool founder_here = false;
  if (FOUNDER_MODE && founder_ == 1 && field_ != 1 && round_ < kFounderUntil && ally_head < 0) founder_goal = FounderGoal(paths, &founder_here);
  const bool founding = FOUNDER_MODE && founder_ == 1 && field_ != 1 && round_ < kFounderUntil && ally_head < 0 && founder_goal >= 0;
  // no growth split on the way, except in a region, or at length kRaceSplitExempt+ (iteration 3: a
  // 6-12 long child of a long starter walked unsplit; 3.1 v 3.6 dragons at r10, 4.7 v 5.6 at r20 on
  // 26 fresh 1-2-starter maps; its split makes two travellers, as the contest's exemption says)
  const bool founder_split_ok = !founding || founder_here || length_ >= kRaceSplitExempt;
  // ms6 delivery: die beside any part of the champion's body, not only its head (a long champion
  // that keeps moving is much easier to meet along its body; it comes back for the pearls, which
  // the reserved ground keeps for it)
  bool at_body = false;
  // g40-3: a feeder in late concentration (not held: shorter than 80% of the champion, by its
  // length known this turn, or else by its reported length)
  const bool late_feed = late_conc_ && feeding && !held &&
                         (in_view || length_ * 10 < champ.length * p_.feed_clear_tenths);
  // g40-3: the champion's reported tile is often walled off in memory (its own and others'
  // remembered bodies; diagnosed: 2 of 3 late feeders had no path to it and went foraging). Walk
  // to the nearest reachable tile within kBodyReach of it instead.
  if (late_feed && !in_view && ally_head >= 0 && paths.dist[ally_head] < 0) {
    const int32_t cx = board_.X(ally_head), cy = board_.Y(ally_head);
    int32_t best = -1;
    for (int dy = -kBodyReach; dy <= kBodyReach; ++dy)
      for (int dx = -kBodyReach + std::abs(dy); dx <= kBodyReach - std::abs(dy); ++dx) {
        const int32_t t = board_.Tile(cx + dx, cy + dy);
        if (paths.dist[t] > 0 && (best < 0 || paths.dist[t] < paths.dist[best])) best = t;
      }
    if (best >= 0) ally_head = best;
  }
  if (in_view && kBodyReach > 0) {
    const int32_t hx = board_.X(head_), hy = board_.Y(head_);
    for (int dy = -kBodyReach; dy <= kBodyReach && !at_body; ++dy)
      for (int dx = -kBodyReach + std::abs(dy); dx <= kBodyReach - std::abs(dy) && !at_body; ++dx) {
        const int32_t t = board_.Tile(hx + dx, hy + dy);
        at_body = board_.Current(t) && board_.at(t).occupant == champ.id;
      }
  }
  if (queen_target ? (false && in_view && FeedAhead(champ)) :
      ((in_view && paths.dist[ally_head] == 1) ||   // g39-5: directly next to its head only ...
       // g40-3 (from gst-giant-2): ... until late concentration: within kLateFeedDistance steps of its head
       (late_feed && in_view && paths.dist[ally_head] > 0 && paths.dist[ally_head] <= kLateFeedDistance)))
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

  // g42-3: the late champion's harvest sprint
  if (late_conc_ && champ.id == spawn_.id) {
    const bot::Action h = ChampionHarvest();
    if (!h.steps.empty()) return h;
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
  int32_t split_ready = 0, split_margin = 0;
  const bool rich = RichSplit();   // g49-1
  if (length_ >= p_.split_min_length && round_ < (leader_ ? p_.leader_split_until : p_.split_until_round) &&
      FreeSlots() > std::max(p_.split_headroom, p_.reserve_slots) &&
      !(round_ >= p_.champ_freeze && champ.id == spawn_.id) &&
      heads_near < p_.split_crowd_limit &&
      // g40-5: an explorer may split once (length 4+, GrowthSplit checks the child has room)
      // g42-6: ... and again whenever dense food is in view (as often as its length allows: r0-59
      // splits were 10.5 v 23.6 in arena-r16's early collapses), but never on the way to food
      // (server-r16: on Trophy it split at r5-18 away from the cup, 18 v 88 at r100) ...
      !EarlyHold() &&
      (founder_split_ok || rich) &&   // g43-6: a founder splits in a region, not on the way (g49-1: or with rich food in view)
      // g49-1: a rich starter with food in view splits at 4 whenever GrowthSplit allows (rich)
      (rich || rush_state_ != 1 || (length_ >= 4 && (!rush_split_ || (CONTEST_MODE && DenseFoodInView())))) &&
      // ... and no worker splits while racing to food it is contesting (a split is a turn not walked):
      // while the food is beyond its view (kView 3; inside it, the dense-food rule above decides), or
      // while a rival would arrive within one round after us (a lost turn loses the food)
      // g42-12: ... except a dragon of length kRaceSplitExempt+ (a starter): its split makes two
      // racers, and holding it unsplit cost 8 v 11 dragons at r40 (fixed-maze-wide 158994)
      // g43-6: ... nor a founder that has arrived (it splits at 4 there, so its children work the region)
      !(CONTEST_MODE && round_ < kContestUntil && !founder_here && !rich && length_ < kRaceSplitExempt && contest_goal_ >= 0 && paths.dist[contest_goal_] > 0 &&
        TileValue(contest_goal_, paths.dist[contest_goal_], &split_ready) > 0 &&
        (paths.dist[contest_goal_] > 3 || (ContestWon(contest_goal_, split_ready, &split_margin) && split_margin <= 1))) &&
      // g38-4 cap-aware growth: within 4 of UNIT_LIMIT a worker of length 4+ keeps its length
      // (grows toward a champion) instead of filling the last slots with short stubs
      !(units_ >= spawn_.unit_limit - kCapGrowMargin && length_ >= 4) &&
      // g39-5: once the team has been at the cap from r250, workers grow in parallel to 15
      !(cap_latched_ && !leader_ && length_ < kParallelGrowLen) &&
      // g40-3: no growth splits during late concentration
      !late_conc_ &&
      GrowthSplit() >= 0) {
    if (rush_state_ == 1) rush_split_ = true;
    if (founder_here) {
      founder_ = 2;
      return Because("founder: split in region", bot::Action::Split(GrowthSplit()));
    }
    return Because(rich ? "rich: split" : "split", bot::Action::Split(GrowthSplit()));
  }

  // 5. a goal, and the first safe step towards it
  poor_ = false;
  Goal target;
  if (ally_head >= 0 && paths.dist[ally_head] > 0) target = Goal{ally_head, false};
  else if (field_ == 1 && [&] {   // g46-1: the opening mode owns the goal outright
             const int32_t f = FieldGoal(paths);
             if (f >= 0) target = Goal{f, false};
             return f >= 0;
           }()) {}
  else if (founding && !founder_here) target = Goal{founder_goal, false};   // g43-6
  else if (rush_state_ == 1 && CONTEST_MODE) {
    // g42-6: an explorer whose best goal is food (scheduled, contest-weighted, reachable within
    // kSchedHorizon) takes it instead of the frontier; with no such food it explores as before.
    // arena-r16 (trophy 152244): our second starter walked to the board edge while the enemy
    // took the food beside it.
    target = ChooseGoal(paths);
    int32_t ready = 0;
    bool food = target.tile >= 0 && paths.dist[target.tile] > 0 &&
                TileValue(target.tile, paths.dist[target.tile], &ready) > 0 && ready <= kSchedHorizon;
    // g42-12: ... but only food that is genuinely contested (a rival head could get there within
    // kSchedHorizon, whoever is first) or close (within kExplorerNear steps). Uncontested far food
    // turned explorers into home farmers on open maps (big_empty 159150: -34 total length at r60).
    if (food && paths.dist[target.tile] > kExplorerNear) {
      int32_t margin = 0;
      ContestWon(target.tile, ready, &margin);
      food = ready + margin <= kSchedHorizon;   // ready + margin: the nearest rival's arrival
    }
    if (!food) {
      poor_ = false;   // ChooseGoal's side effect; the frontier walk never set it
      if (const int32_t rush = RushGoal(paths); rush >= 0) target = Goal{rush, false};
    }
  }
  else if (const int32_t rush = rush_state_ == 1 ? RushGoal(paths) : -1; rush >= 0) target = Goal{rush, false};   // g39-4
  else target = ChooseGoal(paths);
  walking_to_ally_ = ally_head >= 0 && paths.dist[ally_head] > 0;   // (gst-search-4)
  // g42-6: the food this worker races for (an explorer's food goal, or food an enemy head is
  // coming for that we reach first): no growth split until it is eaten or lost (step 4)
  contest_goal_ = -1;
  if (CONTEST_MODE && round_ < kContestUntil && !IsQueen() && target.tile >= 0 && paths.dist[target.tile] > 0) {
    int32_t ready = 0;
    if (TileValue(target.tile, paths.dist[target.tile], &ready) > 0 &&
        ((rush_state_ == 1 && ready <= kSchedHorizon) || ContestWon(target.tile, ready)))
      contest_goal_ = target.tile;
  }
#ifdef CLAIMMID_DBG
  if (contest_goal_ >= 0)
    std::fprintf(stderr, "CMDBG r%d team%d id%d contest%d call%d\n", round_, spawn_.team, spawn_.id, 1,
                 int(call_goal_ || std::find(dbg_detour_.begin(), dbg_detour_.end(), target.tile) != dbg_detour_.end()));
  else if (call_goal_ || std::find(dbg_detour_.begin(), dbg_detour_.end(), target.tile) != dbg_detour_.end())
    std::fprintf(stderr, "CMDBG r%d team%d id%d contest%d call%d\n", round_, spawn_.team, spawn_.id, 0, 1);
#endif
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
  if (round_ < kGiveUpUntil && length_ >= 2 && length_ <= kGiveUpMaxLen && !leader_ && FreeSlots() <= kGiveUpFree &&
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
  // g40-6: the goal is a bed whose pearl is still to grow (rounds to its tick; -1 otherwise)
  const int32_t goal_due = [&] {
    if (!BEDTIME_MODE || goal < 0) return -1;
    const TileMemory& m = board_.at(goal);
    if (m.pearl || m.countdown < 0 || m.seen_round < 0) return -1;
    const int32_t due = m.countdown - (round_ - m.seen_round);
    return due >= 1 ? due : -1;
  }();
  const bool waiting = goal >= 0 && paths.dist[goal] > 0 && (goal_due >= 1 || [&] {
    int32_t ready;
    return TileValue(goal, paths.dist[goal], &ready) > 0.0 && ready > paths.dist[goal];
  }());
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
  int preferred = goal >= 0 ? paths.first[goal] : -1;
  // g40-6: next to a bed before its tick, stepping on would put us on it at the tick (a blocked
  // pearl, the wait wasted). Take a free step that stays nearest to it instead and come back
  // after the tick; ChooseGoal decides each turn whether the wait is still the best use of time.
  bool bed_wait = false;
  if (goal_due >= 1 && paths.dist[goal] == 1 && preferred >= 0 && board_.Step(head_, preferred) == goal) {
    int alt = -1, alt_d = 1 << 20;
    for (int dir = 0; dir < 4; ++dir) {
      const int32_t to = board_.Step(head_, dir);
      if (to < 0 || to == goal || Classify(to) != Cell::kFree) continue;
      const int32_t dx = std::abs(board_.X(to) - board_.X(goal)), dy = std::abs(board_.Y(to) - board_.Y(goal));
      const int32_t dd = std::min(dx, board_.width() - dx) + std::min(dy, board_.height() - dy);
      if (dd < alt_d) alt = dir, alt_d = dd;
    }
    if (alt >= 0) preferred = alt, bed_wait = true;
  }
  // g40-6 v2 (ring-bedlearn-2): on the way to any other goal, a first step onto a bed whose tick
  // falls while our body still covers it (1 <= due <= length, from the countdown we saw) blocks
  // that pearl and the engine redraws its gap. Take an equally short first step instead when one
  // exists, else a sidestep (distance to the goal by a breadth-first pass from it, only on such turns).
  int32_t bed_skip = -1;
  if (BEDTIME_MODE && !bed_wait && goal >= 0 && preferred >= 0) {
    auto blocks = [&](int32_t t) {
      if (t < 0) return false;
      const TileMemory& m = board_.at(t);
      if (m.pearl || m.countdown < 0 || m.seen_round < 0) return false;
      const int32_t due = m.countdown - (round_ - m.seen_round);
      return due >= 1 && due <= length_;
    };
    const int32_t to = board_.Step(head_, preferred);
    if (to != goal && blocks(to)) {
      const int32_t limit = paths.dist[goal];
      std::vector<int32_t> dg(board_.size(), -1);
      std::vector<int32_t> q{goal};
      dg[goal] = 0;
      for (size_t qi = 0; qi < q.size(); ++qi) {
        const int32_t t = q[qi];
        if (dg[t] >= limit + 2) continue;
        for (int dir = 0; dir < 4; ++dir) {
          const int32_t n = board_.Step(t, dir);
          if (n < 0 || dg[n] >= 0 || (n != head_ && Occupied(n))) continue;
          dg[n] = dg[t] + 1;
          q.push_back(n);
        }
      }
      // the shortest way that does not step on it; a sidestep costs at most 2 rounds (one step out,
      // one back), less than the pearl it saves, and lasts only until its tick (due <= length rounds)
      int best_d = 1 << 20;
      if (dg[to] >= 0)
        for (int dir = 0; dir < 4; ++dir) {
          const int32_t n = board_.Step(head_, dir);
          if (n < 0 || n == to || Classify(n) != Cell::kFree || blocks(n) || dg[n] < 0 || dg[n] > dg[to] + 2 || dg[n] >= best_d) continue;
          best_d = dg[n];
          preferred = dir;
          bed_skip = to;
        }
    }
  }
  cur_goal_ = goal;
  avoid_tile_ = bed_wait ? goal : bed_skip;
  bot::Action a = SafeStep(preferred, bed_wait ? false : target.trap, bed_wait ? false : target.grow);
  avoid_tile_ = -1;
  // nowhere else to go (or the way around is not a plain safe goal step): step on after all
  if ((bed_wait && (a.kind != bot::Action::kMove || a.resign)) ||
      (bed_skip >= 0 && (a.kind != bot::Action::kMove || a.resign || std::string(reason_) != "toward goal"))) {
    preferred = paths.first[goal];
    a = SafeStep(preferred, target.trap, target.grow);
    bed_wait = false;
    bed_skip = -1;
  }
  cur_goal_ = -1;
  if (bed_wait && a.kind == bot::Action::kMove && !a.resign) reason_ = "bed: wait for its tick";
  if (bed_skip >= 0 && a.kind == bot::Action::kMove && !a.resign && std::string(reason_) == "toward goal")
    reason_ = "bed: step around its tick";
  // g40-merge: no sprint while a bed step is being held back, or the second step undoes the wait
  if (contest_goal_ >= 0 && contest_goal_ == goal && std::string(reason_) == "toward goal")   // g42-6 labels
    reason_ = rush_state_ == 1 ? "contest: explorer food" : "contest: race";
  if (rush_state_ == 1 && goal >= 0 && !bed_wait && bed_skip < 0) {   // g39-4: a second free step on the rush
    a = RushSprint(a, goal);
    if (a.steps.size() > 1) reason_ = "rush: sprint";
    else if (std::string(reason_) == "toward goal") reason_ = "rush: frontier";
  }
  // g43-5: free sprint steps for non-queen dragons of length 5+ on a plain goal walk. Not while
  // the goal is food not ready on arrival (arriving sooner only waits, or blocks a bed's tick),
  // not on a bed wait/skip, and not while walking to an ally (feeding; the meeting is the plan).
  if (SPRINT_MODE && !IsQueen() && rush_state_ != 1 && goal >= 0 && !bed_wait && bed_skip < 0 && !waiting &&
      goal_due < 1 && !(ally_head >= 0 && goal == ally_head) && length_ >= 5) {
    const std::string why(reason_);
    if (why == "toward goal" || why == "contest: race" || why == "contest: explorer food") {
      a = FreeSprint(a, goal);
      if (a.steps.size() > 1) reason_ = why == "toward goal" ? "sprint: toward goal" : "sprint: contest";
    }
  }
  if (field_ == 1 && goal >= 0) {   // g46-1 labels
    const std::string why(reason_);
    if (why == "toward goal" || why == "contest: race" || why == "contest: explorer food")
      reason_ = field_call_ ? "field: to call" : goal == field_target_ ? "field: heading" : "field: pearl on the way";
    else if (why == "sprint: toward goal" || why == "sprint: contest")
      reason_ = "field: sprint";
  }
  // the last step onto an enemy head we chose to hunt is a trade, not a mistake
  if (goal >= 0 && preferred >= 0 && board_.Step(head_, preferred) == goal && Classify(goal) == Cell::kEnemyHead &&
      WorthTrading(board_.at(goal).occupant))
    return Because("hunt: trade", bot::Action::Move(preferred));
  if (held && std::string(reason_) == "toward goal") reason_ = "held: peer not clearly longer";
  if (call_goal_ && std::string(reason_) == "toward goal") reason_ = "call: toward cluster";
  if (founding && goal == founder_goal &&
      (std::string(reason_) == "toward goal" || std::strncmp(reason_, "contest", 7) == 0)) reason_ = "founder: dispatch";
  call_goal_ = false;
  return a;
}

}  // namespace gm2

namespace gm2 {
// g40-3: who the late champion is, and whether we concentrate into it this turn (see kLateConcFrom)
void StrategyAgent::UpdateLateChampion() {
  late_conc_ = false;
  die_near_ = false;
  if (IsQueen() || round_ < kDieNearFrom) { late_champ_ = -1; return; }
  // never feed a champion while our queen can still grow
  for (int q = 0; q < 2; ++q) {
    const auto r = reports_.find(q);
    if (r != reports_.end() && r->second.length > 0 && round_ - r->second.round <= kQueenAliveFresh) return;
    // a sighting counts unless a later report says she died
    const bool reported_dead = r != reports_.end() && r->second.length == 0;
    if (const auto w = seen_.find(q); w != seen_.end() && w->second.team == spawn_.team && w->second.length > 0 &&
                                      round_ - w->second.round <= kQueenAliveFresh &&
                                      !(reported_dead && r->second.round >= w->second.round))
      return;
  }
  // a candidate is alive while its report is at most kReserveFresh rounds old (we can't tell a
  // dead champion from one out of sonar contact; the freshness the feeding phase already trusts)
  auto alive = [&](int32_t id) {
    const auto it = reports_.find(id);
    return it != reports_.end() && it->second.length > 0 && round_ - it->second.round <= kReserveFresh;
  };
  // g42-8: ONE champion. Every dragon takes the longest live report (ties: lower id), re-decided
  // each round. g40-3's pick stuck until the champion died and newborns named themselves, so groups
  // kept different champions; with one total order every group that hears the same reports names
  // the same dragon, and the champion relays itself, so the groups converge as reports spread.
  late_champ_ = -1;
  int32_t best_len = 0;
  for (const auto& [id, r] : reports_) {
    if (id <= 1 || !alive(id)) continue;
    if (late_champ_ < 0 || r.length > best_len || (r.length == best_len && id < late_champ_)) {
      late_champ_ = id;
      best_len = r.length;
    }
  }
  die_near_ = late_champ_ >= 0;
  late_conc_ = die_near_ && round_ >= kLateConcFrom;
}

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

namespace gm2 {
// g40-4 (ring-qrace-1) queen race. When both queens are alive at r500 the longer queen wins, and
// late our material stays in workers (arena-r14: 16 dragons v 7 in the late losses, the queen fed
// 6.6 v 11.2 from r430). So from kRaceFrom, while the enemy queen is known alive (seen within
// kRaceFresh rounds, by us or relayed), a worker that can reach our queen in time walks to her and
// resigns once its head is within kRaceReach free steps of her head: its pearls are hers.
bot::Action StrategyAgent::QueenRace(const Report& champ) {
#ifdef QRACE_DBG
  if (round_ >= kRaceFrom && (round_ % 10) == 0) std::fprintf(stderr, "QR r%d team%d id%d eq%d champ%d clen%d cage%d\n", round_, spawn_.team, spawn_.id, round_ - enemy_queen_seen_, champ.id, champ.length, round_ - champ.round);
#endif
  if (round_ < kRaceFrom || round_ >= 499 || round_ - enemy_queen_seen_ > kRaceFresh || length_ < kFeedMin) return {};
  if (champ.id < 0 || champ.id > 1 || champ.id == spawn_.id || champ.length <= 0) return {};
  int32_t target = -1;
  bool in_view = false;
  if (const auto it = seen_.find(champ.id); it != seen_.end() && it->second.team == spawn_.team && it->second.round == round_ &&
      it->second.head >= 0 && board_.Current(it->second.head) && board_.at(it->second.head).occupant == champ.id) {
    target = it->second.head;
    in_view = true;
  } else if (champ.tile >= 0 && round_ - champ.round <= kRaceFresh / 3) {
    target = champ.tile;   // relayed every round; she moves slowly late, so a recent report is close
  }
  if (target < 0) return {};
  // her feeding zone: free tiles within kRaceReach steps of her head (pearls she can take at once)
  const Paths zone = Search(board_, target, [&](int32_t t) { return t == head_ || !Occupied(t); }, kRaceReach);
  if (in_view && zone.dist[head_] > 0 && zone.dist[head_] <= kRaceReach) {
    rescue_committed_ = true;
    return Because("qrace: feed queen", bot::Action::Resign());
  }
  // "nearby": we arrive with at least as many rounds left for her to eat our pearls as we walked
  const int32_t budget = (500 - round_) / 2;
  if (budget < 1) return {};
  const Paths paths = Search(board_, head_, [&](int32_t t) {
    return (RING5_MODE != 3 || !Ring5Reserved(t)) && !Occupied(t);
  }, budget);
  const int32_t goal = paths.Nearest([&](int32_t t) { return t != target && zone.dist[t] > 0; });
  if (goal < 0 || paths.dist[goal] <= 0) return {};
  cur_goal_ = goal;
  bot::Action a = SafeStep(paths.first[goal], false);
  cur_goal_ = -1;
  if (std::string(reason_) == "toward goal") reason_ = "qrace: walk to queen";
  return a;
}
}  // namespace gm2

