// A variant's one entry point. Every variant has exactly one line
//
//     LEAGUE_VARIANT(MyAgent)
//
// in one of its .cpp files, where MyAgent is a bot::Agent with a constructor taking
// `const rl::Spawn&`. The same symbol serves both builds: the simulator loads it from
// the variant's plugin (dlopen), and the unswbc bot's main (bot_main.cpp) links it.
#pragma once

#include "agent.hpp"

namespace bot {
using MakeAgentFn = Agent* (*)(const rl::Spawn* spawn);
inline constexpr const char* kMakeAgentSymbol = "league_make_agent";
}  // namespace bot

#define LEAGUE_VARIANT(AgentClass)                                                                    \
  extern "C" __attribute__((visibility("default"))) bot::Agent* league_make_agent(const rl::Spawn* s) { \
    return new AgentClass(*s);                                                                       \
  }
