// g39-sym (ring-sym-1): map symmetry, inferred from observation.
//
// Every contest map declares SYMMETRY x (y -> h-1-y), y (x -> w-1-x) or xy (both: a point
// mirror), and a bed and its mirror share one countdown (docs/game.md, Board). Bots are not
// told which, so each dragon keeps the three candidates and drops one the moment its own
// observations contradict it: a tile and its mirror must agree on being a bed (and on the
// countdown, when both readings are known to lie before the next tick), an edge and its mirror
// on its kind (empty, kelp, portal). g42-2 (ring-symearly-1): a candidate is used as soon as
// it is the only one left (no extra confirming pairs), the surviving set travels in every
// champion, claim and ring sonar word so a teammate's contradiction is ours too, and dragon 0
// checks the starting layout in its round-0 window (starters mirror the other side's). The
// shared commit mirrors edges; beds still wait for the dragon's own evidence (TilesUsable).
//
// Use: the mirror of every observed tile and edge is written into the dragon's BoardMemory
// where it has never seen the mirror itself: empty and kelp edges (not portals: their pairing
// would need invented ids), beds with their shared countdown, and plain tiles. Inferred
// tiles are stamped one round older than their source, so BoardMemory::Current() never
// treats them as seen now (their occupants stay unknown). A bed seen for real before keeps
// its own memory unless that memory is stale and holds no pearl: then the mirror's fresher
// countdown replaces it (the shared timer).
#pragma once

#include <cstdint>
#include <cstdio>
#include <vector>

#include "board_memory.hpp"
#include "observation.hpp"

namespace symm {

inline constexpr int kConfirm = 12;   // own agreeing pairs before beds are mirrored (ring-sym-1's value)

// BoardMemory keeps its arrays private; explicit instantiation may name private members
// (the standard's access exemption for explicit instantiations), which lets this variant
// write the inferred tiles into the shared memory class without changing the shared layer.
template <typename Tag, typename Tag::type M>
struct Rob {
  friend typename Tag::type Get(Tag) { return M; }
};
struct NorthTag {
  using type = std::vector<int32_t> strat::BoardMemory::*;
  friend type Get(NorthTag);
};
struct WestTag {
  using type = std::vector<int32_t> strat::BoardMemory::*;
  friend type Get(WestTag);
};
struct TilesTag {
  using type = std::vector<strat::TileMemory> strat::BoardMemory::*;
  friend type Get(TilesTag);
};
template struct Rob<NorthTag, &strat::BoardMemory::north_>;
template struct Rob<WestTag, &strat::BoardMemory::west_>;
template struct Rob<TilesTag, &strat::BoardMemory::tiles_>;

inline constexpr int32_t kEdgeUnseenCode = INT32_MIN;   // BoardMemory's kEdgeUnseen

class SymInfer {
 public:
  // candidate bits: 1 x (mirror y), 2 y (mirror x), 4 xy (both)
  void Init(int32_t w, int32_t h, int32_t id = -1, int32_t team = -1) {
    id_ = id;
    team_ = team;
    w_ = w;
    h_ = h;
    hk_.assign(w * h, 0);
    vk_.assign(w * h, 0);
    cd_.assign(w * h, kTileUnseen);
    cdr_.assign(w * h, -1);
  }
  // g42-2: a candidate is used once it is the only one left. Every map declares one of the
  // three (the engine reads SYMMETRY and mirrors beds and their shared countdown by it), and a
  // candidate is only ever dropped on a contradiction, so the last one standing is the map's.
  // (ring-sym-1 also asked for 12 agreeing pairs; that guards nothing once the others are
  // contradicted, and it held commits back to r57-77.)
  uint8_t Usable() const { return (alive_ == 1 || alive_ == 2 || alive_ == 4) ? alive_ : 0; }
  uint8_t Alive() const { return alive_; }
  // Tiles (beds, their shared timers, plain tiles) are mirrored into memory only on this dragon's
  // own evidence, as ring-sym-1 did: its own observations left one candidate and kConfirm pairs
  // agree with it. Edges (kelp, empty) follow the shared commit. Why: with the shared commit
  // every dragon, children included, knew the far side's bed timers within a few rounds, and on
  // maps with a contested central cluster (Prisoners Dilemma, Autarky) the small dragons went for
  // those beds into the enemy and the team was ground down (arena 0-8 there); with this split
  // the same maps came back to even.
  bool TilesUsable(int s) const { return own_alive_ == (1 << s) && conf_[s] >= kConfirm; }
  // g42-2: a teammate's surviving candidates (sonar bits 1-3): what it has contradicted is
  // contradicted for us too. 0 (it has contradicted all three: no symmetry we model) is ignored.
  void Hint(uint8_t mask, int32_t round) {
    mask &= 7;
    if (mask == 0) return;
    for (int s = 0; s < 3; ++s)
      if ((alive_ >> s & 1) && !(mask >> s & 1)) Kill(s, round, false);
  }

  void Observe(const rl::Observation& o, strat::BoardMemory& b) {
    constexpr int R = dragons::kVisionRadius, V = dragons::kVision;
    const int32_t round = o.round, hx = o.head_x(), hy = o.head_y();
    // g42-2: the starting layout. Every map we have seen (the 22 contest maps, the rotating
    // worldgen and mapgen) places each side's starters as the mirror image of the other's under
    // its declared symmetry. Dragon 0 is the first to act in round 0, so its window shows the
    // starters exactly where they spawned (only the pearl tick came before): a tile and its
    // mirror, both in view, must be both empty or held by opposite teams.
    if (round == 0 && id_ == 0) {
      std::vector<int8_t> occ(rl::kWindowTiles, 0);   // 0 empty, 1 + team
      std::vector<int32_t> at(rl::kWindowTiles);
      for (int k = 0; k < rl::kWindowTiles; ++k) at[k] = Tile(o.tile_x[k], o.tile_y[k]);
      auto index = [&](int32_t t) {
        for (int k = 0; k < rl::kWindowTiles; ++k)
          if (at[k] == t) return k;
        return -1;
      };
      for (const rl::Segment& g : o.segments)
        if (const int k = index(Tile(g.x, g.y)); k >= 0) occ[k] = int8_t(1 + (g.team & 1));
      for (int s = 0; s < 3; ++s) {
        if (!(alive_ >> s & 1)) continue;
        for (int k = 0; k < rl::kWindowTiles && (alive_ >> s & 1); ++k) {
          const int32_t m = MirrorTile(s, at[k]);
          const int j = m == at[k] ? -1 : index(m);
          if (j < 0) continue;
          if ((occ[k] == 0) != (occ[j] == 0) || (occ[k] != 0 && occ[k] == occ[j])) Kill(s, round);
        }
      }
      // the queens: on those maps dragon 1 starts as dragon 0's mirror image, segment for
      // segment, and it has not moved yet. Seen, it must lie on our mirror; its mirror tile in
      // view, it must be there.
      int32_t eq = -1;   // the enemy queen's head, if in view
      for (const rl::Segment& g : o.segments)
        if (g.id == 1 && g.head) eq = Tile(g.x, g.y);
      const int32_t me = Tile(hx, hy);
      for (int s = 0; s < 3; ++s) {
        if (!(alive_ >> s & 1)) continue;
        const int32_t m = MirrorTile(s, me);
        if (eq >= 0 ? eq != m : index(m) >= 0) Kill(s, round);
      }
    }
    // tiles: bed or not, and the shared countdown
    for (int k = 0; k < rl::kWindowTiles; ++k) {
      const int32_t t = Tile(o.tile_x[k], o.tile_y[k]), c = o.countdown[k] >= 0 ? o.countdown[k] : -1;
      const bool fresh = cd_[t] == kTileUnseen;
      cd_[t] = c;
      cdr_[t] = round;
      for (int s = 0; s < 3; ++s) {
        if (!(alive_ >> s & 1)) continue;
        const int32_t m = MirrorTile(s, t);
        if (m == t || cd_[m] == kTileUnseen) continue;
        bool ok = (c >= 0) == (cd_[m] >= 0);
        if (ok && c >= 0) {
          const int32_t predicted = cd_[m] - (round - cdr_[m]);   // no tick in between while >= 1
          if (cdr_[m] == round || predicted >= 1) ok = c == predicted;
        }
        if (!ok) Kill(s, round);
        else if (fresh && c >= 0) ++conf_[s];
      }
    }
    // edges: the north sides of window rows 0..7, the west sides of window columns 0..7
    for (int r = 0; r <= V; ++r)
      for (int c = 0; c < V; ++c) NoteEdge(true, Tile(hx - R + c, hy - R + r), Kind(o.hedge[r * V + c]), round);
    for (int r = 0; r < V; ++r)
      for (int c = 0; c <= V; ++c) NoteEdge(false, Tile(hx - R + c, hy - R + r), Kind(o.vedge[r * (V + 1) + c]), round);
    // fill: a newly usable candidate mirrors everything seen so far, then the window each turn
    const uint8_t use = Usable();
    for (int s = 0; s < 3; ++s) {
      if (!(use >> s & 1)) continue;
      const bool tiles = TilesUsable(s);
      if (!(filled_ >> s & 1)) {
        filled_ |= uint8_t(1 << s);
#ifdef SYM_TRACE
        std::fprintf(stderr, "SYMUSE r%d w%d h%d id%d team%d cand%d conf%d alive%d\n", round, w_, h_, id_, team_, s, conf_[s], alive_);
#endif
        for (int32_t t = 0; t < w_ * h_; ++t) {
          FillEdge(s, true, t, b);
          FillEdge(s, false, t, b);
        }
      } else {
        for (int r = 0; r <= V; ++r)
          for (int c = 0; c < V; ++c) FillEdge(s, true, Tile(hx - R + c, hy - R + r), b);
        for (int r = 0; r < V; ++r)
          for (int c = 0; c <= V; ++c) FillEdge(s, false, Tile(hx - R + c, hy - R + r), b);
      }
      if (!tiles) continue;
      if (!(tiles_filled_ >> s & 1)) {
        tiles_filled_ |= uint8_t(1 << s);
        for (int32_t t = 0; t < w_ * h_; ++t) FillTile(s, t, b, round);
      } else {
        for (int k = 0; k < rl::kWindowTiles; ++k) FillTile(s, Tile(o.tile_x[k], o.tile_y[k]), b, round);
      }
    }
  }

 private:
  static constexpr int32_t kTileUnseen = -2;
  int32_t Tile(int32_t x, int32_t y) const {
    x = ((x % w_) + w_) % w_;
    y = ((y % h_) + h_) % h_;
    return y * w_ + x;
  }
  static int8_t Kind(int32_t code) {
    return code == dragons::kEdgeEmpty ? 1 : code == dragons::kEdgeKelp ? 2 : 3;
  }
  // s: 0 x (y flips), 1 y (x flips), 2 xy (both)
  int32_t MirrorTile(int s, int32_t t) const {
    const int32_t x = t % w_, y = t / w_;
    return Tile(s == 0 ? x : w_ - 1 - x, s == 1 ? y : h_ - 1 - y);
  }
  // the north side of (x, y) mirrors to the south side of the mirrored tile, which is the
  // north side of the tile below it; the west side likewise to the east side
  int32_t MirrorNorth(int s, int32_t t) const {
    const int32_t x = t % w_, y = t / w_;
    return Tile(s == 0 ? x : w_ - 1 - x, s == 1 ? y : h_ - y);
  }
  int32_t MirrorWest(int s, int32_t t) const {
    const int32_t x = t % w_, y = t / w_;
    return Tile(s == 0 ? x : w_ - x, s == 1 ? y : h_ - 1 - y);
  }
  void Kill(int s, int32_t round, bool own = true) {
    if (own) own_alive_ &= uint8_t(~(1 << s));
#ifdef SYM_TRACE
    if (Usable() >> s & 1) std::fprintf(stderr, "SYMWRONG r%d w%d h%d id%d team%d cand%d conf%d\n", round, w_, h_, id_, team_, s, conf_[s]);
    std::fprintf(stderr, "SYMKILL r%d id%d team%d cand%d\n", round, id_, team_, s);
#endif
    (void)round;
    alive_ &= uint8_t(~(1 << s));
  }
  void NoteEdge(bool north, int32_t t, int8_t kind, int32_t round) {
    std::vector<int8_t>& seen = north ? hk_ : vk_;
    if (seen[t] != 0) return;            // edges never change: only a first sighting adds evidence
    seen[t] = kind;
    for (int s = 0; s < 3; ++s) {
      if (!(alive_ >> s & 1)) continue;
      const int32_t m = north ? MirrorNorth(s, t) : MirrorWest(s, t);
      if (m == t || seen[m] == 0) continue;
      if (seen[m] != kind) Kill(s, round);
      else if (kind != 1) ++conf_[s];
    }
  }
  void FillEdge(int s, bool north, int32_t t, strat::BoardMemory& b) const {
    const int8_t kind = (north ? hk_ : vk_)[t];
    if (kind != 1 && kind != 2) return;   // unseen, or a portal
    const int32_t m = north ? MirrorNorth(s, t) : MirrorWest(s, t);
    int32_t& code = north ? (b.*Get(NorthTag{}))[m] : (b.*Get(WestTag{}))[m];
    if (code == kEdgeUnseenCode) code = kind == 1 ? dragons::kEdgeEmpty : dragons::kEdgeKelp;
  }
  void FillTile(int s, int32_t t, strat::BoardMemory& b, int32_t round) const {
    if (cd_[t] == kTileUnseen) return;
    const int32_t m = MirrorTile(s, t);
    if (m == t) return;
    const int32_t src = cdr_[t];
    strat::TileMemory& tm = (b.*Get(TilesTag{}))[m];
    if (src - 1 <= tm.seen_round) return;    // what we know there is as fresh
    if (cd_[t] < 0 && tm.seen_round >= 0) return;   // a plain tile: filled once
    if (cd_[m] != kTileUnseen) {
      // seen for real: only a stale bed without a remembered pearl takes the shared timer
      if (cd_[t] < 0 || tm.countdown < 0 || tm.pearl || cdr_[m] >= src) return;
    }
    (void)round;
    tm.seen_round = src - 1;
    tm.pearl = false;
    tm.countdown = cd_[t] >= 0 ? cd_[t] + 1 : -1;
    tm.occupant = -1;
    tm.occupant_head = false;
  }

  int32_t w_ = 0, h_ = 0, id_ = -1, team_ = -1;
  std::vector<int8_t> hk_, vk_;           // edges seen for real: 0 never, 1 empty, 2 kelp, 3 portal
  std::vector<int32_t> cd_, cdr_;         // tiles seen for real: countdown (-1 not a bed, -2 never seen), round
  uint8_t alive_ = 7, filled_ = 0, own_alive_ = 7, tiles_filled_ = 0;
  int32_t conf_[3] = {0, 0, 0};
};

}  // namespace symm
