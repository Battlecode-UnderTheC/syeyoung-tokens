// g47-3 (ring-sprint-carpet): the mid-game carpet for idle workers.
//
// Observed cause (docs/analysis-2026-10-07): we are the most crowded bot measured. An ally head
// within 2 of 0.43 of our heads at r30 v aAah's 0.29; we plateau at 6-7 dragons from r40 while the
// top teams triple; board control is below .5 at r20-30 on 13 of 17 maps, and r60 control of 0.7-0.8
// wins 90%. On Tower Defense and Trauma our workers stay home. aAah's mid game is an even carpet of
// single dragons with gaps between them, and it takes the bed rooms early (STYLE.md).
//
// The change: in r30-r249 a worker (never the queen) that has no food goal within kCarpetFood, or
// whose food goal is nearer to another of our heads, walks instead to the ground that widens the
// gap to its nearest ally head, preferring ground near beds it has seen that no ally head holds.
// A worker that has seen no pearl and eaten nothing for kStarved rounds leaves through an exit
// (a portal or a corridor) it has not taken. SafeStep (with the lunge guard) still makes the step.
#include <algorithm>
#include <cstdlib>

#include "strategy.hpp"

namespace gm2 {

namespace {
// the mid game: the opening race (founders, explorers, the contest) is over by r30, and from r250
// the late lookahead and the queen's feeding take over
constexpr int32_t kCarpetFrom = 30, kCarpetUntil = 250;
// an ally head farther than this does not crowd us (aAah's carpet: single dragons 3-5 apart)
constexpr int32_t kGapCap = 5;
// a bed within this of an ally head is held by it (a worker farms the beds within 2 of it)
constexpr int32_t kHeld = 2;
// how far the carpet looks for its ground (the walk it commits to), and the cost per step
constexpr int32_t kCarpetReach = 10;
constexpr double kStepCost = 0.25;
// an ally head within this of ours crowds us (the analysis's "ally within 2" share)
constexpr int32_t kCrowded = 2;
// a tile unseen for kStaleRounds rounds is worth one step of gap, up to kStaleCap (a few patrol rounds)
constexpr double kStaleRounds = 20.0, kStaleCap = 2.0;
// rounds with no pearl in view and nothing eaten before a worker leaves (strategist-g46-swarm #3)
constexpr int32_t kStarved = 8;
constexpr int32_t kLeaveReach = 24;
}  // namespace

int32_t StrategyAgent::CarpetGoal(const Paths& paths, int32_t goal, const char** why) {
  if (!CARPET_MODE || IsQueen() || round_ < kCarpetFrom || round_ >= kCarpetUntil) return -1;
  const std::vector<int32_t> allies = AllyHeads();
  auto gap = [&](int32_t t) {
    int32_t g = kGapCap;
    for (const int32_t a : allies) g = std::min(g, Torus(a, t));
    return g;
  };
  // a food goal that no ally head is nearer to: forage as before
  const int32_t here_gap = gap(head_);
  bool food_theirs = false;
  if (goal >= 0 && paths.dist[goal] > 0) {
    int32_t ready = 0;
    if (TileValue(goal, paths.dist[goal], &ready) > 0) {
      bool theirs = false;
      for (const int32_t a : allies) theirs |= Torus(a, goal) < paths.dist[goal];
      // v3: ... only in a scrum (an ally head within kCrowded of ours): a pearl an ally is nearer to
      // in a wide field is still worth our walk (v1 dropped those and ate less: 24.3 v 26.3 dragons at r100)
      food_theirs = theirs && here_gap <= kCrowded;
      if (!food_theirs) {
        leave_target_ = -1;
        return -1;
      }
    } else if (here_gap > kCrowded) {
      // v2: gst's exploring or patrol goal (no food) already spreads us over unseen ground; it is
      // kept unless an ally head is within kCrowded (the crowding the analysis measured)
      return -1;
    }
  }
  const auto open = [&](int32_t t) { return !Occupied(t); };
  const int32_t need = RoomNeeded();
  // starved: leave through an exit we have not taken
  if (CARPET_MODE != 3 && round_ - std::max({last_eat_, pearl_seen_, born_}) >= kStarved) {
    if (leave_target_ >= 0 && (leave_target_ == head_ || paths.dist[leave_target_] <= 0 || trod_[leave_target_]))
      leave_target_ = -1;
    if (leave_target_ < 0) {
      double top = 0;
      for (int32_t t = 0; t < board_.size(); ++t) {
        const int32_t d = paths.dist[t];
        if (d <= 0 || d > kLeaveReach || Banned(t)) continue;
        // the far side of a portal (we came through it if we stood there), or a corridor tile
        int32_t exit = -1;
        for (int dir = 0; dir < 4 && exit < 0; ++dir)
          if (portal_[t] >> dir & 1) {
            const int32_t to = board_.Step(t, dir);
            if (to >= 0 && paths.dist[to] == d + 1 && !trod_[to]) exit = to;
          }
        if (exit < 0 && Corridor(t) && !trod_[t]) exit = t;
        if (exit < 0) continue;
        const double score = double(1 + gap(exit)) / (paths.dist[exit] + 2);
        if (score > top) top = score, leave_target_ = exit;
      }
    }
    if (leave_target_ >= 0) {
      *why = "carpet: leave";
      return leave_target_;
    }
  } else {
    leave_target_ = -1;
  }
  // beds seen within kCarpetReach + kHeld that no ally head holds
  std::vector<int32_t> beds;
  const int32_t hx = board_.X(head_), hy = board_.Y(head_), R = kCarpetReach + kHeld;
  for (int32_t dy = -R; dy <= R; ++dy)
    for (int32_t dx = -R + std::abs(dy); dx <= R - std::abs(dy); ++dx) {
      const int32_t t = board_.Tile(hx + dx, hy + dy);
      const TileMemory& m = board_.at(t);
      if (m.seen_round < 0 || (m.countdown < 0 && !m.pearl)) continue;
      bool held = false;
      for (const int32_t a : allies) held |= Torus(a, t) <= kHeld;
      if (!held) beds.push_back(t);
    }
  int32_t best = -1, best_beds = 0;
  double top = -1e9;
  for (int32_t dy = -kCarpetReach; dy <= kCarpetReach; ++dy)
    for (int32_t dx = -kCarpetReach + std::abs(dy); dx <= kCarpetReach - std::abs(dy); ++dx) {
      const int32_t t = board_.Tile(hx + dx, hy + dy);
      const int32_t d = paths.dist[t];
      if (d <= 0 || d > kCarpetReach || Occupied(t) || Banned(t)) continue;
      int32_t near_beds = 0;
      for (const int32_t b : beds) near_beds += Torus(b, t) <= kHeld;
      // v2: ground not seen for a while counts too (a spread worker keeps covering the board:
      // v1 parked on its spot and lost board control, .42 v .54 at r200 in duel-08712fa4)
      const int32_t seen = board_.at(t).seen_round;
      const double stale = seen < 0 ? kStaleCap : std::min(kStaleCap, (round_ - seen) / kStaleRounds);
      const double score = gap(t) + std::min<int32_t>(near_beds, 4) + stale - kStepCost * d;
      if (score > top) top = score, best = t, best_beds = near_beds;
    }
  if (best < 0) return -1;
  // leaving a scrum's food: only for unheld beds elsewhere, else keep the food
  if (food_theirs && best_beds == 0) return -1;
  // already spread with no unheld beds to gain: keep gst's own goal (exploring, patrol)
  if (here_gap >= kGapCap && goal >= 0 && top <= kGapCap + kStaleCap) return -1;
  // never walk into a dead end for spacing
  if (RoomAround(board_, best, open, need) < need) return -1;
  *why = "carpet: spread";
  return best;
}

}  // namespace gm2
