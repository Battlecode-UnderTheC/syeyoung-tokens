// g43-6 (ring-founder-1): founder dispatch on small starts (see strategy.hpp, kFounderUnits).
//
// A founder walks to the largest pearl region it has seen that it reaches no later than any enemy
// head seen in the last kRivalMemory rounds (the contest's rival test, RivalDist), and holds back
// from regions an enemy head is nearer to. A region is the pearls (as last seen) linked within
// kRegionLink of each other; its size is their count. Live pearls only, not soon-due beds: on maps
// where every tile is a slow bed (Colosseum's rim, gap up to 250) a few due beds lie around any
// spawn and would make home a region (iteration 1: the founder "arrived" at home on its first
// turn). With no region of kRegionMin pearls (one pearl is food, not a place to settle) it
// explores the frontier as the starters' explorers do (RushGoal: food within 2 first).
// Observation only: no map keys, no fixed board points.
#include <algorithm>
#include <cstdio>

#include "strategy.hpp"

namespace gm2 {
using namespace strat;

namespace {
constexpr double kRegionMin = 2.0;   // rush.cpp's kClusterMin: two live beds are worth farming
constexpr size_t kMaxBeds = 400;     // CPU bound on the region grouping (pairs: 160k at most)
// g53-18 (strategist D g53 #3): a region an enemy head reaches first is dropped only when their
// heads within kRallyReach of it outnumber ours (us included: we are going). Trophy 265787 r24:
// our founder 2-4 tiles from the cross turned to the rim beds while 3 orange heads held it; the
// rally call (call.cpp) brings allies within the same 12 steps, so the count uses that radius.
constexpr int32_t kRallyReach = 12;
}  // namespace

int32_t StrategyAgent::FounderGoal(const Paths& paths, bool* here) {
  *here = false;
  std::vector<int32_t> beds;
  for (int32_t t = 0; t < board_.size() && beds.size() < kMaxBeds; ++t)
    if (board_.at(t).seen_round >= 0 && board_.at(t).pearl && !Banned(t)) beds.push_back(t);
  // regions: connected components of beds linked within kRegionLink
  std::vector<int32_t> region(beds.size(), -1);
  int32_t regions = 0;
  for (size_t i = 0; i < beds.size(); ++i) {
    if (region[i] >= 0) continue;
    region[i] = regions;
    std::vector<size_t> queue{i};
    for (size_t qi = 0; qi < queue.size(); ++qi)
      for (size_t j = 0; j < beds.size(); ++j)
        if (region[j] < 0 && Torus(beds[queue[qi]], beds[j]) <= kRegionLink) {
          region[j] = regions;
          queue.push_back(j);
        }
    ++regions;
  }
  struct Region {
    int32_t size = 0, ours = 1 << 20, theirs = 1 << 20, near = -1;
    int32_t our_heads = 1, their_heads = 0;   // g53-18: heads within kRallyReach of the region
  };
  std::vector<Region> r(regions);
  for (size_t i = 0; i < beds.size(); ++i) {
    Region& g = r[region[i]];
    const int32_t t = beds[i];
    ++g.size;
    const int32_t d = paths.dist[t];
    if (d >= 0 && d < g.ours) {
      g.ours = d;
      g.near = t;
    }
    for (const auto& [id, w] : seen_)
      if (w.team != spawn_.team && w.head >= 0 && round_ - w.round <= kRivalMemory)
        g.theirs = std::min(g.theirs, RivalDist(Torus(w.head, t), round_ - w.round));
  }
  // g53-18: heads within kRallyReach of any bed of the region (allies known now; rivals as the
  // contest remembers them)
  {
    const std::vector<int32_t> allies = AllyHeads();
    std::vector<int32_t> rivals;
    for (const auto& [id, w] : seen_)
      if (w.team != spawn_.team && w.head >= 0 && round_ - w.round <= kRivalMemory) rivals.push_back(id);
    // one pass over the beds per head (heads x beds: 64 x 400 at most)
    std::vector<uint8_t> in(regions);
    auto count = [&](int32_t head, int32_t age, bool ally) {
      std::fill(in.begin(), in.end(), 0);
      for (size_t i = 0; i < beds.size(); ++i) {
        const int32_t k = region[i];
        if (in[k] || r[k].ours <= r[k].theirs) continue;
        const int32_t d = Torus(head, beds[i]);
        if ((ally ? d : RivalDist(d, age)) <= kRallyReach) in[k] = 1;
      }
      for (int32_t k = 0; k < regions; ++k)
        if (in[k]) ++(ally ? r[k].our_heads : r[k].their_heads);
    };
    for (const int32_t a : allies) count(a, 0, true);
    for (const int32_t id : rivals) count(seen_.at(id).head, round_ - seen_.at(id).round, false);
  }
  int32_t best = -1;
  for (int32_t k = 0; k < regions; ++k) {
    const Region& g = r[k];
    // unreachable, small, or theirs first and they outnumber us there (g53-18)
    if (g.near < 0 || g.size < kRegionMin || (g.ours > g.theirs && g.their_heads > g.our_heads)) continue;
    // iteration 2: standing in any such region counts as arrived. With arrival only at the largest
    // one, founders crossed a rich region at length 4 without splitting (duel-eb6b54e5 trophy
    // games 6-7: the founder reached 4 in the cup at r30 and walked on to a larger one, unsplit,
    // while the base's child split there; 3 v 4 dragons at r10, 5 v 8 at r30).
    if (g.ours <= kArrive) *here = true;
    if (best < 0 || g.size > r[best].size || (g.size == r[best].size && g.ours < r[best].ours)) best = k;
  }
#ifdef RALLY_DBG
  for (int32_t k = 0; k < regions; ++k) std::fprintf(stderr, "  reg size%d ours%d theirs%d oh%d th%d near%d,%d\n", r[k].size, r[k].ours, r[k].theirs, r[k].our_heads, r[k].their_heads, r[k].near>=0?board_.X(r[k].near):-1, r[k].near>=0?board_.Y(r[k].near):-1);
  std::fprintf(stderr, "FG r%d id%d beds%zu regions%d best%d size%d ours%d theirs%d here%d\n", round_, spawn_.id, beds.size(), regions, best,
               best >= 0 ? r[best].size : -1, best >= 0 ? r[best].ours : -1, best >= 0 ? r[best].theirs : -1, int(*here));
#endif
  if (best >= 0) {
    if (r[best].ours <= kArrive) {
      *here = true;   // there: it farms (ChooseGoal) and splits at 4, which ends the dispatch (strategy.cpp)
      return -1;
    }
    return r[best].near;   // on the way; in a smaller region on the way it may split (strategy.cpp)
  }
  // no region yet: the nearest lone pearl it reaches first (iteration 5: on Colosseum, seed 11, the
  // frontier walk dithered in the home corner for 18 rounds while lone pearls lay 5-8 steps away
  // towards the fast beds; eating one shows the next part of the board), else the frontier
  int32_t food = -1;
  for (int32_t k = 0; k < regions; ++k) {
    const Region& g = r[k];
    if (g.near >= 0 && g.ours > 0 && g.ours <= g.theirs && (food < 0 || g.ours < r[food].ours)) food = k;
  }
  const int32_t g = food >= 0 ? r[food].near : RushGoal(paths);
#ifdef FOUNDER_DBG
  std::fprintf(stderr, "FG r%d id%d food%d rush%d\n", round_, spawn_.id, food, g);
#endif
  return g;
}

}  // namespace gm2
