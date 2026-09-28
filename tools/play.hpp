// Playing variants against each other in the native engine: the pieces sim, match and
// check share.
#pragma once

#include <array>
#include <cstdint>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "plugin.hpp"

namespace tools {

struct Variant {
  std::string name;
  bot::MakeAgentFn make = nullptr;
};

// A variant by name (build/variants/lib<name>.so next to the running tool) or by path.
Variant Load(const std::string& name_or_path, const std::string& exe);

// Maps from a directory of .map files, or a pool file of `<bytes>\n<text>` records.
std::vector<std::pair<std::string, std::string>> ReadMaps(const std::string& path, size_t limit = 1u << 30);

// Plays one reply.
void Apply(dragons::Game& g, const bot::Action& a);

inline const char* kCauses[5] = {"wall", "self", "body", "head-to-head", "no action"};

struct Outcome {
  int8_t winner = -1;                              // -1 draw, 0 team A, 1 team B
  int32_t rounds = 0;
  std::array<int32_t, 2> longest{}, total{};       // at the end, by team
  std::array<std::array<int32_t, 5>, 2> deaths{};  // by team and cause (kCauses)
  std::array<std::array<int32_t, 2>, 2> lunged{};  // by team: dragons killed head-to-head by an enemy's
  std::array<std::array<int32_t, 2>, 2> lunged_len{};  // multi-step move, [shorter than 10, 10+], and their length
  std::map<std::string, int64_t> reasons[2];       // by team: why its dragons acted
  std::map<std::string, int64_t> fatal[2];         // ... and the reasons behind turns that killed them
  std::array<std::vector<float>, 2> turn_us;       // by team: every Turn()'s wall time, microseconds
};

// One match: sides[0] plays team A, sides[1] team B. trace: print each round's counts and every death.
Outcome Play(const std::string& map_text, const std::array<const Variant*, 2>& sides, bool trace = false);

}  // namespace tools
