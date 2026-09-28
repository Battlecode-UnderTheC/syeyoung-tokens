// Plays two variants against each other in the native engine (the judge's rules), many
// matches at once, every map from both sides.
//
//   sim --a <variant> --b <variant> --maps <dir of .map | pool file> [--limit N] [--threads N]
//   sim --a <variant> --b <variant> --maps <dir> --trace <map name>      one match, verbosely
//
// A variant is a name (loaded from build/variants/lib<name>.so) or a path to a plugin.
// Prints W-D-L from a's side, final length margins, deaths by cause, and the tallies of
// each side's reasons (Agent::reason()) and of the reasons behind its fatal turns.
#include <array>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <map>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include "play.hpp"

using namespace tools;

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
