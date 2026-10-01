// The dragons game, ported from the official engine (github.com/unswcpmsoc/unswbc,
// engine/src, as shipped in unswbc==1.0.0, protocol 2.1.0), with the 1.0.0
// sonar: four directional pings per turn, uint64 words, and the ECHOES line,
// brought up to unswbc 1.2.3 ("Slay Queen", 2026-10-01): dragons 0 and 1 are the
// queens, the round-limit ranking puts the queen's length first, a move's first
// ceil(L/4) steps are free, and a map's DRAGON lines must alternate teams. The
// observation and the bot protocol did not change in 1.2.3.
//
// The rules are the engine's, function for function; what differs is the data
// layout, chosen so a turn allocates nothing and touches only what it needs:
//
//   * every tile knows, per direction, the tile a step lands on (kelp -> -1,
//     portals resolved) -- the engine's TileAfterStep, computed once per map;
//   * an occupancy grid names the living dragon on each tile, replacing the
//     engine's scan over every body (living bodies never overlap, so the grid
//     is exact);
//   * a body is a ring buffer of tile indices addressed by a logical position
//     that grows toward the head, so a tile's segment index is `front - pos`.
//
// Dragon ids equal their index in `dragons`: the engine hands out ids in the
// order dragons are appended, at load and on every split.
#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

namespace dragons {

inline constexpr char kProtocolVersion[] = "2.1.0";
inline constexpr int kVision = 7;
inline constexpr int kVisionRadius = kVision / 2;
inline constexpr int kVisionTiles = kVision * kVision;
inline constexpr int kMinDragonLength = 2;
inline constexpr int kDefaultUnitLimit = 64;
inline constexpr int kMaxRounds = 500;
inline constexpr int kMaxMapSide = 256;
inline constexpr uint32_t kPearlRngSeed = 0x5eed5eed;
// The full-state summary (training critics only) splits the board into this
// many blocks a side.
inline constexpr int kBlocks = 16;

// N E S W, the engine's DirectionOfStepBetween order
inline constexpr char kDirChar[4] = {'N', 'E', 'S', 'W'};
inline constexpr int kDx[4] = {0, 1, 0, -1};
inline constexpr int kDy[4] = {-1, 0, 1, 0};

enum class Symmetry : uint8_t { None, X, Y, XY };

enum DeathReason : uint8_t { kHitWall = 0, kHitSelf, kHitOtherBody, kHitHeadToHead, kNoValidAction };
inline constexpr char kDeathChar[5] = {'W', 'S', 'O', 'H', 'A'};

// Edge codes as a bot sees them: '.', 'w' or the portal id.
inline constexpr int32_t kEdgeEmpty = -1;
inline constexpr int32_t kEdgeKelp = -2;

enum ActionKind : uint8_t { kMove = 0, kSplit = 1, kNoAction = 2 };

// A reply's sonar (unswbc 1.0.0). `SONAR <dir> <uint64>` fills that direction's
// slot, the last line per direction winning. The bare `SONAR <uint64>` fills
// the slot of the facing *after* the action, unless that direction was named
// explicitly; it is the old uint32 form: the engine reads it as strtoull does
// (a sign allowed, negatives wrapping, overflow saturating) and sends the low
// 32 bits. Pings are cast N, E, S, W, whatever order the lines came in.
struct Sonar {
  uint8_t mask = 0;             // bit d: direction d named explicitly
  uint64_t word[4] = {0, 0, 0, 0};
  bool bare = false;
  uint64_t bare_word = 0;
};

// What one ping hit (the engine's CastSonar return value), and the ECHOES
// fields it is counted in: kelp ally ally_head enemy enemy_head.
enum Echo : uint8_t { kEchoKelp = 0, kEchoAlly, kEchoAllyHead, kEchoEnemy, kEchoEnemyHead, kEchoes, kEchoNothing = kEchoes };
// A dragon whose last `PROTOCOL n` had n >= this sees the ECHOES line, and is
// the only kind that receives words wider than uint32: a wider ping that hits
// any other dragon is still cast and echoed, but its word is dropped.
inline constexpr int kEchoProtocol = 3;

struct Dragon {
  int32_t id = 0;
  uint8_t team = 0;             // 0 = A, 1 = B
  uint8_t facing = 0;
  bool alive = true;
  int32_t length = 0;
  uint32_t front = 0;           // logical position of the head
  uint32_t mask = 0;            // ring.size() - 1
  std::vector<int32_t> ring;    // tile indices; segment s at (front - s) & mask
  std::vector<uint64_t> inbox;
  int32_t turns = 0;            // turns this dragon has taken
  // The last `PROTOCOL n` it declared (0: none); a split child inherits it.
  int32_t protocol = 0;
  // What its pings hit on its last turn, per Echo field; reset every turn.
  int32_t echo[kEchoes] = {0, 0, 0, 0, 0};
  bool echoes() const { return protocol >= kEchoProtocol; }

  int32_t Seg(int s) const { return ring[(front - static_cast<uint32_t>(s)) & mask]; }
  int32_t Head() const { return Seg(0); }
  int32_t Tail() const { return Seg(length - 1); }
};

struct Death {
  int32_t id;
  uint8_t team;
  uint8_t reason;
  int32_t round;
  int32_t by = -1;              // hit another's body: that dragon's id (-1 otherwise)
};

// A team's living dragons. queen: the length of its queen (dragon 0 or 1),
// 0 once the queen is dead. The round-limit ranking is (queen, longest, total).
struct Standing {
  int32_t dragons = 0, longest = 0, total = 0, queen = 0;
};

struct Result {
  bool terminated = false;
  uint8_t end_reason = 1;       // 0 a team eliminated, 1 round limit (engine GameEndReason)
  int8_t winner = -1;           // -1 draw, 0 A, 1 B
  Standing team[2];
};

// Everything fixed by the map: parsed once, shared by every game on it.
struct Board {
  int32_t width = 0, height = 0, unit_limit = kDefaultUnitLimit;
  Symmetry symmetry = Symmetry::None;
  std::vector<uint8_t> spawns;          // tile spawns pearls
  std::vector<int32_t> min_gap, max_gap;
  std::vector<int32_t> step_to;         // [tile * 4 + dir] -> tile, or -1 for kelp
  std::vector<int32_t> north_code, west_code;   // edge codes of each tile's north and west sides
  std::vector<int32_t> pearl_beds;      // (bed, mirror) pairs, row-major over the beds owning a countdown
  std::vector<Dragon> dragons;          // at round 0
  std::string text;                     // the map as given
  // full-state summary: each tile's block, and the fixed per-block counts
  std::vector<uint8_t> block_of;        // [tile] -> by * kBlocks + bx
  std::vector<int32_t> block_beds, block_kelp;   // [kBlocks^2]; kelp counts north and west sides

  int32_t Tile(int x, int y) const { return y * width + x; }
  int32_t Wrap(int x, int y) const {
    x %= width; if (x < 0) x += width;
    y %= height; if (y < 0) y += height;
    return y * width + x;
  }
};

// engine: LoadMap. Throws std::runtime_error with the engine's message.
std::shared_ptr<const Board> LoadBoard(const std::string& map_text);

// What changes on a tile, in one 16-byte record: an observation reads 7 rows
// of 7 tiles, and one record per tile keeps that to ~2 cache lines a row.
struct Cell {
  int32_t occ = -1;         // living dragon on the tile, -1
  uint32_t pos = 0;         // that segment's logical position in the dragon's ring
  int32_t pearl_base = 0;   // countdown = pearl_base - round (pearl beds only)
  uint8_t pearl = 0;
  // for a body segment other than the head: the engine's facing of it,
  // DirectionOfStepBetween(segment, the next one toward the head) -- fixed
  // while the segment lives, so computed once when it is laid down
  uint8_t dir = 0;
  uint8_t pad[2] = {0, 0};
};

class Game {
 public:
  Game() = default;
  explicit Game(std::shared_ptr<const Board> board) { Reset(std::move(board)); }

  // A fresh match on `board`: pearl countdowns drawn, round 0 begun, the first
  // dragon to act selected.
  void Reset(std::shared_ptr<const Board> board);

  bool done() const { return done_; }
  int32_t round() const { return round_; }
  int32_t actor() const { return actor_; }            // index of the dragon to act, -1 when done
  const Result& result() const { return result_; }
  const Board& board() const { return *board_; }
  const std::vector<Dragon>& dragons() const { return dragons_; }
  int32_t alive_count(int team) const { return alive_[team]; }
  bool pearl(int32_t tile) const { return cells_[tile].pearl != 0; }
  // the engine's mNextPearl: the countdown a bot sees, -1 on tiles that never spawn
  int32_t next_pearl(int32_t tile) const { return board_->spawns[tile] ? cells_[tile].pearl_base - round_ : -1; }
  int32_t occupant(int32_t tile) const { return cells_[tile].occ; }
  // full-state summary: pearls and each team's body segments per block
  int32_t block_pearls(int b) const { return block_pearls_[b]; }
  int32_t block_body(int team, int b) const { return block_body_[team * kBlocks * kBlocks + b]; }
  uint32_t occ_pos(int32_t tile) const { return cells_[tile].pos; }   // logical position of the segment there
  const struct Cell& cell(int32_t tile) const { return cells_[tile]; }
  // deaths since the last ClearDeaths, in the order the engine reports them
  const std::vector<Death>& deaths() const { return deaths_; }
  void ClearDeaths() { deaths_.clear(); }

  // The actor's turn (engine: Game::TakeTurn after the reply is read): the
  // inbox and echo counts are emptied, a declared protocol (>= 0) recorded,
  // the action applied, and the pings cast if the dragon survived.
  // An empty move or kNoAction is the engine's "no valid action" death.
  void Act(uint8_t kind, const uint8_t* steps, int nsteps, int32_t split, const Sonar& sonar,
           int32_t protocol = -1);

  // engine: DirectionOfStepBetween
  int DirectionOfStep(int32_t from, int32_t to) const {
    const int32_t* s = &board_->step_to[from * 4];
    for (int d = 0; d < 4; ++d) if (s[d] == to) return d;
    return -1;
  }

  // Software prefetch for stepping many games in a row: stage 0 the actor's
  // record, 1 its body ring, 2 the rows of its window (`radius` rows around
  // the head). Each stage reads only what the previous one fetched.
  void Prefetch(int stage, int radius) const {
    if (actor_ < 0) return;
    const Dragon& d = dragons_[actor_];
    if (stage == 0) {
      __builtin_prefetch(&d);
      __builtin_prefetch(reinterpret_cast<const char*>(&d) + 64);
      return;
    }
    if (stage == 1) {
      __builtin_prefetch(&d.ring[d.front & d.mask]);
      return;
    }
    const Board& b = *board_;
    const int head = d.Head(), hx = head % b.width, hy = head / b.width;
    int x0 = hx - kVisionRadius;
    x0 += x0 < 0 ? b.width : 0;
    for (int dy = -radius; dy <= radius; ++dy) {
      int y = hy + dy;
      y += y < 0 ? b.height : 0;
      y -= y >= b.height ? b.height : 0;
      const char* row = reinterpret_cast<const char*>(&cells_[y * b.width + x0]);
      __builtin_prefetch(row);
      __builtin_prefetch(row + 64);
      __builtin_prefetch(row + 112);
    }
  }

  // The judge's text, byte for byte (engine: BuildInitBlock / BuildRoundBlock).
  // Written the slow, literal way; the numeric observation is checked against it.
  std::string InitBlock(int32_t dragon) const;
  std::string RoundBlock() const;

 private:
  void Advance();
  void BeginRound();
  void Step(int32_t d, int dir, bool must_pay);
  void Move(int32_t d, const uint8_t* steps, int n);
  void Split(int32_t d, int32_t child_segments);
  void Kill(int32_t d, uint8_t reason, int32_t by = -1);   // by: the dragon index whose body was hit
  uint8_t CastSonar(int32_t d, int dir, uint64_t value);
  void PushFront(int32_t d, int32_t tile);
  void PopBack(int32_t d);
  void LayBody(int32_t d);
  void SetPearl(int32_t tile, uint8_t v) {
    block_pearls_[board_->block_of[tile]] += static_cast<int32_t>(v) - cells_[tile].pearl;
    cells_[tile].pearl = v;
  }
  int32_t DrawGap(int32_t bed);
  void SetCountdown(uint32_t order, int32_t gap, int32_t from_round);
  Result ResultAfterRound(bool final_round) const;

  std::shared_ptr<const Board> board_;
  std::vector<Dragon> dragons_;
  // A bed's countdown is `pearl_base - round`. Rather than ticking every bed
  // every round, each waits in the bucket of the round it fires (a linked list
  // through bucket_next_), and a round's bucket is sorted back into the
  // row-major order in which the engine's tick draws the gaps.
  std::vector<Cell> cells_;
  std::vector<int32_t> block_pearls_;  // [kBlocks^2]
  std::vector<int32_t> block_body_;    // [2][kBlocks^2]
  std::vector<int32_t> bucket_head_;   // [kMaxRounds] first bed order due that round, -1
  std::vector<int32_t> bucket_next_;   // [beds]
  std::vector<uint32_t> due_now_;      // scratch
  std::vector<Death> deaths_;
  std::mt19937 rng_{kPearlRngSeed};
  int32_t alive_[2] = {0, 0};
  int32_t round_ = 0;
  int32_t actor_ = -1;
  bool done_ = true;
  Result result_;

};

}  // namespace dragons
