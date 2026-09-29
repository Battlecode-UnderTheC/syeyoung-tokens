// counter-swarm-19 (crossover counter-swarm-17 x gather-split-trade-34): gst34's whole movement
// and goal layer (SafeStep escape levels + TurnOut, dead ends worth entering, contest/share
// goals, sonar-checked portal crossing, loop breaker) under counter-swarm-17's identity: its
// multi-step strikes (longest target in reach, the champion strikes too), trade rules, hunting
// cut off at 300, the champion guard from 340 (out of every enemy head's lunge reach) and the
// champion finder for an out-of-view champion.
//
// (gst34's header:) The scripted strategy: explore, gather pearls with many small dragons, feed
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
#include <unordered_set>
#include <vector>

#include "agent.hpp"
#include "board_memory.hpp"
#include "pathing.hpp"

namespace cs38 {
using namespace strat;

struct Params {
  // growth: split off a worker (the child takes the tail half)
  int32_t split_min_length = 4;     // at least this long ... (numbers win the head-to-head attrition: 4 beat 6, 8, 12 in simulation)
  int32_t split_until_round = 400;  // ... and before this round
  int32_t leader_split_until = 300; // leaders (alive at the start) stop splitting at this round, to grow
  int32_t split_headroom = 0;       // ... leaving this many dragons below the unit limit (to split out of traps)
  int32_t big_split_length = 0;     // unused
  // (g20-6, strategist-a P2) skip a routine growth split when the 7x7 window right now shows
  // this many enemy heads or more and at most this many allies: consolidating mass instead of
  // matching a local swarm we can't outnumber. 0 disables the check (matches parent behaviour).
  int32_t local_outnumber_min_enemies = 3;
  int32_t local_outnumber_max_allies = 1;
  // feeding: a worker gives its body to a bigger ally
  int32_t feed_from_round = 340;   // late-surge endgame (g20-6, strategist-a P1: swept from 380;
                                    // a full sweep to 300 -- matching leader_split_until -- measured
                                    // worse on official maps than this smaller 40-round sweep)
  int32_t champ_freeze = 320;      // the champion stops splitting from here (it only grows);
                                    // swept from 330 to start together with the earlier feed
  int32_t feed_distance = 3;        // dies when its head is this close to the ally's head (steps)
  int32_t ally_memory = 80;         // rounds an ally sighting stays worth walking to
  // consolidation: every dragon reports itself by sonar and relays the longest ally it knows
  // of; late in the game every other dragon walks to that one (the team's champion) and dies
  // next to it, so the mass ends up in one dragon
  int32_t report_memory = 40;       // rounds a sonar report of an ally stays usable
  double report_fade = 0.5;         // a report's weight when choosing the champion: its length less this per round of age
  // counter-swarm-20 (from gather-split-trade-38): in the endgame a far champion's relayed reports
  // arrive old: keep them longer and fade them slower, so remote dragons stop electing themselves
  int32_t late_report_from = 330;
  int32_t late_report_memory = 80;
  double late_report_fade = 0.25;
  // a partial sighting of an ally is only a lower bound on its length: it moves the report's
  // position and round on, but never shrinks the length an exact (self) report gave
  int32_t lower_bound_reports = 1;
  int32_t leader_feed_from = 340;   // a leader that is not the champion feeds it from this round ...
                                    // (g20-6: swept 380->340 together with feed_from_round/champ_freeze)
  int32_t leader_feed_fresh = 20;   // ... when the champion was located at most this many rounds ago
  // targets: each tile's value per round it takes to collect it (see TileValue)
  double pearl_decay = 0.97;        // per round since a pearl was seen: the chance it is still there
  double ripe_bed_value = 0.6;      // a bed whose countdown ran out out of view (grown, unless eaten)
  double explore_value = 0.25;      // the edge of the known map
  // late-surge-17 port: a round spent waiting on a bed costs this many steps
  // (0: parent's max(dist, due)); a candidate gains cluster_bonus x the pearls/beds
  // within cluster_radius, each weighted by (d+1)/(r2+1) so a bed far from ripe adds little.
  double wait_factor = 2.0;
  double cluster_bonus = 0.5;
  int32_t cluster_radius = 2;
  // body-wall-17 port (g15-26): on small maps the swarm takes the central field first and
  // out-eats us in the opening; before centre_until, on maps of at most centre_max_tiles
  // tiles, a candidate (pearl, bed or the unexplored edge) gains up to centre_bonus times
  // its score the nearer it is to the map centre (torus distance; 0 at the farthest tile).
  int32_t centre_max_tiles = 1024;
  int32_t centre_until = 50;
  double centre_bonus = 0.3;
  // dead ends: entering one costs a split to get out (the stub dies) and the time
  double trap_cost = 3.0;
  int32_t trap_candidates = 8;      // the best targets checked for being dead ends
  // g19-5 (ported from gather-split-trade-96, g18-3): once the tail-following escape test
  // (EscapeLevel) has already ruled a pocket a true dead end, price it by expected loss rather
  // than the flat trap_cost: a split-out stub costs trap_stub, and a 2-3-long dragon may still
  // enter when the pocket's pearls would grow it to >= 4 with a unit slot free to split with.
  int32_t trap_loss = 1;
  double trap_stub = 2.0;
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
  int32_t trade_margin = 0;         // otherwise a worker trades only for an enemy this much longer than itself ...
  int32_t outnumbered_factor = 2;   // ... or, when they outnumber us, this many times longer (0: no such rule)
  int32_t enemy_memory = 60;        // rounds an enemy sighting counts towards their numbers
  // danger: an enemy head next to ours can step onto it and take both dragons
  int32_t avoid_heads = 1;          // 1: every dragon keeps off tiles next to enemy heads it won't trade with; 0: leaders only
  int32_t survivor_units = 0;       // down to this many dragons and outnumbered: flee enemy heads (0: off; fleeing lost games in simulation)
  // the lunge (from swarm-hunt-2): a MOVE of up to length-1 steps onto an enemy head worth a trade
  int32_t lunge_reach = 6;          // at most this many steps (bounds the search)
  // the unit cap: a SPLIT with no free slot kills the splitter, so near the cap a trapped
  // dragon can't split out of a dead end and resigns whole
  // late-surge-6: feeding only into a clearly longer ally
  int32_t trust_visible = 1;        // an ally whose whole body is in view has exactly that length
  int32_t feed_clear_tenths = 8;    // feed only when our length < this/10 of the target's (known this turn)
  int32_t cap_safety = 1;           // 1: with fewer than cap_margin free slots, a dragon plans as if it can't split out
  int32_t cap_margin = 2;           // free slots needed to count on a split escape (other dragons split the same round)
  int32_t reserve_slots = 0;        // growth splits leave this many slots free for trapped long dragons ...
  int32_t reserve_length = 6;       // ... this long (shorter ones split out only above the reserve)
  // portals (cs38): step through an unknown portal next to the head when only exploring,
  // after a sonar ping through it echoed clear the turn before
  int32_t portal_cross = 1;
  int32_t cross_until = 480;        // ... before this round
  int32_t portal_explore = 1;       // 1: an unknown portal gives explore value only when we would cross it
  // gather-split-trade-42: a KNOWN portal is not a one-way door either: a blind goal step onto
  // one is lifted when a ping through it comes back clean, its exit was seen empty recently, or
  // the same goal's blind step was refused blind_fallback times running
  int32_t known_portal = 1;         // (cs38) lift a known portal's blind-step cap per the rule above
  int32_t blind_fallback = 2;       // (cs38) accept a blind goal step after this many detours for the same goal (0: never)
  // body-wall-15 (replay study #20): a frozen champion never enters a dead end (it coiled into
  // its own pockets and trap-split, leaving a ghost champion for 10-20 rounds)
  int32_t champ_no_trap = 1;
  // body-wall-15 (replay study #19): cross an unknown portal only when this long and a split is
  // possible now (a 1-tile box past the exit is split out of: the child walks out the entry
  // side); this nearly switches the explore crossing off since most dragons split at length 4
  int32_t cross_min_length = 4;
  // loop breaker: the last loop_window heads on at most loop_distinct tiles, nothing eaten:
  // the goal is banned for loop_ban rounds
  int32_t loop_break = 1;
  int32_t loop_window = 12;
  int32_t loop_distinct = 6;
  int32_t loop_ban = 50;
  // gather-split-trade-39: pick the split point so that the ping we cast from our new tail
  // (opposite the facing) runs straight into the child's tail: the child starts knowing the champion
#ifdef CS38_OFF
  int32_t heard_split = 0;
#else
  int32_t heard_split = 1;
#endif
  // an ally at least this long (in view) seen beside our head last turn and gone now died: we
  // report it dead (length 0, this round) and relay that on alternate turns for obit_rounds, so
  // a dead champion's report stops drawing the team (late fade keeps it alive ~80 rounds)
#ifdef CS38_OFF
  int32_t obit_min_length = 9999;
#else
  int32_t obit_min_length = 5;
#endif
  int32_t obit_champion = 10;       // relay a death only of a dragon we knew this long
  int32_t obit_rounds = 20;
  // counter-swarm-17
  int32_t hunt_until = 300;         // hunting (hunt_radius) only before this round
  int32_t strike_reach = 5;         // multi-step strikes onto enemy heads up to this many steps away (replaces the lunge)
  int32_t guard_champion = 1;       // the fed champion keeps out of every enemy head's lunge reach (0: off) ...
  int32_t guard_from = 340;         // ... from this round
  int32_t guard_min_length = 8;     // ... when at least this long
  double guard_share = 0.6;         // ... also any dragon at least this share of the champion's length
  int32_t guard_cut_extra = 3;      // an enemy whose body runs out of view: this much more reach
  int32_t seek_champion = 0;        // a feeder whose champion is unreachable heads for the known tile nearest to it
  // (late-surge-23) a lunge guard for long dragons and the frozen champion: rank a step onto a
  // tile an enemy head could reach in one multi-step MOVE worse than a safe step, but better
  // than a sure trade next to the head
  int32_t guard_length = 10;        // long dragons guard against enemy lunges from this length (reuses lunge_reach above)
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
    bool cut = false;               // part of it on the window's edge: likely longer than seen
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
  bool LocallyOutnumbered() const;   // g20-6: 7x7-window density check for growth splits
  struct Report {
    int32_t id = -1, tile = -1, length = 0, round = -1;
    bool lower = false;   // the length is a lower bound (an ally seen only in part)
  };
  uint64_t Encode(const Report& r) const;
  void Decode(uint64_t word);
  void Note(const Report& r);
  Report Champion() const;                              // the longest ally known (maybe us)
  struct Goal {
    int32_t tile = -1;
    bool trap = false;              // in a dead end worth entering (its gain beats trap_cost)
    bool escapes = false;           // room falls short but the projected-body escape check passed:
                                     // not really a dead end (a corridor or a tail-following pocket)
    bool grow = false;              // (g19-5) too short to split now, but the pocket's pearls fix that
  };
  // (g18-5) the body after walking the shortest open-tile path from the head to `goal`,
  // truncated/extended to length_ tiles by the old tail: what EscapeLevel should see to judge
  // whether the dragon can still get out once it stands on `goal`. Backtracks through `paths`
  // (already computed for the whole turn) instead of re-flooding, so it stays O(path length).
  std::vector<int32_t> ProjectedBody(int32_t goal, const Paths& paths) const;
  // what collecting tile t is worth (0..1) and when it can be collected, arriving in `dist` rounds
  double TileValue(int32_t t, int32_t dist, int32_t* ready) const;
  int32_t RoomNeeded() const;
  Goal ChooseGoal(const Paths& paths) const;            // `paths` searched from the head
  bot::Action SafeStep(int preferred_dir, bool trap_ok, bool escape_ok = false, bool trap_grow = false); // the preferred step if safe, else the roomiest
  // Whether a dragon whose body is `body` (head first) can keep moving: a timed flood from
  // its head in which its own segments free up as its tail moves on. 2: it reaches a freed
  // segment of its own (it can follow its tail) or the unexplored edge; 1: at least `cap`
  // tiles; 0: doomed. *count: the tiles reached (up to cap).
  int EscapeLevel(const std::vector<int32_t>& body, int32_t cap, int32_t* count = nullptr,
                  const std::vector<int32_t>* blocked = nullptr) const;
  bool SplitSafe(int32_t n) const;
  int TurnOut(const std::vector<int32_t>& body, int32_t max_depth) const;
  bool HeardSplit(int32_t n) const;                     // our tail ping after splitting n off reaches the child
  int32_t GrowthSplit() const;                          // how many segments a growth split takes (-1: none safe)
  std::vector<int> StrikePath() const;
  int32_t Torus(int32_t a, int32_t b) const;
  // (cs13) this turn: tiles an enemy head can reach in one MOVE (empty when not guarding)
  std::vector<char> threat_;
  void MarkThreats();
  // (late-surge-23) lunge guard: for our dragons of length >= guard_length and the frozen
  // champion, the tiles an enemy head in view could reach with one multi-step MOVE (up to
  // min(its length - 1, lunge_reach) free steps)
  std::unordered_set<int32_t> lunge_threat_;
  void MarkLungeThreat();
  bool LungeExposed(int32_t t) const { return lunge_threat_.count(t) > 0; }
  bot::Action Decide(const rl::Observation& o);
#ifdef CS38_DEBUG
  void Dump(const char* why) const;
#endif
  int Liberties(int32_t to, const std::vector<int32_t>& body) const;
  int32_t FreeSlots() const { return spawn_.unit_limit - units_; }
  bool CanSplitNow() const;                             // a trapped split now leaves us alive (and fits the reserve)
  bool CanCountOnSplit() const;                         // planning: a dead end can be split out of later
  const char* Bucket(const char* base);                 // base + length bucket + near-cap tag (for tallies)
  char bucket_[64] = {};
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
  // portals (cs38)
  std::vector<uint8_t> portal_;                         // per tile: bit d set when its side d is a portal edge
  int32_t ping_round_ = -10, ping_tile_ = -1;
  int ping_dir_ = -1;
  bool exit_clear_ = false;                             // last turn's ping through that portal echoed no dragon
  // (cs38) known portals: the last goal step we refused only for being blind
  int32_t blind_tile_ = -1, blind_goal_ = -1, blind_round_ = -100;
  int blind_dir_ = -1, blind_streak_ = 0;
  bool exit_dirty_ = false;                             // last turn's ping from here echoed a dragon
  int32_t cur_goal_ = -1;                               // this turn's goal (for SafeStep's fallback)
  bool UnknownPortal(int32_t t, int d) const { return (portal_[t] >> d & 1) && board_.Step(t, d) == kUnknown; }
  bool BodyInView() const;
  bool WillCross() const { return p_.portal_cross && round_ < p_.cross_until; }
  Report obit_;                                         // a dead ally we relay (gst39)
  int32_t obit_until_ = -1;
  int32_t prev_head_ = -1;
  // loop breaker (cs38)
  std::deque<int32_t> recent_;                          // the last heads
  int32_t last_length_ = 0, last_eat_ = 0;
  int32_t last_goal_ = -1;
  std::unordered_map<int32_t, int32_t> banned_;         // goal tile -> banned until this round
  bool Looping() const;
  bool Banned(int32_t t) const {
    const auto it = banned_.find(t);
    return it != banned_.end() && it->second > round_;
  }
};

}  // namespace cs38
