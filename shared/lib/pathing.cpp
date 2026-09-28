#include "pathing.hpp"

#include <algorithm>
#include <deque>

namespace strat {

int32_t Paths::Nearest(const std::function<bool(int32_t)>& goal) const {
  int32_t best = -1;
  for (int32_t t = 0; t < static_cast<int32_t>(dist.size()); ++t)
    if (dist[t] > 0 && goal(t) && (best < 0 || dist[t] < dist[best])) best = t;
  return best;
}

Paths Search(const BoardMemory& board, int32_t from, const std::function<bool(int32_t)>& enter, int32_t max_dist) {
  Paths p{std::vector<int32_t>(board.size(), -1), std::vector<int8_t>(board.size(), -1)};
  std::deque<int32_t> queue{from};
  p.dist[from] = 0;
  while (!queue.empty()) {
    const int32_t t = queue.front();
    queue.pop_front();
    if (p.dist[t] >= max_dist) continue;
    for (int d = 0; d < 4; ++d) {
      const int32_t n = board.Step(t, d);
      if (n < 0 || p.dist[n] >= 0 || !enter(n)) continue;
      p.dist[n] = p.dist[t] + 1;
      p.first[n] = static_cast<int8_t>(t == from ? d : p.first[t]);
      queue.push_back(n);
    }
  }
  return p;
}

int32_t RoomAround(const BoardMemory& board, int32_t from, const std::function<bool(int32_t)>& enter,
                   int32_t limit, std::vector<int32_t>* region) {
  std::vector<uint8_t> seen(board.size(), 0);
  std::deque<int32_t> queue{from};
  seen[from] = 1;
  int32_t count = 1;
  if (region) region->assign(1, from);
  while (!queue.empty() && count < limit) {
    const int32_t t = queue.front();
    queue.pop_front();
    for (int d = 0; d < 4; ++d) {
      const int32_t n = board.Step(t, d);
      if (n == kUnknown) return limit;               // unexplored: assume a way out
      if (n < 0 || seen[n] || !enter(n)) continue;
      seen[n] = 1;
      ++count;
      if (region) region->push_back(n);
      queue.push_back(n);
    }
  }
  return std::min(count, limit);
}

}  // namespace strat
