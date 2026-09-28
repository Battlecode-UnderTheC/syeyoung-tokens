// What every bot is: one dragon's turn in, one action out. The protocol loop
// (protocol_io.hpp), the simulator (tools/sim_main.cpp) and the bot's main only
// ever see this interface; each variant implements it.
#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "observation.hpp"

namespace bot {

// One reply. A MOVE is 1..8 directions (0 N, 1 E, 2 S, 3 W); every step after
// the first costs a tail segment. `resign`: no action at all, which the engine
// punishes with death ("no valid action"): how a dragon gives its body away.
struct Action {
  enum Kind : uint8_t { kMove, kSplit } kind = kMove;
  std::vector<int> steps;
  int split = 0;
  bool resign = false;
  std::array<std::optional<uint64_t>, 4> sonar{};   // a word pinged N, E, S, W

  static Action Move(int dir) { return Action{kMove, {dir}, 0, false, {}}; }
  static Action Split(int n) { return Action{kSplit, {}, n, false, {}}; }
  static Action Resign() { return Action{kMove, {}, 0, true, {}}; }
};

// The reply text for `a`, ending the turn (PROTOCOL 3 keeps the ECHOES line on).
std::string ReplyText(const Action& a);

class Agent {
 public:
  virtual ~Agent() = default;
  virtual Action Turn(const rl::Observation& o) = 0;
  // Why the last action was taken, in a few words (the simulator tallies these; "" if unsaid).
  virtual const char* reason() const { return ""; }
};

// Makes the agent for a dragon when it spawns.
using AgentFactory = std::unique_ptr<Agent> (*)(const rl::Spawn& spawn);

}  // namespace bot
