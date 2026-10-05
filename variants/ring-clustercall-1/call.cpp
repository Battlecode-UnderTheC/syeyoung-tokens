// g42-1 (ring-clustercall-1): a team-wide early cluster call over sonar.
//
// Evidence (g41): on centre-cluster maps the winning line (ring-sched-8) grew from 5.8 to 12
// dragons in r20-40 by piling onto rich bed clusters before any contact; ring-sched-17's frontier
// explorers find such clusters, but only the finder farms them. Here the find is told to the team.
//
// 1. Find: before kCallUntil, any dragon (queen included: she sees too) scores each tile in its
//    7x7 view by ClusterScore (live or soon-due beds within 2, rush.cpp). The best tile is a find
//    when it has at least kCallMin beds and kCallSurplus more beds than our heads within
//    kContestReach of it (one dragon eats one pearl at a time: the surplus is food nobody of ours
//    is there to eat; 2 spare beds bring one 2-long worker to its growth split).
// 2. Word (64-bit, magic 0xA9): team (1) | x (6) | y (6) | surplus (5) | round found (9). Sent on
//    two opposite directions each turn (alternating N/S and E/W), so the champion word keeps the
//    other two (the finder: all four). Once the queen's ring is known every dragon relays the ring
//    word on all four directions instead; the call then rides in that word's spare bits 26-52.
//    Every dragon relays the best fresh call it heard (with its original round, so a call dies
//    kCallFresh rounds after its finder stops seeing the surplus).
// 3. Answer: a worker (not the queen, not a frontier explorer) farther than kCallReached from the
//    cluster, and not having seen it since the call, values the call as a known bed in a field of
//    `surplus` beds (ChooseGoal's cluster bonus) per (rounds to get there + 1), the travel a lower
//    bound (known path to a tile, then torus distance), and walks there when that beats the score
//    of the goal it chose. Once there it sees the beds and the scheduler takes over (exact ticks).
// Observation only: no board-size gates, no fixed board points, no map keys.
#include <algorithm>
#include <cstdio>

#include "strategy.hpp"

namespace gm2 {
using namespace strat;

namespace {
constexpr uint64_t kCallMagic = 0xA9;
constexpr int32_t kCallUntil = 40;     // the opening race for clusters (g41: held by r20-40)
constexpr int32_t kCallFresh = 8;      // a call not refreshed for this long: the finder left or allies came
constexpr int32_t kCallMin = 3;        // beds within 2: more than the frontier's 'worthwhile' 2 (rush.cpp)
// surplus beds over our heads there: a 2-long worker needs 2 pearls to reach the growth-split
// length 4, so 2 spare beds bring one more worker to its split (4 was tried: arena eval .47 v .52)
constexpr int32_t kCallSurplus = 2;
constexpr int32_t kCallNear = 4;       // our heads this near the cluster are already farming it (kContestReach)
constexpr int32_t kCallReached = 2;    // within the cluster's radius: the beds are in view, ChooseGoal decides
}  // namespace

uint64_t StrategyAgent::CallWord(int32_t tile, const Call& c) const {
  return (kCallMagic << 56) | (uint64_t(spawn_.team & 1) << 55) | (uint64_t(board_.X(tile) & 63) << 37) |
         (uint64_t(board_.Y(tile) & 63) << 31) | (uint64_t(std::min(c.value, 31)) << 26) | (uint64_t(c.round & 511) << 10);
}

// the ring word (ring.cpp: magic 0xBA, bits 0-25) with the call in bits 26-52:
// valid (1) | x (6) | y (6) | surplus (5) | round found (9)
uint64_t StrategyAgent::CallInRing(uint64_t ring_word, int32_t tile, const Call& c) const {
  ring_word &= ~(((uint64_t(1) << 27) - 1) << 26);
  return ring_word | (uint64_t(1) << 26) | (uint64_t(board_.X(tile) & 63) << 27) | (uint64_t(board_.Y(tile) & 63) << 33) |
         (uint64_t(std::min(c.value, 31)) << 39) | (uint64_t(c.round & 511) << 44);
}

void StrategyAgent::HearCall(int32_t x, int32_t y, int32_t v, int32_t r) {
  // the 9-bit round: the latest round at or before now with those low bits
  r = round_ - ((round_ - r) & 511);
  if (x >= board_.width() || y >= board_.height() || r < 0 || v <= 0) return;
  Call& c = calls_[board_.Tile(x, y)];
  if (r > c.round || (r == c.round && v > c.value)) c = {v, r};
}

void StrategyAgent::UpdateCall(const rl::Observation& o) {
  own_call_ = -1;
  // what we heard (Decode only sees the word, so the inbox is read here)
  for (const uint64_t w : o.inbox) {
    if ((w >> 56) == kCallMagic && int32_t((w >> 55) & 1) == (spawn_.team & 1))
      HearCall(int32_t((w >> 37) & 63), int32_t((w >> 31) & 63), int32_t((w >> 26) & 31), int32_t((w >> 10) & 511));
    else if ((w >> 56) == 0xBA && int32_t((w >> 25) & 1) == (spawn_.team & 1) && ((w >> 26) & 1))
      HearCall(int32_t((w >> 27) & 63), int32_t((w >> 33) & 63), int32_t((w >> 39) & 31), int32_t((w >> 44) & 511));
  }
  for (auto it = calls_.begin(); it != calls_.end();)
    it = round_ - it->second.round > kCallFresh ? calls_.erase(it) : std::next(it);
  if (round_ >= kCallUntil) return;
  // our own find: the densest cluster centred in view
  int32_t best = -1;
  double best_v = 0;
  for (int k = 0; k < rl::kWindowTiles; ++k) {
    const int32_t t = board_.Tile(o.tile_x[k], o.tile_y[k]);
    const double v = ClusterScore(t);
    if (v > best_v) best_v = v, best = t;
  }
  if (best < 0 || best_v < kCallMin - 1e-9) return;
  int32_t ours = 0, theirs = 0;
  HeadsNear(best, kCallNear, &ours, &theirs);
  if (Torus(head_, best) > kCallNear) --ours;   // HeadsNear counts us whether or not we are near
  const int32_t surplus = static_cast<int32_t>(best_v + 1e-9) - ours;
  if (surplus < kCallSurplus) return;
  own_call_ = best;
  calls_[best] = {surplus, round_};
#ifdef CALL_DBG
  std::fprintf(stderr, "CALLFIND r%d team%d id%d at(%d,%d) head(%d,%d) V%.0f ours%d theirs%d surplus%d\n", round_, spawn_.team, spawn_.id,
               board_.X(best), board_.Y(best), board_.X(head_), board_.Y(head_), best_v, ours, theirs, surplus);
#endif
}

bool StrategyAgent::CallToSend(int32_t* tile, Call* c) const {
  if (own_call_ >= 0) {
    *tile = own_call_;
    *c = calls_.at(own_call_);
    return true;
  }
  int32_t best = -1;
  for (const auto& [t, k] : calls_) {
    if (round_ - k.round > kCallFresh) continue;
    if (best < 0 || k.value > calls_.at(best).value || (k.value == calls_.at(best).value && k.round > calls_.at(best).round))
      best = t;
  }
  if (best < 0) return false;
  *tile = best;
  *c = calls_.at(best);
  return true;
}

// the first tile of our way to the best call worth more than `beat` (-1: none)
int32_t StrategyAgent::CallGoal(const Paths& paths, double beat) const {
  if (IsQueen() || rush_state_ == 1 || calls_.empty() || round_ >= kCallUntil + kCallFresh) return -1;
  int32_t goal = -1;
  double top = beat;
  for (const auto& [c, k] : calls_) {
    if (round_ - k.round > kCallFresh) continue;
    const int32_t here = Torus(head_, c);
    if (here <= kCallReached) continue;
    // we have seen the cluster ourselves since the call (our own find, or we passed by): our own
    // valuation of its beds in ChooseGoal is as fresh as the call
    if (board_.at(c).seen_round >= k.round) continue;
    // the known tile from which the cluster is nearest (known path, then torus distance)
    int32_t via = -1, eta = 1 << 20;
    for (int32_t t = 0; t < board_.size(); ++t) {
      const int32_t d = paths.dist[t];
      if (d <= 0 || Occupied(t) || Banned(t)) continue;
      const int32_t rest = Torus(t, c), e = d + rest;
      if (e < eta || (e == eta && rest < Torus(via, c))) eta = e, via = t;
    }
    if (via < 0 || Torus(via, c) >= here) continue;   // no known way brings us nearer
    // valued as ChooseGoal values a known bed in a field (1 + cluster_bonus per other bed within
    // cluster_radius), counting only the surplus beds: the ones our heads there leave over
    const double s = (1.0 + p_.cluster_bonus * (k.value - 1)) / (eta + 1.0);
    if (s > top) top = s, goal = via;
#ifdef CALL_DBG
    std::fprintf(stderr, "CALLGO? r%d team%d id%d len%d to(%d,%d) here%d eta%d v%d s%.3f beat%.3f %s\n", round_, spawn_.team, spawn_.id, length_,
                 board_.X(c), board_.Y(c), here, eta, k.value, s, beat, goal == via ? "GO" : "no");
#endif
  }
  return goal;
}

}  // namespace gm2
