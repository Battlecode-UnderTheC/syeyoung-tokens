// The unswbc bot's main for any variant: the protocol loop around its agent.
#include "plugin.hpp"
#include "protocol_io.hpp"

extern "C" bot::Agent* league_make_agent(const rl::Spawn* spawn);

namespace {

std::unique_ptr<bot::Agent> Make(const rl::Spawn& spawn) { return std::unique_ptr<bot::Agent>(league_make_agent(&spawn)); }

}  // namespace

int main() { return bot::Run(Make); }
