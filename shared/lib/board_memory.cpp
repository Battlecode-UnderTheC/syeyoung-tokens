#include "board_memory.hpp"

#include <algorithm>

namespace strat {

BoardMemory::BoardMemory(int32_t width, int32_t height)
    : w_(width), h_(height), tiles_(width * height), north_(width * height, kEdgeUnseen),
      west_(width * height, kEdgeUnseen) {}

int32_t BoardMemory::Tile(int32_t x, int32_t y) const {
  x = ((x % w_) + w_) % w_;
  y = ((y % h_) + h_) % h_;
  return y * w_ + x;
}

void BoardMemory::Update(const rl::Observation& o) {
  round_ = o.round;
  constexpr int R = dragons::kVisionRadius, V = dragons::kVision;
  const int32_t hx = o.head_x(), hy = o.head_y();
  // tiles: seen now, pearls and beds as they are, occupants cleared (bodies are added below)
  for (int k = 0; k < rl::kWindowTiles; ++k) {
    TileMemory& m = tiles_[Tile(o.tile_x[k], o.tile_y[k])];
    m.seen_round = round_;
    m.pearl = o.pearl[k] != 0;
    m.countdown = o.countdown[k];
    m.occupant = -1;
  }
  for (const rl::Segment& s : o.segments) {
    TileMemory& m = tiles_[Tile(s.x, s.y)];
    m.occupant = s.id;
    m.occupant_team = s.team;
    m.occupant_head = s.head;
  }
  // edges: north sides of window rows 0..7, west sides of window columns 0..7
  auto note = [&](bool horizontal, int32_t x, int32_t y, int32_t code) {
    const int32_t t = Tile(x, y);
    (horizontal ? north_ : west_)[t] = code;
    if (code >= 0) {                                   // a portal end
      const EdgeRef e{horizontal, X(t), Y(t)};
      auto& ends = portal_ends_[code];
      if (std::find(ends.begin(), ends.end(), e) == ends.end()) ends.push_back(e);
    }
  };
  for (int r = 0; r <= V; ++r)
    for (int c = 0; c < V; ++c) note(true, hx - R + c, hy - R + r, o.hedge[r * V + c]);
  for (int r = 0; r < V; ++r)
    for (int c = 0; c <= V; ++c) note(false, hx - R + c, hy - R + r, o.vedge[r * (V + 1) + c]);
}

int32_t BoardMemory::SideCode(int32_t t, int dir) const {
  const int32_t x = X(t), y = Y(t);
  switch (dir) {
    case 0: return north_[t];
    case 2: return north_[Tile(x, y + 1)];
    case 3: return west_[t];
    default: return west_[Tile(x + 1, y)];
  }
}

// engine: TileAfterCrossing -- out of the partner edge, still heading the same way
int32_t BoardMemory::Exit(const EdgeRef& e, int dir) const {
  if (e.horizontal) return Tile(e.x, dir == 2 ? e.y : e.y - 1);
  return Tile(dir == 1 ? e.x : e.x - 1, e.y);
}

int32_t BoardMemory::Step(int32_t t, int dir) const {
  const int32_t code = SideCode(t, dir);
  if (code == kEdgeUnseen) return kUnknown;
  if (code == dragons::kEdgeKelp) return kBlocked;
  if (code == dragons::kEdgeEmpty) return Tile(X(t) + dragons::kDx[dir], Y(t) + dragons::kDy[dir]);
  // a portal: this side's edge, then the other end with the same id
  const int32_t x = X(t), y = Y(t);
  const EdgeRef here = dir == 0   ? EdgeRef{true, x, y}
                       : dir == 2 ? EdgeRef{true, X(Tile(x, y + 1)), Y(Tile(x, y + 1))}
                       : dir == 3 ? EdgeRef{false, x, y}
                                  : EdgeRef{false, X(Tile(x + 1, y)), Y(Tile(x + 1, y))};
  const auto it = portal_ends_.find(code);
  if (it == portal_ends_.end()) return kUnknown;
  for (const EdgeRef& e : it->second)
    if (!(e == here)) return Exit(e, dir);
  return kUnknown;
}

}  // namespace strat
