// g39-4 (ring-rush-1): win the race for the shared middle.
//
// Server-r13: on centre maps the opponent has 2+ heads on the central bed cluster by about r16;
// we arrive at r26-31 and then stop 1-4 tiles short, because every tile there is in an enemy
// head's reach. Arriving first we go 28-16, otherwise 16-36.
//
// 1. Rushers: the two lowest-id non-queen starters (map dragons alternate teams, so ours are
//    q, q+2, q+4, ...) walk to the board's centre from r0 when it is 5..26 steps away, without
//    growth splits on the way, taking a second free step when their length allows it. Once they
//    see beds near the centre, the target becomes the densest cluster of live or soon-due beds.
//    After arriving (or at r30) they hold the middle until r60: goals far from it count half.
// 2. Local parity: before r60, a worker whose head is in the middle (within 7 of the centre)
//    doesn't count tiles next to enemy heads as exposed while our heads within 4 match theirs,
//    and trades head-on 1-for-1 while ours outnumber theirs.
// Observation only: the board's size and our own spawn, no map names or tables.
#include <algorithm>
#include <cstring>

#include "strategy.hpp"

namespace gm2 {
using namespace strat;

namespace {
constexpr int32_t kRushUntil = 30;       // the rush phase ends here
constexpr int32_t kRushMinDist = 5;      // a starter this close to the centre is already there
constexpr int32_t kRushMaxDist = 26;     // too far to arrive early: no rush
constexpr int32_t kArrive = 2;
constexpr int32_t kRushStall = 6;       // rounds without getting nearer before giving up           // within this many tiles of the target: arrived
constexpr int32_t kClusterScan = 4;      // beds within this of the centre refine the target
constexpr double kClusterMin = 2.0;      // a cluster worth retargeting to
constexpr int32_t kMiddleUntil = 60;     // the hold and the parity rule end here
constexpr int32_t kMiddleRadius = 7;     // "in the middle": head within this of the centre
}  // namespace

int32_t StrategyAgent::Torus(int32_t a, int32_t b) const {
  const int32_t dx = std::abs(board_.X(a) - board_.X(b)), dy = std::abs(board_.Y(a) - board_.Y(b));
  return std::min(dx, board_.width() - dx) + std::min(dy, board_.height() - dy);
}

bool StrategyAgent::InMiddle() const {
  return !IsQueen() && round_ < kMiddleUntil && Torus(head_, Centre()) <= kMiddleRadius;
}

void StrategyAgent::HeadsNear(int32_t at, int32_t r, int32_t* ours, int32_t* theirs) const {
  *ours = 1;   // ourselves
  *theirs = 0;
  for (const auto& [id, w] : seen_) {
    if (id == spawn_.id || w.round != round_ || w.head < 0 || Torus(w.head, at) > r) continue;
    if (w.team == spawn_.team) ++*ours;
    else ++*theirs;
  }
}

void StrategyAgent::UpdateRush() {
  if (IsQueen()) {
    rush_state_ = 3;
    return;
  }
  if (rush_state_ == 0) {
    rush_state_ = 3;
    const int32_t q = spawn_.id & 1;
    if (leader_ && round_ == 0 && (spawn_.id == q + 2 || spawn_.id == q + 4)) {
      const int32_t d = Torus(head_, Centre());
      if (d >= kRushMinDist && d <= kRushMaxDist) {
        rush_state_ = 1;
        rush_target_ = Centre();
      }
    }
  }
  if (rush_state_ == 2 && round_ >= kMiddleUntil) rush_state_ = 3;
  if (rush_state_ != 1) return;
  if (round_ >= kRushUntil) {
    rush_state_ = Torus(head_, rush_target_) <= kMiddleRadius ? 2 : 3;
    return;
  }
  // the densest cluster of live or soon-due beds seen near the centre
  const int32_t c = Centre(), cx = board_.X(c), cy = board_.Y(c);
  auto weight = [&](int32_t t) {
    const TileMemory& m = board_.at(t);
    if (m.seen_round < 0) return 0.0;
    if (m.pearl) return 1.0;
    if (m.countdown < 0) return 0.0;
    const int32_t due = m.countdown - (round_ - m.seen_round);
    return due <= 30 ? 1.0 : 0.0;   // a slow bed (long countdown) is no cluster
  };
  int32_t best = -1;
  double best_score = kClusterMin - 1e-9;
  for (int32_t dy = -kClusterScan; dy <= kClusterScan; ++dy)
    for (int32_t dx = -kClusterScan + std::abs(dy); dx <= kClusterScan - std::abs(dy); ++dx) {
      const int32_t t = board_.Tile(cx + dx, cy + dy);
      if (weight(t) <= 0) continue;
      double s = 0;
      const int32_t x = board_.X(t), y = board_.Y(t);
      for (int32_t ey = -2; ey <= 2; ++ey)
        for (int32_t ex = -2 + std::abs(ey); ex <= 2 - std::abs(ey); ++ex) s += weight(board_.Tile(x + ex, y + ey));
      if (s > best_score + 1e-9 || (best >= 0 && s > best_score - 1e-9 && Torus(t, c) < Torus(best, c))) {
        best = t;
        best_score = s;
      }
    }
  if (best >= 0 && best != rush_target_) {
    rush_target_ = best;
    rush_best_ = -1;
  }
  const int32_t left = Torus(head_, rush_target_);
  if (left <= kArrive) rush_state_ = 2;
  // no step nearer the target for kRushStall rounds: the way is shut, stop rushing
  if (rush_best_ < 0 || left < rush_best_) {
    rush_best_ = left;
    rush_since_ = round_;
  } else if (rush_state_ == 1 && round_ - rush_since_ >= kRushStall) {
    rush_state_ = InMiddle() ? 2 : 3;
  }
}

// the tile to walk to: the target when known and reachable, else the known tile that minimises
// steps there plus the straight distance on to the target
int32_t StrategyAgent::RushGoal(const Paths& paths) const {
  const int32_t T = rush_target_;
  int32_t best = -1, best_cost = 1 << 30, best_left = 1 << 30;
  for (int32_t t = 0; t < board_.size(); ++t) {
    const int32_t d = paths.dist[t];
    if (d <= 0 || Banned(t) || Classify(t) != Cell::kFree) continue;
    const int32_t left = Torus(t, T);
    // only the frontier (a tile beside an edge never seen) or the target's own neighbourhood:
    // a known tile walled off from the target is no way there
    bool frontier = left <= kArrive;
    for (int k = 0; k < 4 && !frontier; ++k) frontier = board_.Step(t, k) == kUnknown;
    if (!frontier) continue;
    int32_t cost = d + left;
    if (cost < best_cost || (cost == best_cost && left < best_left)) {
      best = t;
      best_cost = cost;
      best_left = left;
    }
  }
  return best;
}

// a second free step along the shortest known path to the goal, when our length makes it free
// and the tile is in view, free, not exposed and roomy
bot::Action StrategyAgent::RushSprint(bot::Action a, int32_t goal) const {
  if (goal < 0 || (length_ + 3) / 4 < 2 || a.kind != bot::Action::kMove || a.resign || a.steps.size() != 1 ||
      std::strcmp(reason_, "toward goal") != 0)
    return a;
  const int32_t t1 = board_.Step(head_, a.steps[0]);
  if (t1 < 0 || t1 == goal) return a;
  // distances to the goal over free known tiles
  std::vector<int32_t> g(board_.size(), -1);
  std::vector<int32_t> queue{goal};
  g[goal] = 0;
  for (size_t qi = 0; qi < queue.size(); ++qi) {
    const int32_t t = queue[qi];
    if (t == t1 || g[t] >= 40) continue;
    for (int d = 0; d < 4; ++d) {
      const int32_t n = board_.Step(t, d);
      if (n < 0 || g[n] >= 0 || (n != t1 && Classify(n) != Cell::kFree)) continue;
      g[n] = g[t] + 1;
      queue.push_back(n);
    }
  }
  if (g[t1] < 2) return a;
  const int32_t need = RoomNeeded();
  auto open = [&](int32_t t) { return !Occupied(t) && !Exposed(t); };
  for (int d = 0; d < 4; ++d) {
    const int32_t t2 = board_.Step(t1, d);
    if (t2 < 0 || t2 == head_ || !board_.Current(t2) || Classify(t2) != Cell::kFree || g[t2] != g[t1] - 1) continue;
    if (Exposed(t2) || RoomAround(board_, t2, open, need) < need) continue;
    a.steps.push_back(d);
    return a;
  }
  return a;
}

}  // namespace gm2
