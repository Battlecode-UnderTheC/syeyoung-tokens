// Plays two variants against each other in the native engine (the judge's rules), many
// matches at once, every map from both sides.
//
//   sim --a <variant> --b <variant> --maps <dir of .map | pool file> [--limit N] [--threads N]
//   sim --a <variant> --b <variant> --maps <dir> --trace <map name>      one match, verbosely
//
// A variant is a name (loaded from build/variants/lib<name>.so) or a path to a plugin.
// Prints W-D-L from a's side, final length margins, deaths by cause, and the tallies of
// each side's reasons (Agent::reason()) and of the reasons behind its fatal turns.
#include <dlfcn.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include "plugin.hpp"

namespace {

struct Variant {
  std::string name;
  bot::MakeAgentFn make = nullptr;
};

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
  if (name.rfind("lib", 0) == 0 && arg.find('/') != std::string::npos) name = name.substr(3);
  return {name, make};
}

std::vector<std::pair<std::string, std::string>> ReadMaps(const std::string& path, size_t limit) {
  std::vector<std::pair<std::string, std::string>> maps;   // name, text
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

struct Outcome {
  int8_t winner = -1;
  int32_t rounds = 0;
  std::array<int32_t, 2> longest{}, total{};
  std::array<std::array<int32_t, 5>, 2> deaths{};  // by team and cause (wall, self, body, head, no action)
  std::map<std::string, int64_t> reasons[2];       // by team: why its dragons acted
  std::map<std::string, int64_t> fatal[2];         // ... and the reasons behind turns that killed them
};

const char* kCauses[5] = {"wall", "self", "body", "head-to-head", "no action"};

// One match: sides[0] plays team A, sides[1] team B.
Outcome Play(const std::string& map_text, const std::array<const Variant*, 2>& sides, bool trace = false) {
  dragons::Game g;
  g.Reset(dragons::LoadBoard(map_text));
  std::map<int32_t, std::unique_ptr<bot::Agent>> live;
  Outcome out;
  int32_t last_round = -1;
  while (!g.done()) {
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
    g.ClearDeaths();
    Apply(g, it->second->Turn(rl::Observe(g)));
    const std::string why = it->second->reason();
    if (!why.empty()) ++out.reasons[team][why];
    for (const dragons::Death& death : g.deaths()) {
      if (trace)
        std::printf("    %c dragon %d died (%s) on %s turn%s%s\n", death.team ? 'B' : 'A', death.id,
                    kCauses[death.reason], death.id == d ? "its own" : "another dragon's",
                    why.empty() ? "" : ", acting: ", why.c_str());
      ++out.deaths[death.team][death.reason];
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

}  // namespace

int main(int argc, char** argv) {
  std::string a_arg, b_arg, maps_path, trace_map;
  int threads = static_cast<int>(std::thread::hardware_concurrency());
  size_t limit = 1u << 30;
  for (int i = 1; i + 1 < argc; i += 2) {
    const std::string k = argv[i], v = argv[i + 1];
    if (k == "--a") a_arg = v;
    else if (k == "--b") b_arg = v;
    else if (k == "--maps") maps_path = v;
    else if (k == "--threads") threads = std::stoi(v);
    else if (k == "--limit") limit = std::stoul(v);
    else if (k == "--trace") trace_map = v;
    else {
      std::fprintf(stderr, "unknown option %s\n", k.c_str());
      return 2;
    }
  }
  if (a_arg.empty() || b_arg.empty() || maps_path.empty()) {
    std::fprintf(stderr, "usage: %s --a VARIANT --b VARIANT --maps DIR_OR_POOL [--limit N] [--threads N] [--trace MAP]\n",
                 argv[0]);
    return 2;
  }
  try {
    const Variant a = Load(a_arg, argv[0]), b = Load(b_arg, argv[0]);
    const auto maps = ReadMaps(maps_path, limit);
    if (!trace_map.empty()) {
      for (const auto& [name, text] : maps)
        if (name == trace_map) {
          std::printf("== %s: %s is team A, %s team B\n", name.c_str(), a.name.c_str(), b.name.c_str());
          const Outcome o = Play(text, {&a, &b}, true);
          std::printf("== %s after %d rounds\n", o.winner < 0 ? "draw" : (o.winner == 0 ? "A wins" : "B wins"), o.rounds);
          return 0;
        }
      throw std::runtime_error("no map named " + trace_map);
    }
    struct Job {
      size_t map;
      bool a_is_team_a;
    };
    std::vector<Job> jobs;
    for (size_t m = 0; m < maps.size(); ++m)
      for (const bool side : {true, false}) jobs.push_back({m, side});
    std::vector<Outcome> results(jobs.size());
    std::atomic<size_t> next{0};
    const auto t0 = std::chrono::steady_clock::now();
    std::vector<std::thread> pool;
    for (int t = 0; t < threads; ++t)
      pool.emplace_back([&] {
        for (size_t i; (i = next++) < jobs.size();) {
          const Job& j = jobs[i];
          results[i] = Play(maps[j.map].second, j.a_is_team_a ? std::array<const Variant*, 2>{&a, &b}
                                                              : std::array<const Variant*, 2>{&b, &a});
        }
      });
    for (auto& t : pool) t.join();

    int w = 0, d = 0, l = 0;
    double longest = 0, total = 0, rounds = 0;
    std::array<std::array<double, 5>, 2> deaths{};     // [a, b][cause]
    std::map<std::string, int64_t> reasons[2], fatal[2];
    for (size_t i = 0; i < jobs.size(); ++i) {
      const Outcome& o = results[i];
      const int ta = jobs[i].a_is_team_a ? 0 : 1;
      if (o.winner < 0) ++d;
      else if (o.winner == ta) ++w;
      else ++l;
      longest += o.longest[ta] - o.longest[1 - ta];
      total += o.total[ta] - o.total[1 - ta];
      rounds += o.rounds;
      for (int s = 0; s < 2; ++s) {
        const int team = s ? 1 - ta : ta;
        for (int c = 0; c < 5; ++c) deaths[s][c] += o.deaths[team][c];
        for (const auto& [k, v] : o.reasons[team]) reasons[s][k] += v;
        for (const auto& [k, v] : o.fatal[team]) fatal[s][k] += v;
      }
      if (maps.size() <= 32)
        std::printf("  %-18s %s as %c: %s after %d rounds (longest %d-%d, total %d-%d)\n",
                    maps[jobs[i].map].first.c_str(), a.name.c_str(), ta ? 'B' : 'A',
                    o.winner < 0 ? "draw" : (o.winner == ta ? "WIN " : "loss"), o.rounds, o.longest[ta],
                    o.longest[1 - ta], o.total[ta], o.total[1 - ta]);
    }
    const double n = static_cast<double>(jobs.size());
    std::printf("%s vs %s: %d-%d-%d (W-D-L) over %zu matches, %.1fs\n", a.name.c_str(), b.name.c_str(), w, d, l,
                jobs.size(), std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count());
    std::printf("  mean rounds %.0f; at the end %s leads by %.1f longest, %.1f total\n", rounds / n, a.name.c_str(),
                longest / n, total / n);
    for (int s = 0; s < 2; ++s) {
      std::printf("  deaths per match, %s:", (s ? b : a).name.c_str());
      for (int c = 0; c < 5; ++c) std::printf(" %s %.1f", kCauses[c], deaths[s][c] / n);
      std::printf("\n");
      if (reasons[s].empty()) continue;
      std::printf("    actions:");
      for (const auto& [k, v] : reasons[s]) std::printf(" [%s] %.0f", k.c_str(), v / n);
      std::printf("\n    fatal turns:");
      for (const auto& [k, v] : fatal[s]) std::printf(" [%s] %.1f", k.c_str(), v / n);
      std::printf("\n");
    }
  } catch (const std::exception& e) {
    std::fprintf(stderr, "sim: %s\n", e.what());
    return 1;
  }
  return 0;
}
