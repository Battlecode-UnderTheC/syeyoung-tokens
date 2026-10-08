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
  };
  // g53-2 (ring-sealed-trophy): allies in view now. Observed cause (arena 262844, trophy, sim trace
  // of the same seed): at r24 our founder ate the cup's lower bed at (16,14), the cup's last pearl
  // it knew, and turned back 8 steps to the base's 2-pearl region, where an ally head already stood
  // 1 step from it (it ate them by r25). The founder then came back to the cup and first saw at r28
  // the central cross; the enemy's mirror founder had seen it from the mirror bed at r24 and took it
  // at r31. The region test counted enemy heads only, so an ally on the spot did not lower a
  // region's worth. Now each worker head nearer to a region than we are takes one of its pearls (one
  // dragon eats one pearl at a time, as the contest's claims and the walled-field rule count it).
  // The queen is not counted, as in the scheduler (ChooseGoal, SCHED_MODE): she keeps to her home
  // pull and doesn't walk to a region (iteration 1, sim trophy seed 4: counting her, the founder
  // gave up the cup's upper bed she sat 4 tiles from and walked away from the cup).
  std::vector<int32_t> ally_heads;
  for (const auto& [id, w] : seen_)
    if (w.team == spawn_.team && w.round == round_ && w.head >= 0 && id != spawn_.id && id > 1) ally_heads.push_back(w.head);
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
  // g53-2: a region's pearls less one for each ally head in view that is nearer to it than we are
  std::vector<int32_t> share(regions, 0);
  for (const int32_t a : ally_heads) {
    std::vector<int32_t> nearest(regions, 1 << 20);
    for (size_t i = 0; i < beds.size(); ++i) nearest[region[i]] = std::min(nearest[region[i]], Torus(a, beds[i]));
    for (int32_t k = 0; k < regions; ++k)
      if (r[k].near >= 0 && nearest[k] < r[k].ours) ++share[k];
  }
  for (int32_t k = 0; k < regions; ++k) r[k].size -= share[k];
  int32_t best = -1;
  for (int32_t k = 0; k < regions; ++k) {
    const Region& g = r[k];
    if (g.near < 0 || g.size < kRegionMin || g.ours > g.theirs) continue;   // unreachable, small, or theirs first
    // iteration 2: standing in any such region counts as arrived. With arrival only at the largest
    // one, founders crossed a rich region at length 4 without splitting (duel-eb6b54e5 trophy
    // games 6-7: the founder reached 4 in the cup at r30 and walked on to a larger one, unsplit,
    // while the base's child split there; 3 v 4 dragons at r10, 5 v 8 at r30).
    if (g.ours <= kArrive) *here = true;
    if (best < 0 || g.size > r[best].size || (g.size == r[best].size && g.ours < r[best].ours)) best = k;
  }
#if 0
  for (int32_t k = 0; k < regions; ++k) std::fprintf(stderr, "  reg size%d ours%d theirs%d near%d,%d\n", r[k].size, r[k].ours, r[k].theirs, r[k].near>=0?board_.X(r[k].near):-1, r[k].near>=0?board_.Y(r[k].near):-1);
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
    if (g.near >= 0 && g.size > 0 && g.ours > 0 && g.ours <= g.theirs && (food < 0 || g.ours < r[food].ours)) food = k;
  }
  const int32_t g = food >= 0 ? r[food].near : RushGoal(paths);
#ifdef FOUNDER_DBG
  std::fprintf(stderr, "FG r%d id%d food%d rush%d\n", round_, spawn_.id, food, g);
#endif
  return g;
}

}  // namespace gm2
