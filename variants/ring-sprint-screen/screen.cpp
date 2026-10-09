// g53-14 (ring-sprint-screen): Strategist B g53 #5, pre-emptive screening for the interceptor.
//
// Observed cause: ring-sprint-intercept's trade (TryIntercept) works when it has a chance (arena
// 263249 r224/r286, 263280 r370), but allies near a dying queen were 3-7 path tiles from her killer
// and 2-3 long, so it rarely fired; arena-r27 S1a: 226 late losses where our big queen was lunged by
// a stub with no ally within 3, and allies within 6 of her fall 4.75 -> 1.56 over her last 40 rounds
// while 45 of 58 lungers come from just outside her view.
//
// The change: from kScreenFrom, while our queen is kScreenQueenMin+ long, the two workers of length
// kScreenMin..kInterceptMaxLength nearest her (of those that see her; ties by id) stop foraging and
// feeding and hold kScreenNear..kScreenFar tiles (Chebyshev) from her head on the side facing the
// nearest enemy head seen within kScreenMemory rounds (with none, the side with the most ground
// unseen for kScreenStale rounds). They never step within 1 of her head (the conveyor's rule: a body
// waiting beside her is a wall), so her next-step tiles stay free. TryIntercept fires from there.
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <utility>
#include <vector>

#include "strategy.hpp"

#ifdef SCREEN_DBG
#include <atomic>
#include <cstdio>
static std::atomic<long> g_sc[8];
static struct ScreenReport { ~ScreenReport() { std::fprintf(stderr, "SCREENGATE late-stub %ld sees-queen %ld q10 %ld ranked %ld station %ld pref %ld safe %ld\n", g_sc[0].load(), g_sc[1].load(), g_sc[2].load(), g_sc[3].load(), g_sc[4].load(), g_sc[5].load(), g_sc[6].load()); } } g_screen_report;
#define SC(i) ++g_sc[i]
#else
#define SC(i)
#endif

namespace gm2 {

bool StrategyAgent::TryScreen(const rl::Observation& o, bot::Action* out) {
  if (IsQueen() || round_ < kScreenFrom || length_ < kScreenMin || length_ > kInterceptMaxLength) return false;
  // our queen's head, in view this round
  int queen = -1;
  for (const auto& s : o.segments)
    if (s.head && s.id <= 1 && s.team == spawn_.team) queen = board_.Tile(s.x, s.y);
  SC(0);
  if (queen < 0) return false;
  SC(1);
  int qlen = 0;
  if (auto w = seen_.find(queen_id_); w != seen_.end() && w->second.round == round_) qlen = w->second.length;
  if (auto r = reports_.find(queen_id_); r != reports_.end() && round_ - r->second.round <= 2) qlen = std::max<int>(qlen, r->second.length);
  if (qlen < kScreenQueenMin) return false;
  SC(2);

  const int W = board_.width(), H = board_.height();
  auto disp = [&](int from, int to, int* dx, int* dy) {   // the shortest torus displacement
    int x = board_.X(to) - board_.X(from), y = board_.Y(to) - board_.Y(from);
    if (x > W / 2) x -= W; else if (x < -(W / 2)) x += W;
    if (y > H / 2) y -= H; else if (y < -(H / 2)) y += H;
    *dx = x; *dy = y;
  };
  auto cheb = [&](int a, int b) { int x, y; disp(a, b, &x, &y); return std::max(std::abs(x), std::abs(y)); };

  // rank: the short workers that see her (their head within the 7x7 view of hers), by distance, then id
  const int mine = Torus(head_, queen);
  int rank = 0;
  for (const auto& s : o.segments) {
    if (!s.head || s.team != spawn_.team || s.id <= 1 || s.id == spawn_.id) continue;
    auto w = seen_.find(s.id);
    if (w == seen_.end() || w->second.round != round_ || w->second.partial) continue;
    if (w->second.length < kScreenMin || w->second.length > kInterceptMaxLength) continue;
    const int h = board_.Tile(s.x, s.y);
    if (cheb(h, queen) > 3) continue;
    const int d = Torus(h, queen);
    if (d < mine || (d == mine && int(s.id) < spawn_.id)) ++rank;
  }
  if (rank >= kScreens) return false;
  SC(3);

  // the threat side: the nearest enemy head seen in the last kScreenMemory rounds, else unseen ground
  double tx = 0, ty = 0;
  int best = 1 << 30;
  for (const auto& [id, w] : seen_) {
    if (w.team == spawn_.team || w.head < 0 || round_ - w.round > kScreenMemory) continue;
    const int d = Torus(w.head, queen);
    if (d < best) { best = d; int x, y; disp(queen, w.head, &x, &y); tx = x; ty = y; }
  }
  if (best == 1 << 30) {
    constexpr int R = 8;   // the queen's view radius (3) plus a full lunge of a length-4 stub and margin
    for (int y = -R; y <= R; ++y)
      for (int x = -R; x <= R; ++x) {
        if (std::abs(x) + std::abs(y) > R || (x == 0 && y == 0)) continue;
        const int t = board_.Tile(board_.X(queen) + x, board_.Y(queen) + y);
        const TileMemory& m = board_.at(t);
        if (m.seen_round >= 0 && round_ - m.seen_round <= kScreenStale) continue;
        tx += x; ty += y;
      }
    const double n = std::hypot(tx, ty);
    if (n < 1e-9) return false;   // nothing to face
    tx = tx / n * R; ty = ty / n * R;
  }

  // reachable tiles: a BFS from our head over free tiles in view, never within 1 of her head
  // (bounded by the 7x7 view: at most 49 tiles), remembering each one's first step
  auto allowed = [&](int t) { return t >= 0 && board_.Current(t) && Classify(t) == Cell::kFree && cheb(t, queen) >= kScreenNear; };
  std::vector<std::pair<int, int>> reach{{head_, -1}};   // (tile, first step)
  for (size_t qi = 0; qi < reach.size() && reach.size() < 200; ++qi)
    for (int d = 0; d < 4; ++d) {
      const int n = board_.Step(reach[qi].first, d);
      if (!allowed(n)) continue;
      bool known = false;
      for (const auto& [t, f] : reach) if (t == n) { known = true; break; }
      if (!known) reach.push_back({n, qi == 0 ? d : reach[qi].second});
    }
  // stations: those at Chebyshev kScreenNear..kScreenFar from her head, nearest the threat point
  auto score = [&](int t) { int x, y; disp(queen, t, &x, &y); return (x - tx) * (x - tx) + (y - ty) * (y - ty); };
  std::vector<std::pair<double, int>> stations;   // (score, index in reach)
  for (size_t i = 0; i < reach.size(); ++i) {
    const int c = cheb(reach[i].first, queen);
    if (c >= kScreenNear && c <= kScreenFar) stations.push_back({score(reach[i].first), int(i)});
  }
  if (stations.empty()) return false;
  SC(4);
  std::sort(stations.begin(), stations.end());
  int pick = stations[0].second;
  if (rank == 1) {   // the second screen stands apart from the first
    pick = -1;
    for (const auto& [sc, i] : stations)
      if (cheb(reach[i].first, reach[stations[0].second].first) >= 2) { pick = i; break; }
    if (pick < 0) return false;
  }
  int pref = reach[pick].second;
  if (pick == 0) {   // on the station already: a dragon must move, so take the best station next to it
    double best_score = 1e18;
    for (int d = 0; d < 4; ++d) {
      const int to = board_.Step(head_, d);
      if (!allowed(to)) continue;
      const int c = cheb(to, queen);
      const double sc = score(to) + (c > kScreenFar ? 1000.0 : 0.0);
      if (sc < best_score) { best_score = sc; pref = d; }
    }
  }
  if (pref < 0) return false;
  SC(5);
  const bot::Action a = SafeStep(pref, false);
  if (a.kind != bot::Action::kMove || a.resign || a.steps.size() != 1 || a.steps[0] != pref) return false;
  SC(6);
#ifdef SCREEN_DBG
  std::fprintf(stderr, "SCREEN r%d team%d id%d len%d rank%d qlen%d qd%d threat%d\n", round_, int(spawn_.team), int(spawn_.id),
               length_, rank, qlen, cheb(head_, queen), best == 1 << 30 ? -1 : best);
#endif
  *out = a;
  reason_ = "screen: hold by her";
  return true;
}

}  // namespace gm2
