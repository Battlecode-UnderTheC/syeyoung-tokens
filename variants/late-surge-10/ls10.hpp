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

namespace ls10 {
using namespace strat;

struct Params {
  // growth: split off a worker (the child takes the tail half)
  int32_t split_min_length = 4;     // at least this long ... (numbers win the head-to-head attrition: 4 beat 6, 8, 12 in simulation)
  int32_t split_until_round = 400;  // ... and before this round
  int32_t leader_split_until = 300; // leaders (alive at the start) stop splitting at this round, to grow
  int32_t split_headroom = 0;       // ... leaving this many dragons below the unit limit (to split out of traps)
  int32_t big_split_length = 0;     // unused
  // feeding: a worker gives its body to a bigger ally
  int32_t feed_from_round = 380;   // the surge: from here every dragon (leaders too) walks to the champion and dies next to it
  int32_t champ_freeze = 330;      // the champion stops splitting from here (it only grows)
  int32_t use_lunge = 1;            // quiet play: no multi-step strikes
  int32_t feed_distance = 3;        // dies when its head is this close to the ally's head (steps)
  int32_t ally_memory = 80;         // rounds an ally sighting stays worth walking to
  // consolidation: every dragon reports itself by sonar and relays the longest ally it knows
  // of; late in the game every other dragon walks to that one (the team's champion) and dies
  // next to it, so the mass ends up in one dragon
  int32_t report_memory = 40;       // rounds a sonar report of an ally stays usable
  double report_fade = 0.5;         // a report's weight when choosing the champion: its length less this per round of age
  int32_t leader_feed_from = 380;   // a leader that is not the champion feeds it from this round ...
  int32_t leader_feed_fresh = 20;   // ... when the champion was located at most this many rounds ago
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
  double contest_factor = 0.3;      // a target an enemy head in view gets to first (turn order breaks bed ties) is worth this much
  int32_t share_leader = 1;         // 0: leaders take any target (only workers give way)
  int32_t patrol = 1;
  int32_t turnout_budget = 3000;    // moves searched to find a way out of a small region the flood calls doomed (0: off)
  int32_t escape_steps = 1;
  int32_t doom_first = 0;
  int32_t avoid_blind = 1;          // 1: a step through a portal to a tile out of view counts as crowded
  int32_t doomed_split_room = 0;   // every step is a dead end with at most this many tiles: split now           // 1: avoid a dead end before avoiding enemy heads         // 1: judge steps by whether we can get back out (timed flood); 0: room only
  int32_t trap_small = 1;           // 1: dragons too short to split out of a dead end don't enter one               // no target: walk towards the tiles seen longest ago            // 1: prefer steps that leave a free tile no other head can take first          // 1: split only where both halves can get out
  // kamikaze (a head-to-head kills both dragons)
  int32_t hunt_radius = 0;          // workers chase enemy heads this many steps away ...
  int32_t hunt_min_units = 3;       // ... when we have at least this many dragons
  // A worker trades evenly when we have this many times the enemies it has seen. Off by
  // default (0): we see only a few of theirs, and in simulation these trades lost games.
  int32_t outnumber_factor = 0;
  int32_t trade_margin = 1;         // otherwise a worker trades only for an enemy this much longer than itself ...
  int32_t outnumbered_factor = 2;   // ... or, when they outnumber us, this many times longer (0: no such rule)
  int32_t enemy_memory = 60;        // rounds an enemy sighting counts towards their numbers
  // danger: an enemy head next to ours can step onto it and take both dragons
  int32_t avoid_heads = 1;          // 1: every dragon keeps off tiles next to enemy heads it won't trade with; 0: leaders only
  int32_t survivor_units = 0;       // down to this many dragons and outnumbered: flee enemy heads (0: off; fleeing lost games in simulation)
  // the lunge (from swarm-hunt-2): a MOVE of up to length-1 steps onto an enemy head worth a trade
  int32_t lunge_reach = 6;
  // late-surge-6: feeding only into a clearly longer ally
  int32_t trust_visible = 1;        // an ally whose whole body is in view has exactly that length (from late-surge-5)
  int32_t feed_clear_tenths = 8;    // feed only when our length < this/10 of the target's (known this turn)
  // reactive trading: when the enemy trades into us (an ally and an enemy head adjacent last
  // turn both vanish, or enemy heads keep stepping next to ours) and we aren't outnumbered,
  // trade evenly and hunt like they do; otherwise stay quiet. The flag spreads by sonar (bit 0).
  int32_t trader_trades = 1;        // head-to-head trades seen that set the flag ...
  int32_t trader_approaches = 3;    // ... or enemy heads stepping next to one of ours
  int32_t trader_trade_margin = 0;
  int32_t trader_hunt_radius = 6;
  // late walk (from late-surge-7): from this round, a feeder at least this long with no known
  // path to the champion walks towards its reported area
  int32_t converge_from = 440;
  int32_t converge_min_length = 10;
  // late-surge-10: unknown portals. An explorer beside a portal whose far end it hasn't seen
  // pings through it (only that ping that turn) and steps through next turn if the echo shows
  // no dragon on the line; a loop breaker bans an explore goal we keep circling.
  int32_t portal_cross = 1;
  int32_t portal_ban = 30;          // rounds an unknown portal with a dragon echo gets no explore value
  int32_t loop_window = 12;         // the last this many head tiles ...
  int32_t loop_distinct = 6;        // ... cover at most this many tiles, nothing eaten: a loop
  int32_t loop_ban = 0;             // rounds the goal we were circling is ignored (0: off; 50 lost 40-52 vs gather-split-trade-31)
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
  struct Report {
    int32_t id = -1, tile = -1, length = 0, round = -1;
  };
  uint64_t Encode(const Report& r) const;
  void Decode(uint64_t word);
  void Note(const Report& r);
  Report Champion() const;                              // the longest ally known (maybe us)
  struct Goal {
    int32_t tile = -1;
    bool trap = false;              // in a dead end worth entering (its gain beats trap_cost)
    bool explore = false;           // worth it only as the edge of the known map (or patrol)
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
  int TurnOut(const std::vector<int32_t>& body, int32_t max_depth) const;
  bool Lunge(bot::Action* out) const;
  bot::Action Decide(const rl::Observation& o);
#ifdef LS10_DEBUG
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
  std::unordered_map<int32_t, Report> reports_;         // allies (and us) by id, newest report
  const char* reason_ = "";
  // reactive trading
  bool trader_ = false;                                 // the enemy trades into us (seen or heard)
  int32_t trades_seen_ = 0, approaches_seen_ = 0;
  struct HeadSeen {
    int32_t id, tile;
    uint8_t team;
  };
  std::vector<HeadSeen> prev_heads_;                    // heads in view last turn (us included)
  int32_t prev_round_ = -2;
  bool Trading() const { return trader_ && units_ >= EnemiesSeen(); }
  int32_t TradeMargin() const { return Trading() ? p_.trader_trade_margin : p_.trade_margin; }
  int32_t HuntRadius() const { return Trading() ? p_.trader_hunt_radius : p_.hunt_radius; }
  void WatchTrades(const rl::Observation& o);
  // late-surge-10: portal probing and the loop breaker
  bool Banned(int32_t t) const {
    const auto it = banned_.find(t);
    return it != banned_.end() && it->second > round_;
  }
  bool PortalOk(int32_t t, int dir) const {  // explore value through this side (seen tile: an unknown portal)
    const auto it = banned_portal_.find(t * 4 + dir);
    return it == banned_portal_.end() || it->second <= round_;
  }
  bool BodyInView() const;
  std::unordered_map<int32_t, int32_t> banned_;         // goal tile -> banned until round
  std::unordered_map<int32_t, int32_t> banned_portal_;  // tile*4+dir -> banned until round
  int32_t probe_tile_ = -1, probe_dir_ = -1, probe_round_ = -10;
  bool probe_clear_ = false;                            // last turn's single ping hit no dragon
  int32_t goal_ = -1;                                   // this turn's goal
  bool goal_explore_ = false;
  int32_t last_goal_ = -1;
  bool last_goal_explore_ = false;
  int32_t prev_length_ = 0;
  std::deque<std::pair<int32_t, bool>> recent_;         // (head tile, ate) of the last turns
#ifdef LS10_LOOPSTAT
  std::deque<std::pair<int32_t, bool>> stat_recent_;
#endif
};

}  // namespace ls10
