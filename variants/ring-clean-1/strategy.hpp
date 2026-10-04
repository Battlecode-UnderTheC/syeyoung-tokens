// Arena line: ring-sched (team task scheduling). Comment only; behaviour identical to arena ring-plus-22.
// g39-sym (variant ring-sym-1, arena line ring-sched): the map's symmetry inferred from observation and mirrored into board memory (sym.hpp)
// g36-5 (ring-explore-1): hunger-scaled exploration for workers before kHungerUntil
// g36-7 (ring-explore-2): plus the team's unseen share of the board, workers only
#ifndef EXPLORE_UNSEEN
#define EXPLORE_UNSEEN 1
#endif
#ifndef EXPLORE_HUNGER
#define EXPLORE_HUNGER 1
#endif
inline constexpr int kHungerUntil = 150, kHungerRounds = 20;
// g38-sched (ring-sched-1): deterministic arrival-time assignment of nearby food among the
// workers in view (see ChooseGoal). 0: off (ring-bud-cpu's per-dragon greedy choice).
#ifndef SCHED_MODE
#define SCHED_MODE 2
#endif
inline constexpr int kSchedHorizon = 12, kSchedMaxParts = 12, kSchedHarvest = 8;
inline constexpr double kSchedOther = 0.1;
#ifndef EARLY15_MODE
#define EARLY15_MODE 6
#endif
// g35-1 / ring-clean-1: a queen that cannot split, in a team where no dragon in view can split,
// forages and splits at length 4 until the team has kForageSettleUnits dragons or kForageSettleRound.
#ifndef FORAGE35_MODE
#define FORAGE35_MODE 1
#endif
inline constexpr int kForageSettleUnits = 6, kForageSettleRound = 120, kForageRoom = 20;
// the rules: SPLIT n needs 2 <= n <= length - 2, so a dragon shorter than this cannot split
inline constexpr int kMinSplitLength = 4;
#ifndef SUPPLY14_MODE
#define SUPPLY14_MODE 1
#endif
#ifndef RING12_ATTACK_MODE
#define RING12_ATTACK_MODE 1
#endif
// g38-6: the paid-step lunge (mode 1) is used only while we have this many non-queen dragons;
// with fewer, the attack stays free-step-only (mode 2 behaviour).
inline constexpr int kLungeMinWorkers = 4;
#ifndef RING11_BRANCH_MODE
#define RING11_BRANCH_MODE 2
#endif
#ifndef RING11_RESCUE_MODE
#define RING11_RESCUE_MODE 0
#endif
#ifndef RING10_DANGER_MODE
#define RING10_DANGER_MODE 0
#endif
#ifndef RING10_SPLIT_MODE
#define RING10_SPLIT_MODE 1
#endif
#ifndef RING8_MODE
#define RING8_MODE 0
#endif
#include "queen_survival.hpp"
#ifndef RING5_MODE
#define RING5_MODE 3
#endif
#ifndef RING4_STAGE
#define RING4_STAGE 3
#endif
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
#ifndef RING6_MODE
#define RING6_MODE 3
#endif

#include <algorithm>
#include <cstdint>
#include <deque>
#include <unordered_map>
#include <vector>

#include "agent.hpp"
#include "board_memory.hpp"
#include "pathing.hpp"
#include "sym.hpp"

namespace gm2 {
using namespace strat;

#ifndef QUEEN_FEED_MODE
#define QUEEN_FEED_MODE 1
#endif
#ifndef QUEEN_SHORT
#define QUEEN_SHORT 1
#endif
#ifndef QUEEN_SAFETY_FROM
#define QUEEN_SAFETY_FROM 0
#endif
#ifndef QUEEN_ENDGAME
#define QUEEN_ENDGAME 1 // 1 safety default, 2 scheduled delivery, 3 experimental adaptive growth
#endif
inline constexpr int kPantryFrom = 360;
#ifndef QUEEN_CLEARANCE_MODE
#define QUEEN_CLEARANCE_MODE 2 // 0 off, 1 move only, 2 emergency exit, 3 proactively preserve two exits
#endif
#ifndef QUEEN_GUARD
#define QUEEN_GUARD 0
#endif

// mass-swarm-2 = gather-split-trade-131 (v8) + a swarm schedule: split continuously, never
// consolidate mid-game, feed one champion from kFinal. The cutoff depends on the team's size
// (see Remember): with fewer than kBigTeam dragons alive the swarm consolidates early (kFinal 320, champion
// frozen from 280: stronghold 10-0, autarky 8-2 vs v8); on big maps the swarm keeps eating
// until 400 (big_empty 2-8, schooltime 3-7 with the early schedule; 6-4, 5-5 with the late one).
inline constexpr int32_t kFinal = 320, kChampFreeze = 280;
inline constexpr int32_t kBigTeam = 48, kBigFinal = 400, kBigChampFreeze = 380;
// greedy eating: the nearest-pearl fallback (gst115's opening rule) stays on until this round
inline constexpr int32_t kGreedyUntil = 150;
// ms3: the schedule is decided once, at this round, and then kept (no flip-flopping as feeders die)
inline constexpr int32_t kLatchRound = 270;
// ms4: EscapeLevel counts an unseen edge as this many tiles of room (0: a sure escape, as before)
inline constexpr int32_t kUnknownWorth = 0;
// ms4: sonar reports of allies at most this many rounds old join the target-sharing discount (0: off)
inline constexpr int32_t kSonarShare = 0;
// ms5: cut a feeder of at least kChopMin into 3-long pieces before it dies (see Decide)
inline constexpr int32_t kChopFeed = 1, kChopMin = 6;
// ms5: dragons shorter than this never walk to the champion (they give 1-2 pearls and jam it)
inline constexpr int32_t kFeedMin = 2;
// ms6: feeding phase, non-champions leave pearls within kReserve (Manhattan) of the champion's
// head for it, using a champion report at most kReserveFresh rounds old (0: off)
inline constexpr int32_t kReserve = 4, kReserveFresh = 20;
// ms6 delivery: a feeder dies within kBodyReach (Manhattan) of any champion segment in view (0: head
// only, as before); and a worker starts its walk early enough to arrive: at the normal feed round,
// or at kWalkEnd - kWalkRate * (its distance to the champion's reported tile) if that is sooner
inline constexpr int32_t kBodyReach = 2, kWalkEnd = 480, kWalkRate = 3;
// ms6 parking: the champion keeps its goals within kPark (Manhattan) of where it stood at the feed round (0: off)
inline constexpr int32_t kPark = 0;
// ms6 delivery: a feeder that doesn't see the champion's head walks to its nearest visible segment, else to its reported tile (1: on)
inline constexpr int32_t kSeekBody = 1;
// ms8 (credit: arena-search-6/7): trades judged by value, not a length rule. Trade when
// their length - ours + remains * (2 * share - 1) > kTradeMargin, where remains = ceil(L/2) of
// both corpses and share = (our heads + 0.5) / (all heads + 1) within kShareRadius of the
// collision (the two traders excluded). 0: gst's length rule
inline constexpr int32_t kTradeValue = 1, kShareRadius = 5;
inline constexpr double kTradeMargin = 0.0;
// gm2: dragons up to this long treat a tile beside a SHORTER enemy head as exposed even when the
// trade value says the trade is fine (0: off, as gst-merge-1)
inline constexpr int32_t kCautiousMax = 0;
// gm2: the rollout judge wrap (plugin.cpp): on/off, and whether dead-end farming steps skip review
inline constexpr int32_t kJudge = 1, kJudgeSkipsDeadEnds = 1;
// gm2 (strategist x P4): after this round, idle short dragons no longer give up their slot at the
// cap (9999: never gated, as gst-merge-1)
inline constexpr int32_t kGiveUpUntil = 9999;
// gst-merge-1, from gst-free-1 (P3a): early split hold near enemy heads (see EarlyHold; kEarlyHoldRound 0: off)
inline constexpr int32_t kEarlyHoldRound = 0, kEarlyHoldLength = 8, kEarlyHoldRadius = 6;

struct Params {
  // growth: split off a worker (the child takes the tail half)
  int32_t split_min_length = 4;     // at least this long ... (numbers win the head-to-head attrition: 4 beat 6, 8, 12 in simulation)
  int32_t split_until_round = kFinal;  // ... and before this round
  int32_t leader_split_until = kFinal; // leaders (alive at the start) stop splitting at this round, to grow
  int32_t split_headroom = 0;       // ... leaving this many dragons below the unit limit (to split out of traps)
  int32_t big_split_length = 0;     // unused
  // feeding: a worker gives its body to a bigger ally
  int32_t feed_from_round = kFinal;   // late-surge endgame
  int32_t champ_freeze = kChampFreeze;      // the champion stops splitting from here (it only grows)
  // g15-15/gst62 (from body-wall-15): the frozen champion never takes a dead-end goal. A champion
  // that walks into a pocket trap-splits out of it: the child carries the mass under a new id and
  // the stub under the champion's id dies, so the team feeds a ghost.
  int32_t champ_no_trap = 1;
  // g16-6: replay pattern #30 "relay trap-split twins" - a long non-champion dragon still
  // dead-end-traps late, splitting into two giants that never merge (41+42 vs a single 72 that
  // consolidated, wg7 r438-492). Extend champ_no_trap's dead-end skip to any dragon long enough
  // that a trap-split would itself hand the team a second giant.
  int32_t no_trap_from_round = kFinal; // any dragon >= no_trap_min_length skips dead ends from here
  int32_t no_trap_min_length = 10;
  int32_t feed_distance = 3;        // dies when its head is this close to the ally's head (steps)
  int32_t ally_memory = 80;         // rounds an ally sighting stays worth walking to
  // consolidation: every dragon reports itself by sonar and relays the longest ally it knows
  // of; late in the game every other dragon walks to that one (the team's champion) and dies
  // next to it, so the mass ends up in one dragon
  int32_t report_memory = 40;       // rounds a sonar report of an ally stays usable
  double report_fade = 0.5;         // a report's weight when choosing the champion: its length less this per round of age
  // gather-split-trade-38 (from swarm-hunt-13): in the endgame a far champion's relayed reports
  // arrive old: keep them longer and fade them slower, so remote dragons stop electing themselves
#ifdef GM2_OFF
  int32_t late_report_from = 9999;
#else
  int32_t late_report_from = kFinal - 60;
#endif
  int32_t late_report_memory = 80;
  double late_report_fade = 0.25;
  // a partial sighting of an ally is only a lower bound on its length: it moves the report's
  // position and round on, but never shrinks the length an exact (self) report gave
#ifdef GM2_OFF
  int32_t lower_bound_reports = 0;
#else
  int32_t lower_bound_reports = 1;
#endif
  // gather-split-trade-39: pick the split point so that the ping we cast from our new tail
  // (opposite the facing) runs straight into the child's tail: the child starts knowing the champion
#ifdef GM2_OFF
  int32_t heard_split = 0;
#else
  int32_t heard_split = 1;
#endif
  // an ally at least this long (in view) seen beside our head last turn and gone now died: we
  // report it dead (length 0, this round) and relay that on alternate turns for obit_rounds, so
  // a dead champion's report stops drawing the team (late fade keeps it alive ~80 rounds)
#ifdef GM2_OFF
  int32_t obit_min_length = 9999;
#else
  int32_t obit_min_length = 5;
#endif
  int32_t obit_champion = 10;       // relay a death only of a dragon we knew this long
  int32_t obit_rounds = 20;
  int32_t split_announce = 1;       // (gst64) a mass-off split pings our stub length the same turn
  int32_t leader_feed_from = kFinal;   // a leader that is not the champion feeds it from this round ...
  int32_t leader_feed_fresh = 20;   // ... when the champion was located at most this many rounds ago
  // targets: each tile's value per round it takes to collect it (see TileValue)
  double pearl_decay = 0.97;        // per round since a pearl was seen: the chance it is still there
  double ripe_bed_value = 0.6;      // a bed whose countdown ran out out of view (grown, unless eaten)
  double explore_value = 0.25;      // the edge of the known map
  // ring-clean-1: body-wall-17's opening pull to the geometric centre on boards of at most
  // 1024 tiles is gone (a map-shape assumption). What it did on devil was break the tie
  // between equally near unexplored edges toward the food. The observed version: before
  // field_until, an unexplored-edge tile's explore score gains up to field_bonus times itself
  // by the share of beds among the known tiles within field_radius (food fields continue).
  int32_t field_until = 50;
  int32_t field_radius = 3;
  double field_bonus = 1.0;
  // late-surge-17 (crossover g13-15): waiting on a bed is charged more than walking, and a
  // pearl/bed in a field is worth more than a lone one
  double wait_factor = 2.0;         // a bed ready in `due` rounds at `dist` steps: dist + wait_factor*max(0, due-dist)
  double cluster_bonus = 0.5;       // a candidate gains this much x the value of nearby pearls/beds
  int32_t cluster_radius = 2;       // "nearby": Manhattan distance
  int32_t cluster_pool = 40;        // only the top-scoring this many tiles (and their neighbours) pay for the scan
  // g14-9: crowded midgame exploration (swh17): more than this many allies in view triggers the
  // stale-ground multiplier and folded-in patrol scoring
  int32_t crowd_threshold = 5;
  double stale_divisor = 50.0;      // age (rounds since seen) / this = the stale multiplier
  double stale_cap = 4.0;           // stale multiplier never exceeds this
  // dead ends: entering one costs a split to get out (the stub dies) and the time
  double trap_cost = 3.0;
  int32_t trap_candidates = 8;      // the best targets checked for being dead ends
  // g18-3: a dead end's cost is the expected loss, not a flat trap_cost: 0 if the pocket holds
  // our grown body plus a tile (we follow our tail out), else the stub a split leaves behind
  // (trap_stub segments); a 2-3 long dragon may enter when the pocket's pearls grow it to >= 4
  // and a unit slot is free (it can then split out)
  int32_t trap_loss = 0;            // gst131 (strategist-o Exp 3): off, flat trap_cost beat the loss model (see NOTES)
  double trap_stub = 2.0;
  // g22-1/gst115 (strategist-g P1, P3): the opening (r < opening_end) is a pearl-throughput
  // race, not a farming problem; ts8's simple greedy nearest-pearl search out-throughputs our
  // careful dead-end/crowd/contest scoring by ~2x in total length by r50. Pay dead-end costs
  // less heavily while it still matters to move fast, and fall back to the nearest reachable
  // pearl (ignoring all the scoring machinery above) rather than idling or patrolling when
  // nothing scored.
  int32_t opening_end = kGreedyUntil;
  double trap_opening_cost = 3.0;   // trap_cost while round_ < opening_end; gst131: unified with trap_cost (was 0.5)
  int32_t opening_pearl_radius = 40;  // nearest-pearl fallback only within this many tiles
  // round 2 (orchestrator feedback): the fallback above only fired when best.tile<0, which
  // never happens on Devil's open centre. Take the nearest pearl instead of the scored best
  // whenever the best is this much weaker than the pearl's own plain (undiscounted) score;
  // 1.0: prefer the pearl unless the scored best is at least as good on the same scale.
  double opening_pearl_factor = 1.0;
  // safety
  int32_t room_margin = 3;          // a step must leave at least length + this many tiles reachable ...
  int32_t room_min = 8;             // ... and never fewer than this (short dragons get boxed in too)
  int32_t escape_slack = 12;        // no way back out, but this many tiles more than twice our length: fine
  int32_t escape_min = 24;
  int32_t check_split = 1;
  int32_t liberties = 1;
  double share_factor = 0.1;        // a target an ally in view is nearer to is worth this much (1: ignore allies)
  // g16-27: replay pattern #33 - several dragons pile onto the same bare frontier tile in the
  // opening instead of fanning out. share_factor's discount (above) doesn't stop this for pure
  // explore tiles (their score is a max(), not a product, with the shared-tile discount) and
  // gst82's attempt to fold the discount into the explore score just left the discounted dragon
  // with nothing else scoring. Instead: before this round, a frontier tile within fan_out_radius
  // of an ally seen this turn who is nearer to it than we are gets no explore credit at all, so
  // the scan naturally lands on a different (further) frontier tile instead of a discounted copy
  // of the same one.
  int32_t fan_out_until = 80;
  int32_t fan_out_radius = 3;
  // g17-11: replay pattern #35 - short stubs pile onto the same bed/pearl inside one small
  // bed-filled room and trap each other (league-6, gst84's biggest non-cap length sink at 276
  // length/game, 520/520 trapped deaths walled in by our own or ally bodies vs trading-swarm-5).
  // fan_out's ally list only guards the opening (round_ < fan_out_until); this discount reuses
  // the same idea (allies seen this turn) but runs for the whole game, and discounts bed/pearl
  // goals (not just bare explore tiles) by how many ally heads are already crowding them.
  int32_t crowd_discount_radius = 3;    // an ally head within this many (torus) tiles counts
  double crowd_discount_factor = 0.55;  // score *= this ^ (ally heads within radius)
  // g17-11: skip a growth split when this many ally heads are already within crowd_discount_radius
  // of our own head, so children aren't born straight into the crowd that traps them.
  int32_t split_crowd_limit = 99;  // mass-swarm: split even in a crowd
  double contest_factor = 0.3;      // a target an enemy head in view gets to first (turn order breaks bed ties) is worth this much
  // g15-6: an enemy head in view contests only its this many nearest pearls/beds (ties included);
  // 0: every tile it reaches first (gst51)
#ifdef GM2_OFF
  int32_t enemy_claims = 0;
#else
  int32_t enemy_claims = 2;
#endif
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
  // g16-6: from merge_round, a long non-champion (>= merge_min_length) drops the tenths hold and
  // feeds the champion once it's longer by any margin at all, merging the team's two giants
  // instead of leaving them to finish the game separately.
  int32_t merge_round = kFinal + 30;
  int32_t merge_min_length = 10;
  int32_t cap_safety = 1;           // 1: with fewer than cap_margin free slots, a dragon plans as if it can't split out
  int32_t cap_margin = 2;           // free slots needed to count on a split escape (other dragons split the same round)
  int32_t reserve_slots = 0;        // growth splits leave this many slots free for trapped long dragons ...
  int32_t reserve_length = 6;       // ... this long (shorter ones split out only above the reserve)
  // portals (gst115): step through an unknown portal next to the head when only exploring,
  // after a sonar ping through it echoed clear the turn before
  int32_t portal_cross = 1;
  int32_t cross_until = 480;        // ... before this round
  int32_t known_portal = 1;         // (gst115) a known portal's blind step: ping it first; clean echo or exit seen empty lifts the cap
  int32_t blind_fallback = 2;       // (gst115) accept a blind goal step after this many detours for the same goal (0: never)
  int32_t portal_explore = 1;       // 1: an unknown portal gives explore value only when we would cross it
  // (gst115, ported from gst83/gst98 via g16-26 g16-20) a poor home region: few known reachable
  // pearls/beds per ally seen nearby, or nothing eaten for a while, and more seen-but-unreachable
  // valued tiles than reachable ones (a locked room, not just a spread-out team). Then an unknown
  // portal's explore value is multiplied (so it beats far goals in an empty strip) and a dragon
  // beside one whose exit pinged clean crosses even with a valued goal elsewhere.
  int32_t poor_per_unit = 1;        // poor: fewer reachable valued tiles than this x allies seen nearby ...
  int32_t poor_min = 3;             // ... or than this
  int32_t starve_rounds = 35;       // ... or nothing eaten for this many rounds
  double poor_portal_mult = 4.0;    // unknown-portal explore value multiplier when poor
  int32_t poor_cross = 6;           // >0: when poor, cross a clean-pinged unknown portal even with a valued goal this many steps away or more
  // loop breaker: the last loop_window heads on at most loop_distinct tiles, nothing eaten:
  // the goal is banned for loop_ban rounds
  int32_t loop_break = 1;
  int32_t loop_window = 12;
  int32_t loop_distinct = 6;
  int32_t loop_ban = 50;
  // gst-merge-1, from gst-hunter-1 (hunter team; strategist-w P1/P2, the server opponent's corridor
  // economy): a bed seen to reset to a countdown <= hot_gap is hot. A hot bed is worth a full
  // pearl whenever it is due, a dead-end pocket holding hot beds gains hot_pocket_bonus each, and
  // a 2-3 long dragon may enter such a pocket when its food grows it to 4 (it splits out at the
  // tip). A farmer (length <= hot_claim_max_len) heading for a hot bed within hot_claim_dist
  // claims it over sonar (0xA8); allies skip hot beds within hot_claim_radius of a claim for
  // hot_claim_memory rounds.
  int32_t hot_farm = 1;
  int32_t hot_gap = 3;
  double hot_pocket_bonus = 1.0;
  int32_t hot_claim = 1;
  int32_t hot_claim_max_len = 5;
  int32_t hot_claim_dist = 8;
  int32_t hot_claim_radius = 2;
  int32_t hot_claim_memory = 30;
};

class StrategyAgent : public bot::Agent {
 public:
  StrategyAgent(const rl::Spawn& spawn, Params params = {});
  bot::Action Turn(const rl::Observation& o) override;
  // why the last action was taken (for the simulator's tallies and debugging)
  const char* reason() const override { return reason_; }
  // (gst-search-4) this turn's move walks to an ally (feeding the champion)
  bool WalkingToAlly() const { return walking_to_ally_; }
  int32_t FeedFrom() const { return p_.feed_from_round; }   // (gst-search-8) this game's consolidation round
  bool TryQueenAttack(const rl::Observation& o, bot::Action* out) const;
  bool IsQueen() const { return spawn_.id <= 1; }
  bool RescueCommitted() const { return rescue_committed_; }
  bot::Action ProtectQueenAction(bot::Action a, const bot::Action& fallback);

  bool walking_to_ally_ = false;
  // gm2: whether a step from our head in direction d lands on a free tile (for the rollout judge's
  // overrides, which can't see our full body memory: gst-giant-1 failed check on self deaths)
  bool FreeStep(int d) const { return Classify(board_.Step(head_, d)) == Cell::kFree; }
  // (gst-search-4) a multi-step move our memory says is clear: free tiles, an enemy head only at the end
  bool PathFree(const std::vector<int>& steps) const {
    int32_t t = head_;
    for (size_t i = 0; i < steps.size(); ++i) {
      t = board_.Step(t, steps[i]);
      if (t < 0) return false;
      const Cell c = Classify(t);
      if (c == Cell::kFree) continue;
      if (c == Cell::kEnemyHead && i + 1 == steps.size()) continue;
      return false;
    }
    return true;
  }

 private:
  struct Sighting {
    int32_t head = -1;              // tile of its head
    int32_t round = -1;
    int32_t length = 0;             // segments seen (a lower bound)
    uint8_t team = 0;
    bool partial = true;
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
    bool lower = false;   // the length is a lower bound (an ally seen only in part)
  };
  uint64_t Encode(const Report& r) const;
  void Decode(uint64_t word);
  void Note(const Report& r);
  Report Champion() const;                              // the longest ally known (maybe us)
  bot::Action QueenMove();
  bool Corridor(int32_t t) const;
  int OpenRoom(int32_t t, const std::vector<int32_t>& body, int cap) const;
  bot::Action RingQueen();
  bot::Action RingWorker(bot::Action normal, const rl::Observation& o);
  std::vector<int32_t> ring_;
  int ring_seen_ = -1000;
  uint64_t RingWord() const;
  bool ring_worker_override_ = false;
  int ring_child_release_ = -1;
  bot::Action RingChildEscape();
  int courier_ = -1, dock_ = -1, courier_since_ = -1;
  int delivery_seen_ = -1000, delivery_queen_head_ = -1;
  bool queen_growing_ = false, delivery_growing_ = false, queen_forage_ = false;
  uint64_t DeliveryWord() const;
  void ReadDelivery(const rl::Observation& o);
  void SelectCourier();
  bot::Action CourierMove();
  mutable int queen_risk_round_ = -1;
  mutable std::vector<int> queen_risk_cache_;
  // (ring-cpu-1) reusable scratch: epoch-stamped board-sized arrays, never cleared per call
  struct Scratch {
    std::vector<uint32_t> stamp;
    std::vector<int32_t> value;
    uint32_t epoch = 0;
    // a fresh mark set over n tiles; Has(t) is false for every t until Set(t)
    void Begin(int32_t n) {
      if (static_cast<int32_t>(stamp.size()) != n) { stamp.assign(n, 0); value.assign(n, 0); epoch = 0; }
      if (++epoch == 0) { std::fill(stamp.begin(), stamp.end(), 0); epoch = 1; }
    }
    bool Has(int32_t t) const { return stamp[t] == epoch; }
    void Set(int32_t t, int32_t v = 0) { stamp[t] = epoch; value[t] = v; }
    int32_t Get(int32_t t) const { return value[t]; }
  };
  mutable Scratch esc_index_, esc_when_, q_body_, q_dist_, q_mine_, cont_index_, sched_seen_;
  mutable std::vector<int32_t> esc_queue_, q_queue_, sched_queue_;
  // QueenContinuation's states: the base body plus up to four new head tiles (ring.cpp)
  struct ContState { int added[4]; int k = 0; int size = 0; int ate[4]; int ne = 0; int first = -1; };
  mutable std::vector<ContState> cont_beam_, cont_next_;
  // ~540 judge points a unit (sandbox, big_empty); the turn's fixed cost is ~10M, so the cap
  // holds the queen's beam near 65M. Normal games peak near 50k (r440, a length-74 queen).
  static constexpr int64_t kQueenWorkCap = 100000;
  mutable int64_t work_ = 0;   // (ring-cpu-1) this turn's work: tiles expanded by the searches below
  int QueenRisk(int tile) const;
  std::vector<int> ring4_observed_body_;
  int ring5_last_patrol_ = -1000;
  int ring5_recover_until_ = -1;
  int ring5_next_ = -1;
  void ReadRing5(const rl::Observation& o);
  bool Ring5Reserved(int tile) const;
  bool GrowthReady() const;
  int QueenContinuation(const std::vector<int32_t>& body, const std::vector<int>& eaten, int* branches) const;
  queen_survival::Result QueenFuture(const std::vector<int>& body, const std::vector<int>& eaten, int& budget, int& expanded) const;
  std::string queen_guard_reason_;
  bot::Action Ring4Exit();
  bot::Action Ring4Supply();
  bot::Action Supply14Yield(bot::Action action);
  bot::Action RescueQueen(const bot::Action& normal, const rl::Observation& observation);
  bool FeedAhead(const Report& queen) const;
  struct Goal {
    int32_t tile = -1;
    bool trap = false;              // in a dead end worth entering (its gain beats trap_cost)
    bool grow = false;              // (g18-3) too short to split now, but the pocket's pearls fix that
  };
  // what collecting tile t is worth (0..1) and when it can be collected, arriving in `dist` rounds
  double TileValue(int32_t t, int32_t dist, int32_t* ready) const;
  double TileValue(int32_t t, int32_t dist, int32_t* ready, bool occupied) const;  // occupied precomputed (ChooseGoal: Mine() is O(length))
  int32_t RoomNeeded() const;
  Goal ChooseGoal(const Paths& paths) const;            // `paths` searched from the head
  bot::Action SafeStep(int preferred_dir, bool trap_ok, bool trap_grow = false); // the preferred step if safe, else the roomiest
  // Whether a dragon whose body is `body` (head first) can keep moving: a timed flood from
  // its head in which its own segments free up as its tail moves on. 2: it reaches a freed
  // segment of its own (it can follow its tail) or the unexplored edge; 1: at least `cap`
  // tiles; 0: doomed. *count: the tiles reached (up to cap).
  int EscapeLevel(const std::vector<int32_t>& body, int32_t cap, int32_t* count = nullptr,
                  const std::vector<int32_t>* blocked = nullptr) const;
  int32_t FeedStart(const Report& champ) const;          // (ms6) the round this worker starts walking to the champion
  bool SplitSafe(int32_t n) const;
  bool HeardSplit(int32_t n) const;                     // our tail ping after splitting n off reaches the child
  int32_t GrowthSplit() const;                          // how many segments a growth split takes (-1: none safe)
  int TurnOut(const std::vector<int32_t>& body, int32_t max_depth) const;
  bool Lunge(bot::Action* out) const;
  bot::Action Decide(const rl::Observation& o);
#ifdef GM2_DEBUG
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
  symm::SymInfer sym_;   // g39-sym: the map's symmetry, inferred (sym.hpp)
  bool leader_ = false;
  bool first_turn_ = true;
  int32_t born_ = 0;
  // this turn
  int32_t head_ = 0, length_ = 0, units_ = 1, round_ = 0;
  int32_t anchor_ = -1;
  int32_t queen_id_ = -1;
  int32_t queen_home_ = -1;
  bool rescue_committed_ = false;
  // gst-merge-1 (from gst-hunter-1): hot beds and farmer claims
  std::vector<int32_t> cd_last_, cd_round_;
  std::vector<uint8_t> hot_;
  std::unordered_map<int32_t, std::pair<int32_t, int32_t>> claims_;   // bed tile -> (claimer id, round)
  int32_t claim_tile_ = -1;
  void UpdateHot(const rl::Observation& o);
  bool EarlyHold() const;
  bool ClaimedByOther(int32_t t) const;     // (ms6) the champion's parking tile in the feeding phase (-1: not parked)
  int32_t big_team_ = -1;   // ms3: the schedule, latched at kLatchRound (-1: not yet)
  bool cap_latched_ = false; // g39-5: the team reached the cap from kNoBurnFrom (parallel growth)
  std::unordered_map<int32_t, Sighting> seen_;          // other dragons by id
  // this dragon's body, head first: always the last `length` tiles its head stood on
  // (eating keeps the tail, a split cuts it), so it is known even out of view, e.g.
  // on the far side of a portal it just came through
  std::deque<int32_t> body_;
  std::unordered_map<int32_t, Report> reports_;         // allies (and us) by id, newest report
  Report obit_;                                         // a dead ally we relay (gst115)
  int32_t obit_until_ = -1;
  int32_t prev_head_ = -1;
  const char* reason_ = "";
  // portals (gst115)
  std::vector<uint8_t> portal_;                         // per tile: bit d set when its side d is a portal edge
  int32_t ping_round_ = -10, ping_tile_ = -1;
  int ping_dir_ = -1;
  bool exit_clear_ = false;                             // last turn's ping through that portal echoed no dragon
  // (gst115) known portals: the last goal step we refused only for being blind
  int32_t blind_tile_ = -1, blind_goal_ = -1, blind_round_ = -100;
  int blind_dir_ = -1, blind_streak_ = 0;
  bool exit_dirty_ = false;                             // last turn's ping from here echoed a dragon
  int32_t cur_goal_ = -1;                               // this turn's goal (for SafeStep's fallback)
  mutable bool poor_ = false;                           // (gst115) set by ChooseGoal: the home region is poor
  bool UnknownPortal(int32_t t, int d) const { return (portal_[t] >> d & 1) && board_.Step(t, d) == kUnknown; }
  bool BodyInView() const;
  bool WillCross() const { return p_.portal_cross && round_ < p_.cross_until; }
  // loop breaker (gst115)
  std::deque<int32_t> recent_;                          // the last heads
  int32_t last_length_ = 0, last_eat_ = 0;
  int32_t last_goal_ = -1;
  std::unordered_map<int32_t, int32_t> banned_;         // goal tile -> banned until this round
  bool Looping() const;
#ifdef GM2_DEBUG
  int32_t exp_tile_ = -1, exp_round_ = -1;
  bool exp_pearl_ = false;
  std::vector<int32_t> exp_ids_;
#endif
  bool Banned(int32_t t) const {
    const auto it = banned_.find(t);
    return it != banned_.end() && it->second > round_;
  }
};

}  // namespace gm2
