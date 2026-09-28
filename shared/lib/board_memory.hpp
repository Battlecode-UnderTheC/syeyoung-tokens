// What one dragon knows of the board: every tile and edge it has seen, where
// pearls were, which portals pair up, and who stood where when last seen.
// Built only from its own observations (a dragon's process sees nothing else).
#pragma once

#include <cstdint>
#include <unordered_map>
#include <vector>

#include "observation.hpp"

namespace strat {

inline constexpr int32_t kBlocked = -1;   // Step(): kelp
inline constexpr int32_t kUnknown = -2;   // Step(): an edge or a portal's far end never seen

struct TileMemory {
  int32_t seen_round = -1;                // last round in view (-1 never)
  bool pearl = false;                     // a pearl there when last seen
  int32_t countdown = -1;                 // its bed's countdown when last seen (-1 no bed)
  int32_t occupant = -1;                  // dragon id on it when last seen (-1 none)
  uint8_t occupant_team = 0;
  bool occupant_head = false;
};

class BoardMemory {
 public:
  BoardMemory(int32_t width, int32_t height);

  // Folds in one observation: the 7x7 window's tiles, edges and dragons.
  void Update(const rl::Observation& o);

  int32_t width() const { return w_; }
  int32_t height() const { return h_; }
  int32_t size() const { return w_ * h_; }
  int32_t Tile(int32_t x, int32_t y) const;   // wrapped (the board is a torus)
  int32_t X(int32_t t) const { return t % w_; }
  int32_t Y(int32_t t) const { return t / w_; }
  const TileMemory& at(int32_t t) const { return tiles_[t]; }

  // The tile a step from `t` towards `dir` (0 N, 1 E, 2 S, 3 W) lands on, as far as
  // this dragon knows: through portals it has seen both ends of; kBlocked for kelp;
  // kUnknown for an edge it never saw or a portal whose far end it hasn't found.
  int32_t Step(int32_t t, int dir) const;

  // Seen in the current round (so its occupant is exact, not a stale memory).
  bool Current(int32_t t) const { return tiles_[t].seen_round == round_; }
  int32_t round() const { return round_; }

 private:
  static constexpr int32_t kEdgeUnseen = INT32_MIN;
  struct EdgeRef {
    bool horizontal;                      // north side of (x, y); else west side of (x, y)
    int32_t x, y;
    bool operator==(const EdgeRef&) const = default;
  };
  int32_t SideCode(int32_t t, int dir) const;          // the edge on that side of t
  int32_t Exit(const EdgeRef& partner, int dir) const; // the tile past a portal's far end

  int32_t w_, h_, round_ = -1;
  std::vector<TileMemory> tiles_;
  std::vector<int32_t> north_, west_;                  // edge codes: -1 empty, -2 kelp, portal id
  std::unordered_map<int32_t, std::vector<EdgeRef>> portal_ends_;
};

}  // namespace strat
