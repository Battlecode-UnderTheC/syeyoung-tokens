// g47-2 (ring-sprint-escort): escorts for the late queen.
//
// Observed (g46-4, ring-sprint-24/27 arena games): of 18 late deaths of queens of length 5+, in 17
// the killer was 4-5 tiles off, outside her 7x7 view, and her own move brought her into its reach;
// in 17 of 18 no ally head was within 3 of her, so nobody saw the killer and nobody could relay it.
// The top teams' queens travel with about two escorts (docs/analysis-2026-10-07/STYLE.md).
//
// From kEscortFrom (r400: the late phase before her r430 ring exit, so escorts are in place when she
// starts to move), the kEscorts short workers (length kEscortMinLen..kEscortMaxLen: cheap to lose,
// one-for-one trades) nearest to her in our view, within kEscortRecruit steps:
//  - trade: an enemy head seen this turn that could reach her head (its rules' reach ceil(L/4)+L-2,
//    floor 3, cap 8, unbounded-as-8 when its body leaves view, plus 1 because she moves first next
//    round) and that we can reach this turn (our own free + paid steps through free tiles in view)
//    is hit head-on: both die, she lives;
//  - station: otherwise walk to the tile kEscortNear..kEscortFar steps from her head whose 7x7 view
//    covers the most ground she can't see (tiles reachable from her head within kEscortBlind steps,
//    more than 3 from her in Chebyshev distance, and not already in view of another ally head),
//    with enemy heads seen in the last few rounds weighted up. Escorts don't step on pearls within
//    kLineReach of her (her feed lines) and never on tiles beside her head.
// Escorts are not counted as "first in line" by conveyor feeders, and don't lay feed lines.
#include "strategy.hpp"

#include <algorithm>
#include <cstdio>
#include <string>

namespace gm2 {

int32_t StrategyAgent::QueenHeadNow(bool* in_view) const {
  *in_view = false;
  if (const auto it = seen_.find(queen_id_); it != seen_.end() && it->second.team == spawn_.team && it->second.round == round_ &&
      it->second.head >= 0 && board_.Current(it->second.head) && board_.at(it->second.head).occupant == queen_id_ &&
      board_.at(it->second.head).occupant_head) {
    *in_view = true;
    return it->second.head;
  }
  if (const auto r = reports_.find(queen_id_); r != reports_.end() && r->second.length > 0 && r->second.tile >= 0 &&
      round_ - r->second.round <= kRaceFresh / 3 && !board_.Current(r->second.tile))
    return r->second.tile;
  return -1;
}

bool StrategyAgent::EscortRanked(int32_t id) const {
  return escort_round_ == round_ && std::find(escort_ids_.begin(), escort_ids_.end(), id) != escort_ids_.end();
}

bot::Action StrategyAgent::Escort() {
  escort_ids_.clear();
  escort_round_ = round_;
  if (!ESCORT_MODE || IsQueen() || round_ < kEscortFrom || round_ >= 499 || units_ < 4) return {};
  bool in_view = false;
  const int32_t q = QueenHeadNow(&in_view);
  if (q < 0) return {};
  // iteration 2: escorts cost the queen her nearest feeders (duel v ring-sprint-21: 5.6 v 7.6 feed
  // lines laid beside her a game from r430, she ate 13.6 v 18.5 pearls), so the role is only taken
  // while there is something to guard against: an enemy head we saw within kEscortThreatRounds
  // rounds within kEscortThreatDist of her head (the reach of a 12-long lunge, ceil(12/4)+12-2 = 13,
  // less the 3 rounds of walking such a sighting is allowed to age). Without one, we feed as before.
  bool threat = false;
  for (const auto& [id, w] : seen_)
    if (w.team != spawn_.team && w.head >= 0 && w.length > 0 && round_ - w.round <= kEscortThreatRounds &&
        Torus(w.head, q) <= kEscortThreatDist) { threat = true; break; }
  if (!threat) return {};
  auto eligible = [](int32_t len) { return len >= kEscortMinLen && len <= kEscortMaxLen; };
  // iteration 3: an escort that eats on its post keeps it up to kEscortKeepLen (duel game: a 5-long
  // escort ate one pearl 4 tiles from her, dropped the role and left; she died to the 11-long head
  // it had been shadowing). 8 is the last length with the same 2 free steps as 5.
  const bool was_escort = escort_last_ == round_ - 1;
  // rank the eligible heads we see (and us) by their distance to her; an escort of last turn keeps
  // its place against a newcomer up to one step closer (no swapping every round)
  std::vector<std::pair<int32_t, int32_t>> ranked;
  for (const auto& [id, w] : seen_)
    if (w.team == spawn_.team && w.round == round_ && w.head >= 0 && id > 1 && id != spawn_.id && eligible(w.length)) {
      const int32_t d = Torus(w.head, q);
      if (d <= kEscortRecruit) ranked.push_back({2 * d, id});
    }
  const int32_t mine = Torus(head_, q);
  if ((eligible(length_) || (was_escort && length_ >= kEscortMinLen && length_ <= kEscortKeepLen)) && mine <= kEscortRecruit && int(body_.size()) == length_)
    ranked.push_back({2 * mine - (was_escort ? 3 : 0), spawn_.id});
  std::sort(ranked.begin(), ranked.end());
  for (size_t i = 0; i < ranked.size() && int(i) < kEscorts; ++i) escort_ids_.push_back(ranked[i].second);
  if (std::find(escort_ids_.begin(), escort_ids_.end(), spawn_.id) == escort_ids_.end()) return {};
  escort_last_ = round_;
  auto cheb = [&](int32_t a, int32_t b) {
    int32_t dx = std::abs(board_.X(a) - board_.X(b)), dy = std::abs(board_.Y(a) - board_.Y(b));
    return std::max(std::min(dx, board_.width() - dx), std::min(dy, board_.height() - dy));
  };

  // 1. trade into an enemy head that could reach her
  {
    const int32_t free = (length_ + 3) / 4, reach = free + std::max(0, length_ - 2);
    int32_t best_dq = 1 << 20;
    std::vector<int> best_steps;
    for (const auto& [id, w] : seen_) {
      if (w.team == spawn_.team || w.round != round_ || w.head < 0 || !board_.Current(w.head) ||
          !board_.at(w.head).occupant_head || board_.at(w.head).occupant != id)
        continue;
      const int32_t re = w.partial ? 8 : std::min(8, std::max(3, (w.length + 3) / 4 + w.length - 2));
      const int32_t dq = Torus(w.head, q);
      if (dq > re + 1 || dq >= best_dq) continue;
      // our path to its head through free tiles in view
      const Paths p = Search(board_, head_, [&](int32_t t) {
        return t == w.head || (board_.Current(t) && !Occupied(t));
      }, reach);
      if (p.dist[w.head] <= 0 || p.dist[w.head] > reach) continue;
      std::vector<int> steps;
      for (int32_t t = w.head; t != head_;) {
        // walk back: the neighbour one closer to us
        int32_t prev = -1;
        int dir = -1;
        for (int d = 0; d < 4 && prev < 0; ++d) {
          const int32_t n = board_.Step(t, d);
          if (n >= 0 && p.dist[n] == p.dist[t] - 1 && board_.Step(n, (d + 2) % 4) == t) { prev = n; dir = (d + 2) % 4; }
        }
        if (prev < 0) { steps.clear(); break; }
        steps.push_back(dir);
        t = prev;
      }
      if (steps.empty()) continue;
      std::reverse(steps.begin(), steps.end());
      best_dq = dq;
      best_steps = steps;
    }
    if (!best_steps.empty()) {
      rescue_committed_ = true;
#ifdef ESCORT_DBG
      std::fprintf(stderr, "ESCT r%d team%d id%d len%d steps%d dq%d\n", round_, spawn_.team, spawn_.id, length_, int(best_steps.size()), best_dq);
#endif
      return Because("escort: trade", bot::Action{bot::Action::kMove, best_steps, 0, false, {}});
    }
  }

  // 2. station on the side of her blind ground
  const Paths qp = Search(board_, q, [&](int32_t t) { return !Occupied(t) || t == head_; }, kEscortBlind);
  const bool ring_fresh = round_ - ring_seen_ <= 12 && !ring_.empty();
  const Paths mp = Search(board_, head_, [&](int32_t t) {
    if (Occupied(t)) return false;
    if (ring_fresh && std::find(ring_.begin(), ring_.end(), t) != ring_.end()) return false;
    return !(board_.at(t).pearl && Torus(t, q) <= kLineReach);   // her feed lines are hers
  }, kEscortRecruit + kEscortFar);
  // weights of the blind tiles around her
  std::vector<std::pair<int32_t, int32_t>> blind;   // tile, weight
  const int32_t qx = board_.X(q), qy = board_.Y(q);
  for (int dy = -kEscortBlind; dy <= kEscortBlind; ++dy)
    for (int dx = -kEscortBlind; dx <= kEscortBlind; ++dx) {
      if (std::max(std::abs(dx), std::abs(dy)) <= 3) continue;
      const int32_t u = board_.Tile(qx + dx, qy + dy);
      if (qp.dist[u] <= 0) continue;
      bool covered = false;
      for (const auto& [id, w] : seen_)
        if (w.team == spawn_.team && w.round == round_ && w.head >= 0 && id != spawn_.id && cheb(w.head, u) <= 3) { covered = true; break; }
      if (!covered) blind.push_back({u, 1});
    }
  for (const auto& [id, w] : seen_)
    if (w.team != spawn_.team && w.head >= 0 && round_ - w.round <= 5 && Torus(w.head, q) <= 2 * kEscortBlind)
      blind.push_back({w.head, 6});
  int32_t best = -1;
  int32_t best_score = -(1 << 20);
  for (int dy = -kEscortFar; dy <= kEscortFar; ++dy)
    for (int dx = -kEscortFar; dx <= kEscortFar; ++dx) {
      const int32_t t = board_.Tile(qx + dx, qy + dy);
      if (t == head_ || qp.dist[t] < kEscortNear || qp.dist[t] > kEscortFar || mp.dist[t] <= 0) continue;
      int32_t s = 0;
      for (const auto& [u, wt] : blind) if (cheb(t, u) <= 3) s += wt;
      s = 2 * s - mp.dist[t];
      if (s > best_score) { best_score = s; best = t; }
    }
  if (best < 0) return {};
  cur_goal_ = best;
  bot::Action a = SafeStep(mp.first[best], false);
  cur_goal_ = -1;
  if (std::string(reason_) != "toward goal") return {};
  walking_to_ally_ = true;   // plugin.cpp keeps this step (not the late lookahead's)
#ifdef ESCORT_DBG
  std::fprintf(stderr, "ESCS r%d team%d id%d len%d qd%d goal_qd%d goal_md%d view%d\n", round_, spawn_.team, spawn_.id, length_, mine, qp.dist[best], mp.dist[best], int(in_view));
#endif
  reason_ = "escort: station";
  return a;
}

}  // namespace gm2
