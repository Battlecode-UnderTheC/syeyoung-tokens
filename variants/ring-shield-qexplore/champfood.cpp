// g42-3 (ring-champfood-1): the late champion grows from pearls (see kLastFeedFrom in strategy.hpp).
// In late concentration the champion takes a free multi-step move (at most ceil(L/4) steps, only
// over tiles in view) chosen by a small beam: pearls eaten on the way, plus the food it can reach
// next from where the move ends (pearls, and beds by their exact countdowns via TileValue), summed
// with a discount by arrival time, so a rich field beats one lone pearl. Enemy reach, no escape
// and no liberties are penalised like the late queen's harvest.
#include <algorithm>
#include <cmath>
#include <cstdio>

#include "strategy.hpp"

namespace gm2 {

bot::Action StrategyAgent::ChampionHarvest() {
#ifdef CF_DBG
  std::fprintf(stderr, "CF r%d id%d len%d body%d\n", round_, spawn_.id, length_, int(body_.size()));
#endif
  // a partly known body (a split child, or a champion that took over late) is fine: the sprint
  // steps only on tiles in view, and our own segments in view that memory lacks are refused below
  if (body_.empty() || length_ < 2) return {};
  struct Route {
    std::vector<int32_t> body;
    std::vector<int> steps;
    std::vector<int32_t> eaten;
    int risk = 0;
    double value = 0, food = 0;
  };
  Route root;
  root.body.assign(body_.begin(), body_.end());
  std::vector<Route> beam{root};
  Route best;
  double best_value = -1e30;
  const int free_steps = std::min<int>((length_ + 3) / 4, kChampMaxSteps);
  const int64_t work_start = work_;
  q_mine_.Begin(board_.size());
  for (const int32_t t : body_) if (t >= 0) q_mine_.Set(t);
  for (int depth = 0; depth < free_steps; ++depth) {
    std::vector<Route> next;
    bool over = false;
    for (const auto& route : beam) {
      if (depth > 0 && work_ - work_start > kChampWorkCap) { over = true; break; }
      for (int d = 0; d < 4; ++d) {
        const int32_t t = board_.Step(route.body.front(), d);
        // only tiles we see now (an unseen tile may hold a body), never our own body, tail included
        if (t < 0 || !board_.Current(t) ||
            std::find(route.body.begin(), route.body.end(), t) != route.body.end()) continue;
        const auto& cell = board_.at(t);
        if (cell.occupant >= 0 && cell.occupant != spawn_.id) continue;
        if (cell.occupant == spawn_.id && !q_mine_.Has(t)) continue;
        work_ += 16 + static_cast<int>(route.body.size()) / 4;
        Route c = route;
        c.steps.push_back(d);
        c.body.insert(c.body.begin(), t);
        if (cell.pearl && std::find(c.eaten.begin(), c.eaten.end(), t) == c.eaten.end()) {
          c.eaten.push_back(t);
        } else if (static_cast<int>(c.body.size()) > length_ + static_cast<int>(c.eaten.size())) {
          c.body.pop_back();
        }
        c.risk = std::max(c.risk, QueenRisk(t));
        int room = 0;
        const int escape = EscapeLevel(c.body, std::max(18, 2 * static_cast<int>(c.body.size())), &room);
        q_body_.Begin(board_.size());
        for (const int32_t b : c.body) if (b >= 0) q_body_.Set(b);
        int liberties = 0;
        for (int dir = 0; dir < 4; ++dir) {
          const int32_t n = board_.Step(t, dir);
          if (n >= 0 && !q_body_.Has(n) &&
              !(board_.Current(n) && board_.at(n).occupant >= 0 && board_.at(n).occupant != spawn_.id)) ++liberties;
        }
        // the food reachable from here: a breadth-first pass (12 steps) over free tiles, each
        // valued tile counted with weight 0.8^(rounds until we could eat it)
        double food = 0;
        q_dist_.Begin(board_.size());
        q_queue_.assign(1, t);
        q_dist_.Set(t, 0);
        for (size_t qi = 0; qi < q_queue_.size(); ++qi) {
          const int32_t u = q_queue_[qi], du = q_dist_.Get(u);
          ++work_;
          if (qi > 0) {
            int ready;
            const bool occupied = q_mine_.Has(u) || (board_.Current(u) && board_.at(u).occupant >= 0);
            const bool taken = std::find(c.eaten.begin(), c.eaten.end(), u) != c.eaten.end();
            const double v = taken ? 0.0 : TileValue(u, du, &ready, occupied);
            if (v > 0) food += v * std::pow(0.8, std::max(ready, du));
          }
          if (du >= 12) continue;
          for (int dir = 0; dir < 4; ++dir) {
            const int32_t n = board_.Step(u, dir);
            if (n < 0 || q_dist_.Has(n) || q_body_.Has(n)) continue;
            if (board_.Current(n) && board_.at(n).occupant >= 0 && board_.at(n).occupant != spawn_.id) continue;
            q_dist_.Set(n, du + 1);
            q_queue_.push_back(n);
          }
        }
        c.food = food;
        c.value = 12.0 * c.eaten.size() + 6.0 * food + 0.03 * room + 0.3 * liberties - 0.03 * c.steps.size() -
                  1000.0 * QueenRisk(t) - 100.0 * c.risk - (escape == 0 ? 1e6 : 0) - (liberties == 0 ? 1e6 : 0);
        if (c.value > best_value) { best_value = c.value; best = c; }
        next.push_back(std::move(c));
      }
    }
    if (over) break;
    std::sort(next.begin(), next.end(), [](const Route& a, const Route& b) { return a.value > b.value; });
    if (next.size() > 6) next.resize(6);
    beam = std::move(next);
    if (beam.empty()) break;
  }
  // nothing to eat in reach, or no safe way: the usual goal walk (it sees the whole board)
#ifdef CF_DBG
  std::fprintf(stderr, "CFB r%d id%d steps%d val%.1f eaten%d food%.2f\n", round_, spawn_.id, int(best.steps.size()), best_value, int(best.eaten.size()), best.food);
#endif
  if (best.steps.empty() || best_value < -1000 || (best.eaten.empty() && best.food <= 0)) return {};
  body_.assign(best.body.begin(), best.body.end());
  bot::Action a;
  a.steps = best.steps;
  return Because("champion: harvest", a);
}

}  // namespace gm2
