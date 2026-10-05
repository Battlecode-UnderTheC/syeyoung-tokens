// g43-9 (ring-portal-1, iteration 2): portal pairs shared over sonar, and allies' pings through
// a portal heard as "an ally stands at that end".
//
// Each dragon remembers a portal's far end only once it has seen both ends itself (BoardMemory is
// per dragon, and a split child starts empty), so most workers saw most portals as unknown: on
// Trophy, whose one pair has ends 16 rows apart, our workers crossed 1.5 times a game in r0-150.
// A dragon that knows a pair now tells its allies, and an ally that hears it treats that portal
// as known (pathing, frontier, arrival times), exactly as if it had seen both ends.
//
// Word (magic 0xAB, its own word, never mixed into another):
//   63-56 magic | 55 team | 54-43 portal code (the pair's smaller edge id; codes > 4095 are not sent)
//   42 h1 | 41-34 x1 | 33-26 y1      the sender's end: north side of (x1, y1) if h1, else west side
//   25 h2 | 24-17 x2 | 16-9 y2       the far end (valid when bit 7 is set)
//   8 ping: the sender stands beside its end and pings through it this round (bits 1-0: that side)
//   7 the far end is valid
// Coordinates take 8 bits (boards are at most 256 a side).
#include <algorithm>
#include <type_traits>

#include "strategy.hpp"

namespace pshare {
// BoardMemory keeps its portal ends private (see sym.hpp for the same access exemption)
template <typename Tag, auto M>
struct Grab {
  friend auto& Ends(Tag, strat::BoardMemory& b) { return b.*M; }
};
struct EndsTag {};
auto& Ends(EndsTag, strat::BoardMemory& b);
template struct Grab<EndsTag, &strat::BoardMemory::portal_ends_>;
constexpr uint64_t kPortalMagic = 0xAB;
}  // namespace pshare

namespace gm2 {

namespace {
struct Edge {
  bool h;
  int32_t x, y;
};
}  // namespace

// the edge on side d of tile t (as BoardMemory::Step names it)
static Edge SideEdge(const strat::BoardMemory& b, int32_t t, int d) {
  const int32_t x = b.X(t), y = b.Y(t);
  if (d == 0) return {true, x, y};
  if (d == 2) return {true, b.X(b.Tile(x, y + 1)), b.Y(b.Tile(x, y + 1))};
  if (d == 3) return {false, x, y};
  return {false, b.X(b.Tile(x + 1, y)), b.Y(b.Tile(x + 1, y))};
}

int32_t StrategyAgent::PortalCode(int32_t t, int d) const {
  auto& b = const_cast<strat::BoardMemory&>(board_);
  const Edge e = SideEdge(board_, t, d);
  const auto& arr = e.h ? b.*Get(symm::NorthTag{}) : b.*Get(symm::WestTag{});
  return arr[board_.Tile(e.x, e.y)];
}

uint64_t StrategyAgent::PortalWord(int32_t t, int d, bool ping) const {
  const int32_t code = PortalCode(t, d);
  if (code < 0 || code > 4095) return 0;
  const Edge me = SideEdge(board_, t, d);
  uint64_t w = pshare::kPortalMagic << 56 | uint64_t(spawn_.team & 1) << 55 | uint64_t(code) << 43 |
               uint64_t(me.h) << 42 | uint64_t(me.x & 255) << 34 | uint64_t(me.y & 255) << 26 | uint64_t(d & 3);
  if (ping) w |= uint64_t(1) << 8;
  auto& ends = pshare::Ends(pshare::EndsTag{}, const_cast<strat::BoardMemory&>(board_));
  if (const auto it = ends.find(code); it != ends.end())
    for (const auto& e : it->second)
      if (!(e.horizontal == me.h && e.x == me.x && e.y == me.y)) {
        w |= uint64_t(1) << 7 | uint64_t(e.horizontal) << 25 | uint64_t(e.x & 255) << 17 | uint64_t(e.y & 255) << 9;
        break;
      }
  return w;
}

// a known pair to broadcast: cycles through the pairs this dragon knows (0 when none)
uint64_t StrategyAgent::NextPairWord() {
  auto& ends = pshare::Ends(pshare::EndsTag{}, board_);
  std::vector<int32_t> codes;
  for (const auto& [code, v] : ends)
    if (v.size() >= 2 && code <= 4095) codes.push_back(code);
  if (codes.empty()) return 0;
  std::sort(codes.begin(), codes.end());
  const int32_t code = codes[pair_cursor_++ % codes.size()];
  const auto& v = ends[code];
  return pshare::kPortalMagic << 56 | uint64_t(spawn_.team & 1) << 55 | uint64_t(code) << 43 |
         uint64_t(v[0].horizontal) << 42 | uint64_t(v[0].x & 255) << 34 | uint64_t(v[0].y & 255) << 26 |
         uint64_t(1) << 7 | uint64_t(v[1].horizontal) << 25 | uint64_t(v[1].x & 255) << 17 | uint64_t(v[1].y & 255) << 9;
}

void StrategyAgent::HearPortal(uint64_t w) {
  if ((w >> 56) != pshare::kPortalMagic || int32_t((w >> 55) & 1) != (spawn_.team & 1)) return;
  const int32_t code = int32_t((w >> 43) & 4095);
  const Edge e1{((w >> 42) & 1) != 0, int32_t((w >> 34) & 255), int32_t((w >> 26) & 255)};
  if (e1.x >= board_.width() || e1.y >= board_.height()) return;
  const int d1 = int(w & 3);
  if ((w >> 8) & 1) {
    // an ally beside its end pinged through it last round: its head is on the tile at side d1's
    // near side, and a step through the pair lands on (or beside) it
    const int32_t at = e1.h ? (d1 == 0 ? board_.Tile(e1.x, e1.y) : board_.Tile(e1.x, e1.y - 1))
                            : (d1 == 3 ? board_.Tile(e1.x, e1.y) : board_.Tile(e1.x - 1, e1.y));
    ally_pings_.push_back({at, d1, code});
  }
  if (!((w >> 7) & 1)) return;
  const Edge e2{((w >> 25) & 1) != 0, int32_t((w >> 17) & 255), int32_t((w >> 9) & 255)};
  if (e2.x >= board_.width() || e2.y >= board_.height() || e2.h != e1.h) return;
  auto& ends = pshare::Ends(pshare::EndsTag{}, board_);
  auto& v = ends[code];
  using Ref = typename std::remove_reference_t<decltype(v)>::value_type;
  for (const Edge& e : {e1, e2}) {
    const Ref r{e.h, e.x, e.y};
    if (std::find(v.begin(), v.end(), r) == v.end()) {
      if (v.size() >= 2) return;   // conflicts with what we saw: keep our own
      v.push_back(r);
    }
    // the edge itself, and the portal bits of the tiles beside it
    auto& arr = e.h ? board_.*Get(symm::NorthTag{}) : board_.*Get(symm::WestTag{});
    const int32_t t = board_.Tile(e.x, e.y);
    if (arr[t] == symm::kEdgeUnseenCode) arr[t] = code;
    if (e.h) {
      portal_[t] |= 1 << 0;
      portal_[board_.Tile(e.x, e.y - 1)] |= 1 << 2;
    } else {
      portal_[t] |= 1 << 3;
      portal_[board_.Tile(e.x - 1, e.y)] |= 1 << 1;
    }
  }
  ++pairs_heard_;
}

// an ally pinged through a portal last round from the tile our step through side d lands on
// (pair known), or through an unknown portal towards us (heading opposite to ours)
bool StrategyAgent::AllyAtLanding(int d) const {
  if (ally_pings_.empty() || !(portal_[head_] >> d & 1)) return false;
  const int32_t land = board_.Step(head_, d), code = PortalCode(head_, d);
  for (const AllyPing& p : ally_pings_) {
    if (land >= 0 && p.tile == land) return true;
    if (p.code == code && p.dir == (d + 2) % 4) return true;
  }
  return false;
}

}  // namespace gm2
