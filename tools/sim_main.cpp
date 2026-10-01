// Plays two variants against each other in the native engine (the judge's rules), many
// matches at once, every map from both sides.
//
//   sim --a <variant> --b <variant> --maps <dir of .map | pool file> [--limit N] [--threads N]
//   sim --a <variant> --b <variant> --maps <dir> --trace <map name>      one match, verbosely
//   ... --seed S[,S...]   pearl seeds (the judge's `unswbc run --seed`; decimal or 0x hex): every
//                         map is played from both sides at each seed (default: kPearlRngSeed,
//                         0x5eed5eed = 1592614637). --trace uses the first.
//
// With the same seed a match here is the judge's (`unswbc run ... --sandbox --seed S`) move for
// move: tools/judge_engine.cpp draws pearls as unswbc 1.2 does; the rules are 1.2.3's.
//
// A variant is a name (loaded from build/variants/lib<name>.so) or a path to a plugin.
// Prints W-D-L from a's side, what decided the games (unswbc 1.2.3's order: elimination,
// then at round 500 the longer queen, the longest dragon, the greater total), each side's
// queen (dragon 0 or 1) at r100 and at the end and how often it died, final length margins,
// deaths by cause, and the tallies of each side's reasons (Agent::reason()) and of the
// reasons behind its fatal turns.
#include <array>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include "play.hpp"

using namespace tools;

int main(int argc, char** argv) {
  std::string a_arg, b_arg, maps_path, trace_map;
  std::vector<uint64_t> seeds;
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
    else if (k == "--seed" || k == "--seeds") {
      std::stringstream list(v);
      for (std::string item; std::getline(list, item, ',');)
        if (!item.empty()) seeds.push_back(std::stoull(item, nullptr, 0));
    }
    else {
      std::fprintf(stderr, "unknown option %s\n", k.c_str());
      return 2;
    }
  }
  if (a_arg.empty() || b_arg.empty() || maps_path.empty()) {
    std::fprintf(stderr, "usage: %s --a VARIANT --b VARIANT --maps DIR_OR_POOL [--limit N] [--threads N] [--seed S[,S...]] [--trace MAP]\n",
                 argv[0]);
    return 2;
  }
  if (seeds.empty()) seeds.push_back(dragons::kPearlRngSeed);
  try {
    const Variant a = Load(a_arg, argv[0]), b = Load(b_arg, argv[0]);
    const auto maps = ReadMaps(maps_path, limit);
    if (!trace_map.empty()) {
      for (const auto& [name, text] : maps)
        if (name == trace_map) {
          std::printf("== %s: %s is team A, %s team B\n", name.c_str(), a.name.c_str(), b.name.c_str());
          std::printf("== seed %llu (0x%016llx)\n", static_cast<unsigned long long>(seeds[0]),
                      static_cast<unsigned long long>(seeds[0]));
          const Outcome o = Play(text, {&a, &b}, true, seeds[0]);
          std::printf("== %s after %d rounds\n", o.winner < 0 ? "draw" : (o.winner == 0 ? "A wins" : "B wins"), o.rounds);
          return 0;
        }
      throw std::runtime_error("no map named " + trace_map);
    }
    struct Job {
      size_t map;
      bool a_is_team_a;
      uint64_t seed;
    };
    std::vector<Job> jobs;
    for (size_t m = 0; m < maps.size(); ++m)
      for (const uint64_t seed : seeds)
        for (const bool side : {true, false}) jobs.push_back({m, side, seed});
    std::vector<Outcome> results(jobs.size());
    std::atomic<size_t> next{0};
    const auto t0 = std::chrono::steady_clock::now();
    std::vector<std::thread> pool;
    for (int t = 0; t < threads; ++t)
      pool.emplace_back([&] {
        for (size_t i; (i = next++) < jobs.size();) {
          const Job& j = jobs[i];
          results[i] = Play(maps[j.map].second, j.a_is_team_a ? std::array<const Variant*, 2>{&a, &b}
                                                              : std::array<const Variant*, 2>{&b, &a},
                            false, j.seed);
        }
      });
    for (auto& t : pool) t.join();

    int w = 0, d = 0, l = 0;
    double longest = 0, total = 0, rounds = 0;
    // queens, [a, b]: summed length at the end, at r100 (over games that reached it), deaths
    std::array<double, 2> queen{}, queen100{}, queen_dead{};
    int queen100_n = 0;
    std::array<std::array<int, 5>, 3> decided{};   // [a won, draw, b won][Decided]
    std::array<std::array<double, 5>, 2> deaths{};     // [a, b][cause]
    std::array<std::array<double, 5>, 2> own{};        // [a, b][cause], died on its own turn
    std::array<std::array<double, 4>, 2> lunged{};     // [a, b][killed short, their length, killed 10+, their length]
    std::map<std::string, int64_t> reasons[2], fatal[2];
    std::array<std::array<std::array<double, 3>, 2>, 6> snap{};  // [kSnapRounds][a, b][dragons, longest, total]
    std::array<int, 6> snap_n{};                                  // matches that reached each snapshot round
    for (size_t i = 0; i < jobs.size(); ++i) {
      const Outcome& o = results[i];
      const int ta = jobs[i].a_is_team_a ? 0 : 1;
      if (o.winner < 0) ++d;
      else if (o.winner == ta) ++w;
      else ++l;
      decided[o.winner < 0 ? 1 : (o.winner == ta ? 0 : 2)][o.decided] += 1;
      if (o.queen_r100[0] >= 0) ++queen100_n;
      for (int s = 0; s < 2; ++s) {
        const int team = s ? 1 - ta : ta;
        queen[s] += o.queen[team];
        if (o.queen_r100[team] >= 0) queen100[s] += o.queen_r100[team];
        queen_dead[s] += o.queen_died[team] >= 0;
      }
      longest += o.longest[ta] - o.longest[1 - ta];
      total += o.total[ta] - o.total[1 - ta];
      rounds += o.rounds;
      for (int r = 0; r < 6; ++r)
        if (o.snapped[r]) {
          ++snap_n[r];
          for (int s = 0; s < 2; ++s)
            for (int k = 0; k < 3; ++k) snap[r][s][k] += o.snaps[r][s ? 1 - ta : ta][k];
        }
      for (int s = 0; s < 2; ++s) {
        const int team = s ? 1 - ta : ta;
        for (int c = 0; c < 5; ++c) deaths[s][c] += o.deaths[team][c], own[s][c] += o.own_move[team][c];
        for (int k = 0; k < 2; ++k) lunged[s][2 * k] += o.lunged[team][k], lunged[s][2 * k + 1] += o.lunged_len[team][k];
        for (const auto& [k, v] : o.reasons[team]) reasons[s][k] += v;
        for (const auto& [k, v] : o.fatal[team]) fatal[s][k] += v;
      }
      if (jobs.size() <= 64) {
        if (seeds.size() > 1) std::printf("  seed %-20llu", static_cast<unsigned long long>(jobs[i].seed));
        std::printf("  %-18s %s as %c: %s after %d rounds (by %s; queen %d-%d, longest %d-%d, total %d-%d)%s%s\n",
                    maps[jobs[i].map].first.c_str(), a.name.c_str(), ta ? 'B' : 'A',
                    o.winner < 0 ? "draw" : (o.winner == ta ? "WIN " : "loss"), o.rounds, kDecided[o.decided],
                    o.queen[ta], o.queen[1 - ta], o.longest[ta], o.longest[1 - ta], o.total[ta], o.total[1 - ta],
                    o.queen_died[ta] >= 0 ? " [own queen died]" : "", o.queen_died[1 - ta] >= 0 ? " [enemy queen died]" : "");
      }
    }
    const double n = static_cast<double>(jobs.size());
    std::printf("%s vs %s: %d-%d-%d (W-D-L) over %zu matches (%zu seed%s), %.1fs\n", a.name.c_str(), b.name.c_str(), w,
                d, l, jobs.size(), seeds.size(), seeds.size() == 1 ? "" : "s", std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count());
    std::printf("  mean rounds %.0f; at the end %s leads by %.1f longest, %.1f total\n", rounds / n, a.name.c_str(),
                longest / n, total / n);
    std::printf("  queens: %s at r100 %.1f (%d matches), at the end %.1f, died in %.0f%% | %s at r100 %.1f, at the end "
                "%.1f, died in %.0f%%\n",
                a.name.c_str(), queen100_n ? queen100[0] / queen100_n : 0.0, queen100_n, queen[0] / n,
                100 * queen_dead[0] / n, b.name.c_str(), queen100_n ? queen100[1] / queen100_n : 0.0, queen[1] / n,
                100 * queen_dead[1] / n);
    std::printf("  decided by:");
    for (int k = 0; k < 5; ++k) {
      if (k == kByNothing) {
        if (decided[1][k]) std::printf(" draw %d", decided[1][k]);
        continue;
      }
      if (decided[0][k] + decided[2][k] + decided[1][k] == 0) continue;
      std::printf(" %s %d-%d", kDecided[k], decided[0][k], decided[2][k]);
      if (decided[1][k]) std::printf(" (+%d drawn)", decided[1][k]);
    }
    std::printf(" (%s wins-losses)\n", a.name.c_str());
    for (int r = 4; r < 6; ++r)
      if (snap_n[r])
        std::printf("  at r%d (%d matches): %s %.1f dragons, longest %.1f, total %.1f | %s %.1f dragons, longest %.1f, "
                    "total %.1f\n",
                    kSnapRounds[r], snap_n[r], a.name.c_str(), snap[r][0][0] / snap_n[r], snap[r][0][1] / snap_n[r],
                    snap[r][0][2] / snap_n[r], b.name.c_str(), snap[r][1][0] / snap_n[r], snap[r][1][1] / snap_n[r],
                    snap[r][1][2] / snap_n[r]);
    std::printf("  opening (dragons/total) r10-r40: %s", a.name.c_str());
    for (int r = 0; r < 4; ++r)
      if (snap_n[r]) std::printf(" %.1f/%.1f", snap[r][0][0] / snap_n[r], snap[r][0][2] / snap_n[r]);
    std::printf(" | %s", b.name.c_str());
    for (int r = 0; r < 4; ++r)
      if (snap_n[r]) std::printf(" %.1f/%.1f", snap[r][1][0] / snap_n[r], snap[r][1][2] / snap_n[r]);
    std::printf("\n");
    for (int s = 0; s < 2; ++s) {
      std::printf("  deaths per match, %s:", (s ? b : a).name.c_str());
      for (int c = 0; c < 5; ++c) std::printf(" %s %.1f", kCauses[c], deaths[s][c] / n);
      std::printf("; on its own move:");
      for (int c = 0; c < 5; ++c) std::printf(" %s %.1f", kCauses[c], own[s][c] / n);
      std::printf("; lunged by the enemy: %.1f short (length %.1f), %.1f of 10+ (length %.1f)\n", lunged[s][0] / n,
                  lunged[s][1] / n, lunged[s][2] / n, lunged[s][3] / n);
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
