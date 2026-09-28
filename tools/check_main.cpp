// The gate a variant passes before it is tagged: self-play on the given maps.
//  1. no exceptions;
//  2. no avoidable deaths: dying on a wall or on its own body while a neighbouring tile
//     in view was free (judged from the game, whatever the variant believes);
//  3. the slowest turn, reported (the judge meters CPU; see DESIGN.md);
//  and, since it is cheap, an audit of the shared BoardMemory against the engine.
//
//   check <variant> <maps dir or pool> [limit]          exit status 0 when every check passed
#include <dlfcn.h>

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "board_memory.hpp"
#include "plugin.hpp"

namespace {

int failures = 0;

void Fail(const std::string& what) {
  if (++failures <= 20) std::printf("FAIL: %s\n", what.c_str());
}

bot::MakeAgentFn Load(const std::string& arg, const std::string& exe) {
  std::string path = arg;
  if (arg.find('/') == std::string::npos) {
    const auto dir = std::filesystem::path(exe).parent_path().parent_path() / "variants";
    for (const char* ext : {".so", ".dylib"})
      if (const auto p = dir / ("lib" + arg + ext); std::filesystem::exists(p)) path = p.string();
  }
  void* h = dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);
  if (!h) throw std::runtime_error("cannot load variant " + arg + ": " + dlerror());
  auto make = reinterpret_cast<bot::MakeAgentFn>(dlsym(h, bot::kMakeAgentSymbol));
  if (!make) throw std::runtime_error(arg + " has no " + bot::kMakeAgentSymbol + " (LEAGUE_VARIANT missing)");
  return make;
}

std::vector<std::pair<std::string, std::string>> ReadMaps(const std::string& path, size_t limit) {
  std::vector<std::pair<std::string, std::string>> maps;
  if (std::filesystem::is_directory(path)) {
    std::vector<std::filesystem::path> files;
    for (const auto& e : std::filesystem::directory_iterator(path))
      if (e.path().extension() == ".map") files.push_back(e.path());
    std::sort(files.begin(), files.end());
    for (const auto& f : files) {
      std::ifstream in(f);
      std::stringstream s;
      s << in.rdbuf();
      maps.emplace_back(f.stem().string(), s.str());
    }
  } else {
    std::ifstream in(path, std::ios::binary);
    size_t n;
    while (maps.size() < limit && in >> n) {
      in.get();
      std::string text(n, '\0');
      in.read(text.data(), static_cast<std::streamsize>(n));
      maps.emplace_back("pool" + std::to_string(maps.size()), std::move(text));
    }
  }
  if (maps.size() > limit) maps.resize(limit);
  return maps;
}

void Apply(dragons::Game& g, const bot::Action& a) {
  dragons::Sonar sonar;
  for (int d = 0; d < 4; ++d)
    if (a.sonar[d]) {
      sonar.mask |= 1 << d;
      sonar.word[d] = *a.sonar[d];
    }
  if (a.resign) g.Act(dragons::kNoAction, nullptr, 0, 0, sonar, 3);
  else if (a.kind == bot::Action::kSplit) g.Act(dragons::kSplit, nullptr, 0, a.split, sonar, 3);
  else {
    std::vector<uint8_t> steps(a.steps.begin(), a.steps.end());
    g.Act(dragons::kMove, steps.data(), static_cast<int>(steps.size()), 0, sonar, 3);
  }
}

// A neighbouring tile of the head that is in view, not kelp, and holds no dragon.
bool HadFreeStep(const dragons::Board& board, const rl::Observation& o) {
  const int32_t w = board.width, head = o.head_y() * w + o.head_x();
  for (int d = 0; d < 4; ++d) {
    const int32_t to = board.step_to[head * 4 + d];
    if (to < 0) continue;
    bool in_view = false, occupied = false;
    for (int k = 0; k < rl::kWindowTiles; ++k)
      if (o.tile_y[k] * w + o.tile_x[k] == to) in_view = true;
    for (const rl::Segment& s : o.segments)
      if (s.y * w + s.x == to) occupied = true;
    if (in_view && !occupied) return true;
  }
  return false;
}

void Check(const std::string& name, const std::string& text, bot::MakeAgentFn make) {
  const auto board = dragons::LoadBoard(text);
  dragons::Game g;
  g.Reset(board);
  std::map<int32_t, std::unique_ptr<bot::Agent>> agents;
  std::map<int32_t, strat::BoardMemory> memories;          // the shared BoardMemory, audited
  int avoidable = 0, audited = 0, memory_errors = 0;
  double slowest_ms = 0;
  while (!g.done()) {
    const int32_t d = g.actor();
    const rl::Spawn spawn = rl::SpawnOf(g, d);
    auto it = agents.find(d);
    if (it == agents.end()) {
      it = agents.emplace(d, std::unique_ptr<bot::Agent>(make(&spawn))).first;
      memories.emplace(d, strat::BoardMemory(spawn.width, spawn.height));
    }
    const rl::Observation o = rl::Observe(g);
    strat::BoardMemory& mem = memories.at(d);
    mem.Update(o);
    for (int32_t t = 0; t < mem.size(); ++t)
      for (int dir = 0; dir < 4; ++dir) {
        const int32_t known = mem.Step(t, dir);
        if (known == strat::kUnknown) continue;
        const int32_t truth = board->step_to[t * 4 + dir];
        ++audited;
        if (known != (truth < 0 ? strat::kBlocked : truth)) ++memory_errors;
      }
    const bool free_step = HadFreeStep(*board, o);
    const auto t0 = std::chrono::steady_clock::now();
    const bot::Action action = it->second->Turn(o);
    slowest_ms = std::max(slowest_ms, std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count());
    g.ClearDeaths();
    Apply(g, action);
    for (const dragons::Death& death : g.deaths()) {
      if (death.id == d && free_step && (death.reason == dragons::kHitWall || death.reason == dragons::kHitSelf) &&
          ++avoidable <= 3)
        std::printf("  %s round %d: dragon %d died on a %s with a free step in view (reason: \"%s\")\n", name.c_str(),
                    o.round, d, death.reason == dragons::kHitWall ? "wall" : "own body", it->second->reason());
      agents.erase(death.id);
      memories.erase(death.id);
    }
  }
  if (avoidable) Fail(name + ": " + std::to_string(avoidable) + " avoidable wall/self deaths");
  if (memory_errors) Fail(name + ": BoardMemory wrong on " + std::to_string(memory_errors) + " steps (shared/lib bug)");
  std::printf("%-20s %3d rounds, slowest turn %.1f ms, %d avoidable deaths, %d memory steps audited\n", name.c_str(),
              g.round(), slowest_ms, avoidable, audited);
}

}  // namespace

int main(int argc, char** argv) {
  if (argc < 3) {
    std::fprintf(stderr, "usage: %s <variant> <maps dir or pool> [limit]\n", argv[0]);
    return 2;
  }
  try {
    const bot::MakeAgentFn make = Load(argv[1], argv[0]);
    for (const auto& [name, text] : ReadMaps(argv[2], argc > 3 ? std::stoul(argv[3]) : 20)) Check(name, text, make);
  } catch (const std::exception& e) {
    Fail(std::string("exception: ") + e.what());
  }
  std::printf(failures ? "%d FAILURES\n" : "all checks passed\n", failures);
  return failures ? 1 : 0;
}
