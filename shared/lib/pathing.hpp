// Searches over a dragon's known board (BoardMemory::Step: portals, wrap, kelp).
#pragma once

#include <cstdint>
#include <functional>
#include <vector>

#include "board_memory.hpp"

namespace strat {

// Breadth-first search from `from`. A tile is entered only if enter(t) says so
// (the caller decides what is safe). For every reached tile: its distance and the
// direction of the first step on a shortest path to it.
struct Paths {
  std::vector<int32_t> dist;               // -1 unreached
  std::vector<int8_t> first;               // -1 for `from` and unreached tiles
  int32_t Nearest(const std::function<bool(int32_t)>& goal) const;   // -1 if none reached
};
Paths Search(const BoardMemory& board, int32_t from, const std::function<bool(int32_t)>& enter,
             int32_t max_dist = 1 << 30);

// Tiles reachable from `from` through tiles enter() allows, counting up to `limit`.
// An edge never seen counts as an escape (reaching it returns `limit`). `region`, if
// given, receives the tiles counted.
int32_t RoomAround(const BoardMemory& board, int32_t from, const std::function<bool(int32_t)>& enter,
                   int32_t limit, std::vector<int32_t>* region = nullptr);

}  // namespace strat
