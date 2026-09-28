// A fixed reference for the ratings: the shortest free path to the nearest pearl in view,
// else straight on (else any free step). Never splits, never remembers.
#include <deque>
#include <random>

#include "plugin.hpp"
#include "window.hpp"

namespace {

class GreedyAgent : public bot::Agent {
 public:
  explicit GreedyAgent(const rl::Spawn& s) : rng_(static_cast<uint32_t>(s.id * 7919 + s.team)) {}

  bot::Action Turn(const rl::Observation& o) override {
    // breadth-first over free window tiles, remembering each tile's first step
    int first[rl::kWindowTiles];
    std::fill(std::begin(first), std::end(first), -1);
    std::deque<int> queue;
    for (int d = 0; d < 4; ++d)
      if (const int k = win::Step(o, win::kCenter, d); k >= 0 && !win::Occupied(o, k) && first[k] < 0) {
        first[k] = d;
        queue.push_back(k);
      }
    for (; !queue.empty(); queue.pop_front()) {
      const int k = queue.front();
      if (o.pearl[k]) {
        why_ = "toward pearl";
        return bot::Action::Move(first[k]);
      }
      for (int d = 0; d < 4; ++d)
        if (const int n = win::Step(o, k, d); n >= 0 && n != win::kCenter && first[n] < 0 && !win::Occupied(o, n)) {
          first[n] = first[k];
          queue.push_back(n);
        }
    }
    if (const int k = win::Step(o, win::kCenter, o.facing); k >= 0 && !win::Occupied(o, k)) {
      why_ = "straight on";
      return bot::Action::Move(o.facing);
    }
    int free[4], n = 0;
    for (int d = 0; d < 4; ++d)
      if (const int k = win::Step(o, win::kCenter, d); k >= 0 && !win::Occupied(o, k)) free[n++] = d;
    why_ = n ? "turn" : "stuck";
    if (n == 0) return bot::Action::Move(o.facing);
    return bot::Action::Move(free[std::uniform_int_distribution<int>(0, n - 1)(rng_)]);
  }
  const char* reason() const override { return why_; }

 private:
  std::mt19937 rng_;
  const char* why_ = "";
};

}  // namespace

LEAGUE_VARIANT(GreedyAgent)
