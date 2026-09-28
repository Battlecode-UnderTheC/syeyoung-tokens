// The 7x7 window as a tiny grid: which steps from a window tile stay on plain ground.
// Enough for the simplest bots (the anchors); real variants remember the board
// (board_memory.hpp).
#pragma once

#include "observation.hpp"

namespace win {

inline constexpr int kCenter = rl::kWindowTiles / 2;

// The window tile one step from tile k in direction d (0 N, 1 E, 2 S, 3 W), or -1 when the
// step leaves the window, crosses kelp or a portal, or ends off the board.
inline int Step(const rl::Observation& o, int k, int d) {
  const int r = k / rl::kWindow, c = k % rl::kWindow, W = rl::kWindow;
  const int edge = d == 0 ? o.hedge[r * W + c]
                 : d == 2 ? o.hedge[(r + 1) * W + c]
                 : d == 3 ? o.vedge[r * (W + 1) + c]
                          : o.vedge[r * (W + 1) + c + 1];
  if (edge != -1) return -1;
  const int nr = r + dragons::kDy[d], nc = c + dragons::kDx[d];
  if (nr < 0 || nr >= W || nc < 0 || nc >= W) return -1;
  const int n = nr * W + nc;
  return o.tile_x[n] < 0 || o.tile_y[n] < 0 ? -1 : n;
}

// Whether a dragon segment lies on window tile k.
inline bool Occupied(const rl::Observation& o, int k) {
  for (const rl::Segment& s : o.segments)
    if (s.x == o.tile_x[k] && s.y == o.tile_y[k]) return true;
  return false;
}

}  // namespace win
