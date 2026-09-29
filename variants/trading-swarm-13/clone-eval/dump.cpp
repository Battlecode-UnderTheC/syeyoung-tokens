// dump <A> <B> <map> <seed> <team>: one JSON line per turn of <team> (0/1) in rounds < 200:
// r, L, f, act, ate, pearls/beds(countdown)/ally heads/enemy heads/occupied, window-relative (dx,dy)
#include <cstdio>
#include <memory>
#include <map>
#include <string>
#include "play.hpp"
#include "window.hpp"
// tiles reachable in the window from step d (not back through the head); a portal edge on the way counts 20
static int RoomOf(const rl::Observation& o, int d) {
  const int start = win::Step(o, 24, d);
  if (start < 0) {
    const int r = 3, c = 3, W = 7;
    const int e = d == 0 ? o.hedge[r * W + c] : d == 2 ? o.hedge[(r + 1) * W + c] : d == 3 ? o.vedge[r * (W + 1) + c] : o.vedge[r * (W + 1) + c + 1];
    return e >= 0 ? 20 : -1;
  }
  bool occ[49] = {};
  for (const rl::Segment& sg : o.segments)
    for (int j = 0; j < 49; ++j) if (o.tile_x[j] == sg.x && o.tile_y[j] == sg.y) occ[j] = true;
  if (occ[start]) return -1;
  bool seen[49] = {}; seen[24] = seen[start] = true;
  int st[49], sp = 0, n = 0, bonus = 0; st[sp++] = start;
  while (sp) {
    const int k = st[--sp]; ++n;
    for (int e = 0; e < 4; ++e) {
      const int r = k / 7, c = k % 7, W = 7;
      const int ed = e == 0 ? o.hedge[r * W + c] : e == 2 ? o.hedge[(r + 1) * W + c] : e == 3 ? o.vedge[r * (W + 1) + c] : o.vedge[r * (W + 1) + c + 1];
      if (ed >= 0) bonus = 20;
      const int m = win::Step(o, k, e);
      if (m >= 0 && !seen[m] && !occ[m]) { seen[m] = true; st[sp++] = m; }
    }
  }
  return std::min(20, n + bonus);
}
using namespace tools;
int main(int argc, char** argv) {
  const Variant a = Load(argv[1], argv[0]), b = Load(argv[2], argv[0]);
  const auto maps = ReadMaps(argv[3], 1);
  const uint64_t seed = std::stoull(argv[4], nullptr, 0);
  const int want = std::atoi(argv[5]);
  SetPearlSeed(seed);
  dragons::Game g;
  g.Reset(dragons::LoadBoard(maps[0].second));
  std::map<int32_t, std::unique_ptr<bot::Agent>> live;
  const Variant* sides[2] = {&a, &b};
  while (!g.done() && g.round() < 200) {
    const int32_t d = g.actor();
    const int team = g.dragons()[d].team;
    auto it = live.find(d);
    if (it == live.end()) {
      const rl::Spawn spawn = rl::SpawnOf(g, d);
      it = live.emplace(d, std::unique_ptr<bot::Agent>(sides[team]->make(&spawn))).first;
    }
    const rl::Observation o = rl::Observe(g);
    const bot::Action action = it->second->Turn(o);
    const int before = g.dragons()[d].length;
    g.ClearDeaths();
    Apply(g, action);
    if (team == want) {
      std::string s = "{\"r\":" + std::to_string(o.round) + ",\"id\":" + std::to_string(d) + ",\"L\":" + std::to_string(o.length) +
                      ",\"f\":" + std::to_string(o.facing) + ",\"hx\":" + std::to_string(o.head_x()) + ",\"hy\":" + std::to_string(o.head_y());
      if (action.resign) s += ",\"act\":[\"x\"]";
      else if (action.kind == bot::Action::kSplit) s += ",\"act\":[\"s\"," + std::to_string(action.split) + "]";
      else {
        s += ",\"act\":[\"m\",[";
        for (size_t i = 0; i < action.steps.size(); ++i) s += (i ? "," : "") + std::to_string(action.steps[i]);
        s += "]]";
      }
      const bool alive = g.dragons()[d].alive;
      s += ",\"ate\":" + std::to_string(alive && action.kind == bot::Action::kMove && !action.resign ? g.dragons()[d].length - before + (int)action.steps.size() - 1 : 0);
      s += ",\"died\":" + std::string(alive ? "0" : "1");
      std::string pe, be, al, en, oc;
      for (int k = 0; k < rl::kWindowTiles; ++k) {
        const int dx = k % 7 - 3, dy = k / 7 - 3;
        const std::string p = "[" + std::to_string(dx) + "," + std::to_string(dy);
        if (o.tile_x[k] < 0) continue;
        if (o.pearl[k]) pe += (pe.empty() ? "" : ",") + p + "]";
        else if (o.countdown[k] >= 0) be += (be.empty() ? "" : ",") + p + "," + std::to_string(o.countdown[k]) + "]";
      }
      for (const rl::Segment& sg : o.segments) {
        int k = -1;
        for (int j = 0; j < rl::kWindowTiles; ++j) if (o.tile_x[j] == sg.x && o.tile_y[j] == sg.y) { k = j; break; }
        if (k < 0) continue;
        const std::string p = "[" + std::to_string(k % 7 - 3) + "," + std::to_string(k / 7 - 3) + "]";
        oc += (oc.empty() ? "" : ",") + p;
        if (sg.head && sg.id != d) (sg.team == team ? al : en) += ((sg.team == team ? al : en).empty() ? "" : ",") + p;
      }
      s += ",\"room\":[" + std::to_string(RoomOf(o, 0)) + "," + std::to_string(RoomOf(o, 1)) + "," + std::to_string(RoomOf(o, 2)) + "," + std::to_string(RoomOf(o, 3)) + "]";
      s += ",\"pearls\":[" + pe + "],\"beds\":[" + be + "],\"ally\":[" + al + "],\"enemy\":[" + en + "],\"occ\":[" + oc + "]}";
      std::puts(s.c_str());
    }
    for (const dragons::Death& death : g.deaths()) live.erase(death.id);
  }
  return 0;
}
