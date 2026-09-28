// A fixed reference for the ratings: a uniformly random step onto free ground.
#include <random>

#include "plugin.hpp"
#include "window.hpp"

namespace {

class RandomAgent : public bot::Agent {
 public:
  explicit RandomAgent(const rl::Spawn& s) : rng_(static_cast<uint32_t>(s.id * 7919 + s.team)) {}

  bot::Action Turn(const rl::Observation& o) override {
    int free[4], n = 0;
    for (int d = 0; d < 4; ++d)
      if (const int k = win::Step(o, win::kCenter, d); k >= 0 && !win::Occupied(o, k)) free[n++] = d;
    if (n == 0) return bot::Action::Move(o.facing);
    return bot::Action::Move(free[std::uniform_int_distribution<int>(0, n - 1)(rng_)]);
  }

 private:
  std::mt19937 rng_;
};

}  // namespace

LEAGUE_VARIANT(RandomAgent)
