#include "play.hpp"

#include <dlfcn.h>

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>
#include <stdexcept>

namespace tools {

Variant Load(const std::string& arg, const std::string& exe) {
  std::string path = arg;
  if (arg.find('/') == std::string::npos) {                 // a name: the build's plugin
    const auto dir = std::filesystem::path(exe).parent_path().parent_path() / "variants";
    for (const char* ext : {".so", ".dylib"})
      if (const auto p = dir / ("lib" + arg + ext); std::filesystem::exists(p)) path = p.string();
  }
  void* h = dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);
  if (!h) throw std::runtime_error("cannot load variant " + arg + ": " + dlerror());
  auto make = reinterpret_cast<bot::MakeAgentFn>(dlsym(h, bot::kMakeAgentSymbol));
  if (!make) throw std::runtime_error(arg + " has no " + bot::kMakeAgentSymbol + " (LEAGUE_VARIANT missing)");
  std::string name = std::filesystem::path(arg).stem().string();
  if (arg.find('/') != std::string::npos && name.rfind("lib", 0) == 0) name = name.substr(3);
  return {name, make};
}

std::vector<std::pair<std::string, std::string>> ReadMaps(const std::string& path, size_t limit) {
  std::vector<std::pair<std::string, std::string>> maps;   // name, text
  auto read_file = [](const std::filesystem::path& f) {
    std::ifstream in(f);
    std::stringstream s;
    s << in.rdbuf();
    return s.str();
  };
  if (std::filesystem::is_directory(path)) {
    std::vector<std::filesystem::path> files;
    for (const auto& e : std::filesystem::directory_iterator(path))
      if (e.path().extension() == ".map") files.push_back(e.path());
    std::sort(files.begin(), files.end());
    for (const auto& f : files) maps.emplace_back(f.stem().string(), read_file(f));
  } else if (std::filesystem::path(path).extension() == ".map") {
    maps.emplace_back(std::filesystem::path(path).stem().string(), read_file(path));
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
  if (maps.empty()) throw std::runtime_error("no maps in " + path);
  return maps;
}

void Apply(dragons::Game& g, const bot::Action& a) {
  dragons::Sonar sonar;
  for (int d = 0; d < 4; ++d)
    if (a.sonar[d]) {
      sonar.mask |= 1 << d;
      sonar.word[d] = *a.sonar[d];
    }
  if (a.resign) {
    g.Act(dragons::kNoAction, nullptr, 0, 0, sonar, 3);
  } else if (a.kind == bot::Action::kSplit) {
    g.Act(dragons::kSplit, nullptr, 0, a.split, sonar, 3);
  } else {
    std::vector<uint8_t> steps(a.steps.begin(), a.steps.end());
    g.Act(dragons::kMove, steps.data(), static_cast<int>(steps.size()), 0, sonar, 3);
  }
}

Outcome Play(const std::string& map_text, const std::array<const Variant*, 2>& sides, bool trace) {
  dragons::Game g;
  g.Reset(dragons::LoadBoard(map_text));
  std::map<int32_t, std::unique_ptr<bot::Agent>> live;
  Outcome out;
  int32_t last_round = -1;
  while (!g.done()) {
    for (int s = 0; s < 2; ++s)
      if (g.round() == (s ? 100 : 50) && !out.snapped[s]) {
        out.snapped[s] = true;
        for (int t = 0; t < 2; ++t)
          for (const auto& dr : g.dragons())
            if (dr.alive && dr.team == t) {
              ++out.snaps[s][t][0];
              out.snaps[s][t][1] = std::max(out.snaps[s][t][1], dr.length);
              out.snaps[s][t][2] += dr.length;
            }
      }
    if (trace && g.round() != last_round) {
      last_round = g.round();
      int32_t n[2] = {0, 0}, len[2] = {0, 0};
      for (const auto& dr : g.dragons())
        if (dr.alive) {
          ++n[dr.team];
          len[dr.team] += dr.length;
        }
      std::printf("round %3d: A %2d dragons (length %3d) | B %2d dragons (length %3d)\n", last_round, n[0], len[0], n[1],
                  len[1]);
    }
    const int32_t d = g.actor();
    const int team = g.dragons()[d].team;
    auto it = live.find(d);
    if (it == live.end()) {
      const rl::Spawn spawn = rl::SpawnOf(g, d);
      it = live.emplace(d, std::unique_ptr<bot::Agent>(sides[team]->make(&spawn))).first;
    }
    const rl::Observation o = rl::Observe(g);
    const auto t0 = std::chrono::steady_clock::now();
    const bot::Action action = it->second->Turn(o);
    out.turn_us[team].push_back(std::chrono::duration<float, std::micro>(std::chrono::steady_clock::now() - t0).count());
    std::vector<int32_t> before;  // lengths before the move, to price what a lunge kills
    const bool multi = !action.resign && action.kind != bot::Action::kSplit && action.steps.size() > 1;
    if (multi)
      for (const auto& dr : g.dragons()) before.push_back(dr.length);
    g.ClearDeaths();
    Apply(g, action);
    const std::string why = it->second->reason();
    if (!why.empty()) ++out.reasons[team][why];
    for (const dragons::Death& death : g.deaths()) {
      if (trace)
        std::printf("    %c dragon %d died (%s) on %s turn%s%s\n", death.team ? 'B' : 'A', death.id,
                    kCauses[death.reason], death.id == d ? "its own" : "another dragon's",
                    why.empty() ? "" : ", acting: ", why.c_str());
      ++out.deaths[death.team][death.reason];
      if (multi && death.team != team && death.reason == 3 && death.id < static_cast<int32_t>(before.size())) {
        const int32_t len = before[death.id];
        ++out.lunged[death.team][len >= 10];
        out.lunged_len[death.team][len >= 10] += len;
      }
      if (death.id == d && !why.empty()) ++out.fatal[team][why];
      live.erase(death.id);
    }
  }
  const dragons::Result r = g.result();
  out.winner = r.winner;
  out.rounds = g.round();
  for (int t = 0; t < 2; ++t) {
    out.longest[t] = r.team[t].longest;
    out.total[t] = r.team[t].total;
  }
  return out;
}

}  // namespace tools
