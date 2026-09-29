// snap <A> <B> <map> <seed>: JSON line with per-team count/longest/total at end of rounds 25,50,100,200 and final.
#include <cstdio>
#include <memory>
#include <map>
#include "play.hpp"
using namespace tools;
int main(int argc, char** argv) {
  const Variant a = Load(argv[1], argv[0]), b = Load(argv[2], argv[0]);
  const auto maps = ReadMaps(argv[3], 1);
  const uint64_t seed = std::stoull(argv[4], nullptr, 0);
  SetPearlSeed(seed);
  dragons::Game g;
  g.Reset(dragons::LoadBoard(maps[0].second));
  std::map<int32_t, std::unique_ptr<bot::Agent>> live;
  const int R[7] = {25, 50, 100, 200, 300, 400, 450};
  bool done[7] = {};
  std::printf("{\"snaps\": {");
  bool first = true;
  const Variant* sides[2] = {&a, &b};
  auto snap = [&](const char* key) {
    int n[2] = {}, lo[2] = {}, to[2] = {};
    for (const auto& dr : g.dragons())
      if (dr.alive) { ++n[dr.team]; lo[dr.team] = std::max(lo[dr.team], dr.length); to[dr.team] += dr.length; }
    std::printf("%s\"%s\": [[%d,%d,%d],[%d,%d,%d]]", first ? "" : ", ", key, n[0], lo[0], to[0], n[1], lo[1], to[1]);
    first = false;
  };
  int cz[2][5] = {}, eat[2][5] = {}, spl[2][5] = {}, dth[2][5] = {};
  while (!g.done()) {
    for (int s = 0; s < 7; ++s)
      if (g.round() == R[s] + 1 && !done[s]) { done[s] = true; snap(std::to_string(R[s]).c_str()); }
    const int32_t d = g.actor();
    const int team = g.dragons()[d].team;
    auto it = live.find(d);
    if (it == live.end()) {
      const rl::Spawn spawn = rl::SpawnOf(g, d);
      it = live.emplace(d, std::unique_ptr<bot::Agent>(sides[team]->make(&spawn))).first;
    }
    const rl::Observation o = rl::Observe(g);
    const bot::Action action = it->second->Turn(o);
    const int before = g.dragons()[d].length; const int ndr = (int)g.dragons().size(); const int c = std::min(4, g.round() / 100);
    g.ClearDeaths();
    Apply(g, action);
    if ((int)g.dragons().size() > ndr) ++spl[team][c];
    if (g.dragons()[d].alive && !action.resign && action.kind == bot::Action::kMove)
      eat[team][c] += g.dragons()[d].length - before + (int)action.steps.size() - 1;
    for (const dragons::Death& death : g.deaths()) { live.erase(death.id); ++dth[death.team][c]; if (c <= 1) ++cz[death.team][death.reason]; }
  }
  snap("final");
  const dragons::Result r = g.result();
  std::printf("}, \"eat\": [[%d,%d,%d,%d,%d],[%d,%d,%d,%d,%d]], \"spl\": [[%d,%d,%d,%d,%d],[%d,%d,%d,%d,%d]], \"dth\": [[%d,%d,%d,%d,%d],[%d,%d,%d,%d,%d]]",
    eat[0][0],eat[0][1],eat[0][2],eat[0][3],eat[0][4],eat[1][0],eat[1][1],eat[1][2],eat[1][3],eat[1][4],
    spl[0][0],spl[0][1],spl[0][2],spl[0][3],spl[0][4],spl[1][0],spl[1][1],spl[1][2],spl[1][3],spl[1][4],
    dth[0][0],dth[0][1],dth[0][2],dth[0][3],dth[0][4],dth[1][0],dth[1][1],dth[1][2],dth[1][3],dth[1][4]);
  std::printf(", \"cz\": [[%d,%d,%d,%d,%d],[%d,%d,%d,%d,%d]]", cz[0][0],cz[0][1],cz[0][2],cz[0][3],cz[0][4],cz[1][0],cz[1][1],cz[1][2],cz[1][3],cz[1][4]);
  std::printf(", \"rounds\": %d, \"winner\": %d}\n", g.round(), (int)r.winner);
  return 0;
}
