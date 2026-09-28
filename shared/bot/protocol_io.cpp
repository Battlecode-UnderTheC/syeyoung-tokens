#include "protocol_io.hpp"

#include <cstdio>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace bot {

namespace {

// the next line with its comment removed that has any text
bool DataLine(std::string& line) {
  while (std::getline(std::cin, line)) {
    if (const size_t c = line.find('#'); c != std::string::npos) line.erase(c);
    if (line.find_first_not_of(" \t\r") != std::string::npos) return true;
  }
  return false;
}

std::string FirstWord(const std::string& line) {
  std::istringstream s(line);
  std::string w;
  s >> w;
  return w;
}

int CountAfter(const std::string& line) {
  std::istringstream s(line);
  std::string w;
  int n = 0;
  s >> w >> n;
  return n;
}

}  // namespace

std::string ReplyText(const Action& a) {
  std::string out;
  if (!a.resign) {
    if (a.kind == Action::kSplit) {
      out += "SPLIT " + std::to_string(a.split) + "\n";
    } else if (!a.steps.empty()) {
      out += "MOVE ";
      for (const int d : a.steps) out += dragons::kDirChar[d];
      out += "\n";
    }
  }
  for (int d = 0; d < 4; ++d)
    if (a.sonar[d]) out += std::string("SONAR ") + dragons::kDirChar[d] + " " + std::to_string(*a.sonar[d]) + "\n";
  return out + "PROTOCOL 3\nENDTURN\n";
}

bool ReadRoundBlock(std::string& block) {
  std::string line;
  if (!DataLine(line) || FirstWord(line) == "ENDGAME") return false;
  // the block in its fixed structure: ROUND, DIR, LENGTH, UNIT_COUNT, NUM_MSGS k and k
  // words, an optional ECHOES line, 49 tiles, DRAGON_BODIES n and n bodies, 8 + 7 edge rows
  block = line + "\n";
  auto take = [&](int n) {
    for (int i = 0; i < n; ++i) {
      if (!DataLine(line)) throw std::runtime_error("round block cut short");
      block += line + "\n";
    }
  };
  take(4);
  take(CountAfter(line));
  take(1);
  take(FirstWord(line) == "ECHOES" ? rl::kWindowTiles : rl::kWindowTiles - 1);
  take(1);
  take(CountAfter(line));
  take((rl::kWindow + 1) + rl::kWindow);
  return true;
}

int Run(AgentFactory make) {
  std::setvbuf(stdout, nullptr, _IOFBF, 1 << 16);
  try {
    std::string line, init;
    for (int k = 0; k < 4; ++k) {                  // ID, TEAM, MAP, UNIT_LIMIT
      if (!DataLine(line)) return 0;
      init += line + "\n";
    }
    const rl::Spawn spawn = rl::ParseSpawn(init);
    std::unique_ptr<Agent> agent = make(spawn);
    std::string block;
    while (ReadRoundBlock(block)) {
      const std::string reply = ReplyText(agent->Turn(rl::ParseRound(spawn, block)));
      std::fwrite(reply.data(), 1, reply.size(), stdout);
      std::fflush(stdout);
    }
  } catch (const std::exception& e) {
    std::fprintf(stderr, "bot error: %s\n", e.what());
    return 1;
  }
  return 0;
}

}  // namespace bot
