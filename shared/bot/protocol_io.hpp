// The unswbc protocol on stdin/stdout for one dragon's process: read the spawn
// block, then each round block, ask the agent, print its reply. Every bot's
// main is `return bot::Run(factory);`.
#pragma once

#include <string>

#include "agent.hpp"

namespace bot {

// Reads the next round block (false at ENDGAME or end of input).
bool ReadRoundBlock(std::string& block);

int Run(AgentFactory make);

}  // namespace bot
