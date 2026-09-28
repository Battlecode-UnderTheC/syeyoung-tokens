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

#include "agent.hpp"
#include "board_memory.hpp"
#include "pathing.hpp"

namespace cs5 {
using namespace strat;

struct Params {
  // growth: split off a worker (the child takes the tail half)
  int32_t split_min_length = 4;     // at least this long ... (numbers win the head-to-head attrition: 4 beat 6, 8, 12 in simulation)
  int32_t split_until_round = 300;  // ... and before this round
  int32_t leader_split_until = 300; // a leader (alive at the start) stops splitting here and grows
  // feeding: a worker gives its body to a bigger ally
  int32_t feed_from_round = 340;
  int32_t feed_distance = 2;        // dies when its head is this close to the ally's head (steps)
  int32_t ally_memory = 200;        // rounds an ally sighting stays worth walking to
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
  int32_t strike_reach = 5;
  int32_t snipe_memory = 0;         // rounds a big enemy sighting stays worth chasing
  double snipe_factor = 2;          // a big enemy: at least this many times our length
  double patrol_value = 0;          // a tile unseen for patrol_age rounds, from patrol_from on
  int32_t patrol_from = 300, patrol_age = 40;
  int32_t report_memory = 40;       // rounds a sonar report of an ally stays usable
  int32_t leader_feed_from = 440;   // a leader that is not the champion feeds it from this round ...
  int32_t leader_feed_fresh = 10;   // ... when the champion was located at most this many rounds ago         // multi-step strikes onto enemy heads up to this many steps away
  double x1 = 0.9, x2 = 0, x3 = 8;  // x2,x3: snipe length, radius  // x1: a pearl an ally head is closer to is worth this much less
  int32_t min_libs = 2;             // the goal step must leave this many (half-)liberties       // down to this many dragons and outnumbered: flee enemy heads (0: off; fleeing lost games in simulation)
};

class StrategyAgent : public bot::Agent {
 public:
  StrategyAgent(const rl::Spawn& spawn, Params params = {});
  bot::Action Turn(const rl::Observation& o) override;
  bot::Action Decide(const rl::Observation& o);
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
  struct Goal {
    int32_t tile = -1;
    bool trap = false;              // in a dead end worth entering (its gain beats trap_cost)
  };
  // what collecting tile t is worth (0..1) and when it can be collected, arriving in `dist` rounds
  double TileValue(int32_t t, int32_t dist, int32_t* ready) const;
  int32_t RoomNeeded() const;
  Goal ChooseGoal(const Paths& paths) const;            // `paths` searched from the head
  int32_t Liberties(int32_t to) const;
  int32_t Torus(int32_t a, int32_t b) const;
  std::vector<int> StrikePath() const;
  bot::Action SafeStep(int preferred_dir, bool trap_ok); // the preferred step if safe, else the roomiest
  bot::Action Because(const char* why, bot::Action a) {
    reason_ = why;
    return a;
  }

  Params p_;
  rl::Spawn spawn_;
  BoardMemory board_;
  bool leader_ = false;
  bool first_turn_ = true;
  // this turn
  int32_t head_ = 0, length_ = 0, units_ = 1, round_ = 0;
  std::unordered_map<int32_t, Sighting> seen_;          // other dragons by id
  // this dragon's body, head first: always the last `length` tiles its head stood on
  // (eating keeps the tail, a split cuts it), so it is known even out of view, e.g.
  // on the far side of a portal it just came through
  std::deque<int32_t> body_;
  const char* reason_ = "";
  // champion over sonar (from gather-split-trade-4): every dragon pings the longest ally it
  // knows of (itself included); receivers keep the newest report per id and relay the best
  struct Report {
    int32_t id = -1, tile = -1, length = 0, round = -1;
  };
  std::unordered_map<int32_t, Report> reports_;
  uint64_t Encode(const Report& r) const;
  void Decode(uint64_t word);
  Report Champion() const;
};

}  // namespace cs5
