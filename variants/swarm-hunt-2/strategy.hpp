// The scripted strategy: explore, gather pearls with many small dragons, feed
// them into a few big ones late in the game, and trade dragons head-to-head when
// the numbers favour us.
//
// Roles: a dragon alive at the start of the game (it first acts in round 0) is a
// LEADER: it gathers, splits off workers early, and never gives itself away. A
// split child is a WORKER: it gathers, hunts enemy heads when we have dragons to
// spare, and late in the game walks to a bigger ally and dies next to it (a death
// leaves pearls on every second segment, which the ally eats: the only way to
// "combine" dragons in this game).
//
// Every decision rule reads its thresholds from Params; nothing else is tuned.
#pragma once

#include <cstdint>
#include <deque>
#include <unordered_map>
#include <vector>

#include "agent.hpp"
#include "board_memory.hpp"
#include "pathing.hpp"

namespace swh2 {
using namespace strat;

struct Params {
  // growth: split off a worker (the child takes the tail half)
  int32_t split_min_length = 4;     // at least this long ... (numbers win the head-to-head attrition: 4 beat 6, 8, 12 in simulation)
  int32_t split_until_round = 300;  // ... and before this round
  int32_t leader_split_until = 300; // leaders (alive at the start) stop splitting at this round, to grow
  int32_t split_headroom = 0;       // ... leaving this many dragons below the unit limit (to split out of traps)
  int32_t big_split_length = 0;     // unused
  // feeding: a worker gives its body to a bigger ally
  int32_t feed_from_round = 400;
  int32_t feed_distance = 2;        // dies when its head is this close to the ally's head (steps)
  int32_t ally_memory = 80;         // rounds an ally sighting stays worth walking to
  // targets: each tile's value per round it takes to collect it (see TileValue)
  double pearl_decay = 0.97;        // per round since a pearl was seen: the chance it is still there
  double ripe_bed_value = 0.6;      // a bed whose countdown ran out out of view (grown, unless eaten)
  double explore_value = 0.25;      // the edge of the known map
  // dead ends: entering one costs a split to get out (the stub dies) and the time
  double trap_cost = 3.0;
  int32_t trap_candidates = 8;      // the best targets checked for being dead ends
  // safety
  int32_t room_margin = 3;          // a step must leave at least length + this many tiles reachable ...
  int32_t room_min = 8;             // ... and never fewer than this (short dragons get boxed in too)
  int32_t escape_slack = 12;        // no way back out, but this many tiles more than twice our length: fine
  int32_t escape_min = 24;
  int32_t check_split = 1;
  int32_t liberties = 1;
  double share_factor = 0.1;        // a target an ally in view is nearer to is worth this much (1: ignore allies)
  int32_t share_leader = 1;         // 0: leaders take any target (only workers give way)
  int32_t patrol = 1;
  int32_t turnout_budget = 3000;    // moves searched to find a way out of a small region the flood calls doomed (0: off)
  int32_t escape_steps = 1;
  int32_t doom_first = 0;
  int32_t avoid_blind = 1;          // 1: a step through a portal to a tile out of view counts as crowded
  int32_t doomed_split_room = 0;   // every step is a dead end with at most this many tiles: split now           // 1: avoid a dead end before avoiding enemy heads         // 1: judge steps by whether we can get back out (timed flood); 0: room only
  int32_t trap_small = 1;           // 1: dragons too short to split out of a dead end don't enter one               // no target: walk towards the tiles seen longest ago            // 1: prefer steps that leave a free tile no other head can take first          // 1: split only where both halves can get out
  // kamikaze (a head-to-head kills both dragons)
  int32_t hunt_radius = 6;          // workers chase enemy heads this many steps away ...
  int32_t hunt_min_units = 3;       // ... when we have at least this many dragons
  // A worker trades evenly when we have this many times the enemies it has seen. Off by
  // default (0): we see only a few of theirs, and in simulation these trades lost games.
  int32_t outnumber_factor = 0;
  int32_t trade_margin = 0;         // otherwise a worker trades only for an enemy this much longer than itself ...
  int32_t outnumbered_factor = 2;   // ... or, when they outnumber us, this many times longer (0: no such rule)
  int32_t enemy_memory = 60;        // rounds an enemy sighting counts towards their numbers
  // danger: an enemy head next to ours can step onto it and take both dragons
  int32_t avoid_heads = 1;          // 1: every dragon keeps off tiles next to enemy heads it won't trade with; 0: leaders only
  int32_t survivor_units = 0;
  // the lunge: a MOVE of up to length-1 steps reaches an enemy head before it can react
  int32_t lunge_reach = 6;          // at most this many steps (bounds the search)
  int32_t keeper_guard = 1;         // leaders keep this many steps from enemy heads they won't trade with
  int32_t threat_budget = 400;       // tiles searched per enemy head for the guard       // down to this many dragons and outnumbered: flee enemy heads (0: off; fleeing lost games in simulation)
};

class StrategyAgent : public bot::Agent {
 public:
  StrategyAgent(const rl::Spawn& spawn, Params params = {});
  bot::Action Turn(const rl::Observation& o) override;
  // why the last action was taken (for the simulator's tallies and debugging)
  const char* reason() const override { return reason_; }

 private:
  struct Sighting {
    int32_t head = -1;              // tile of its head
    int32_t round = -1;
    int32_t length = 0;             // segments seen (a lower bound)
    uint8_t team = 0;
  };
  enum class Cell { kFree, kWall, kUnknown, kBody, kEnemyHead, kAllyHead };

  void Remember(const rl::Observation& o);
  Cell Classify(int32_t tile) const;
  bool Occupied(int32_t tile) const;                    // a dragon on it right now (as far as we know)
  bool Mine(int32_t tile) const;                        // one of this dragon's own segments
  bool WorthTrading(int32_t enemy_id) const;            // a head-to-head with it pays off
  bool Exposed(int32_t tile) const;                     // an enemy head we won't trade with could step onto it
  int FleeDirection() const;                            // the step furthest from the enemy heads seen (-1: none seen)
  int32_t EnemiesSeen() const;
  const Sighting* BiggestAlly() const;
  struct Goal {
    int32_t tile = -1;
    bool trap = false;              // in a dead end worth entering (its gain beats trap_cost)
  };
  // what collecting tile t is worth (0..1) and when it can be collected, arriving in `dist` rounds
  double TileValue(int32_t t, int32_t dist, int32_t* ready) const;
  int32_t RoomNeeded() const;
  Goal ChooseGoal(const Paths& paths) const;            // `paths` searched from the head
  bot::Action SafeStep(int preferred_dir, bool trap_ok); // the preferred step if safe, else the roomiest
  // Whether a dragon whose body is `body` (head first) can keep moving: a timed flood from
  // its head in which its own segments free up as its tail moves on. 2: it reaches a freed
  // segment of its own (it can follow its tail) or the unexplored edge; 1: at least `cap`
  // tiles; 0: doomed. *count: the tiles reached (up to cap).
  int EscapeLevel(const std::vector<int32_t>& body, int32_t cap, int32_t* count = nullptr,
                  const std::vector<int32_t>* blocked = nullptr) const;
  bool SplitSafe(int32_t n) const;
  bool Lunge(bot::Action* out) const;
  void MarkThreats();
  int TurnOut(const std::vector<int32_t>& body, int32_t max_depth) const;
#ifdef SWH2_DEBUG
  bot::Action TurnImpl(const rl::Observation& o);
  void Dump(const char* why) const;
#endif
  int Liberties(int32_t to, const std::vector<int32_t>& body) const;
  int32_t EscapeCap(int32_t length) const { return std::max(2 * length + p_.escape_slack, p_.escape_min); }
  bot::Action Because(const char* why, bot::Action a) {
    reason_ = why;
    return a;
  }

  Params p_;
  rl::Spawn spawn_;
  BoardMemory board_;
  bool leader_ = false;
  bool first_turn_ = true;
  int32_t born_ = 0;
  // this turn
  int32_t head_ = 0, length_ = 0, units_ = 1, round_ = 0;
  std::unordered_map<int32_t, Sighting> seen_;          // other dragons by id
  // this dragon's body, head first: always the last `length` tiles its head stood on
  // (eating keeps the tail, a split cuts it), so it is known even out of view, e.g.
  // on the far side of a portal it just came through
  std::deque<int32_t> body_;
  std::vector<uint8_t> threat_;     // leaders: tiles an enemy head could lunge onto next turn
  const char* reason_ = "";
};

}  // namespace swh2
