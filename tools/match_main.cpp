// One match, as one line of JSON: what the league runs, one process per match (a
// crashing or hanging variant takes down only its own match).
//
//   match <variant A> <variant B> <map file>
//
// Variants by name or plugin path. Output fields: winner ("A", "B", "draw"), rounds, and
// per team (arrays [A, B]): longest, total, deaths {cause: n}, reasons {why: n},
// fatal {why: n}, turn_us {p50, p99, max, n}. On an error: {"error": "..."} and exit 1.
#include <algorithm>
#include <cstdio>
#include <string>

#include "play.hpp"

namespace {

std::string Quote(const std::string& s) {
  std::string out = "\"";
  for (const char c : s) {
    if (c == '"' || c == '\\') out += '\\';
    if (static_cast<unsigned char>(c) >= 0x20) out += c;
  }
  return out + "\"";
}

std::string Counts(const std::map<std::string, int64_t>& m) {
  std::string out = "{";
  for (const auto& [k, v] : m) out += (out.size() > 1 ? ", " : "") + Quote(k) + ": " + std::to_string(v);
  return out + "}";
}

std::string Percentiles(std::vector<float> v) {
  if (v.empty()) return "{\"p50\": 0, \"p99\": 0, \"max\": 0, \"n\": 0}";
  std::sort(v.begin(), v.end());
  auto at = [&](double q) { return v[static_cast<size_t>(q * (v.size() - 1))]; };
  char buf[160];
  std::snprintf(buf, sizeof buf, "{\"p50\": %.1f, \"p99\": %.1f, \"max\": %.1f, \"n\": %zu}", at(0.5), at(0.99), v.back(),
                v.size());
  return buf;
}

}  // namespace

int main(int argc, char** argv) {
  if (argc != 4) {
    std::fprintf(stderr, "usage: %s <variant A> <variant B> <map file>\n", argv[0]);
    return 2;
  }
  try {
    const tools::Variant a = tools::Load(argv[1], argv[0]), b = tools::Load(argv[2], argv[0]);
    const auto maps = tools::ReadMaps(argv[3], 1);
    const tools::Outcome o = tools::Play(maps[0].second, {&a, &b});
    std::string deaths[2];
    for (int t = 0; t < 2; ++t) {
      std::map<std::string, int64_t> m;
      for (int c = 0; c < 5; ++c) m[tools::kCauses[c]] = o.deaths[t][c];
      deaths[t] = Counts(m);
    }
    std::printf(
        "{\"winner\": \"%s\", \"rounds\": %d, \"longest\": [%d, %d], \"total\": [%d, %d], \"deaths\": [%s, %s], "
        "\"reasons\": [%s, %s], \"fatal\": [%s, %s], \"turn_us\": [%s, %s]}\n",
        o.winner < 0 ? "draw" : (o.winner == 0 ? "A" : "B"), o.rounds, o.longest[0], o.longest[1], o.total[0],
        o.total[1], deaths[0].c_str(), deaths[1].c_str(), Counts(o.reasons[0]).c_str(), Counts(o.reasons[1]).c_str(),
        Counts(o.fatal[0]).c_str(), Counts(o.fatal[1]).c_str(), Percentiles(o.turn_us[0]).c_str(),
        Percentiles(o.turn_us[1]).c_str());
  } catch (const std::exception& e) {
    std::printf("{\"error\": %s}\n", Quote(e.what()).c_str());
    return 1;
  }
  return 0;
}
