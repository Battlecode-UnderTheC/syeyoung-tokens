// ring-founder-1 (g43-6): ring-contest-m1 + founder dispatch on 1-2-starter maps (founder.cpp, see NOTES.md)
// ring-contest-m1 (g42 merge, arena line ring-contest): ring-sched-m3 + ring-contest-2 + ring-qgate-1 + ring-champdie-1 + ring-clustercall-2 (see NOTES.md)
// ring-frontier-1 (g40-5): ring-sched-m2 with the centre rush replaced by frontier exploration (rush.cpp).
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
// g40-4 (ring-qrace-1): from kRaceFrom (the round our queen leaves her ring and starts to grow:
// ring4 exit at 430) while the enemy queen was seen alive within kRaceFresh rounds, workers walk to
// our queen and resign within kRaceReach of her head, so the queen-length tiebreak gets their mass.
#ifndef QRACE_MODE
#define QRACE_MODE 1
#endif
inline constexpr int kRaceFrom = 430, kRaceFresh = 30, kRaceReach = 2;
inline constexpr int kQGateFrom = 300;   // g42-4: see EnemyQueenRace()
// g45-6 (ring-sprint-racetime): what the conveyor adds to our queen per round once it runs, as
// measured on the conveyor itself (ring-sprint-6's pre-promotion check, 150 sides with our queen
// alive at r430: +24.1 from r430 to the end, 70 rounds, 0.34 a round; 191697 4 -> 15 by r471,
// 0.27). From kQGateFrom (the round a sighting is the observed sign of a race), while a race is
// evidenced, the conveyor starts as soon as the gap to the longest enemy known exceeds what it
// can add in the rounds left: gap > (500 - round) * kConveyorGrowNum / kConveyorGrowDen.
#ifndef RACETIME_MODE
#define RACETIME_MODE 1
#endif
inline constexpr int kConveyorGrowNum = 1, kConveyorGrowDen = 3;
// g44-7 (ring-sprint-conveyor): the feeding conveyor (conveyor.cpp) replaces QueenRace and the
// r450 feed14 wave. kLineReach: her line is busy while a pearl lies within this many free steps
// of her head. In the race she is 2-12 long (1-3 free steps a turn), so she takes such a pearl in
// one or two turns; a corpse's pearls lie 2 apart, so the far end of a line (4+ steps out) no
// longer holds the next feeder back, and lines follow each other without a gap.
#ifndef CONVEYOR_MODE
#define CONVEYOR_MODE 1
#endif
// kQueueDist: a waiting feeder keeps out of her zone plus the kRaceReach tiles it would cross to
// lay itself there: kLineReach + kRaceReach = 5.
inline constexpr int kLineReach = 3, kQueueDist = kLineReach + kRaceReach;
inline constexpr int kHungerUntil = 150, kHungerRounds = 20;
// g38-sched (ring-sched-1): deterministic arrival-time assignment of nearby food among the
// workers in view (see ChooseGoal). 0: off (ring-bud-cpu's per-dragon greedy choice).
#ifndef SCHED_MODE
#define SCHED_MODE 2
#endif
inline constexpr int kSchedHorizon = 12, kSchedMaxParts = 12, kSchedHarvest = 8;
inline constexpr double kSchedOther = 0.1;
// g42-6 (ring-contest-1): the opening food contest (ChooseGoal, Decide steps 4-5). 0: off.
#ifndef SPRINT_MODE
#define SPRINT_MODE 2   // g43-5: free sprint steps for non-queen dragons of length 5+ (2: none with an enemy head in view)
#endif
#ifndef CONTEST_MODE
#define CONTEST_MODE 1
#endif
inline constexpr int kContestUntil = 60;   // the opening, as rush.cpp's kMiddleUntil (r30-59 is where we lost the food)
// an enemy head seen up to kRivalMemory rounds ago still counts: arena-r16's A losses show the
// enemy taking a food area by ~r16 and staying on it (median turning point r20), so where a head
// was seen is where it farms. It may have come up to kRivalDrift steps nearer since (a farmer moves
// around its patch; beyond that the sighting says nothing about the direction it went).
inline constexpr int kRivalMemory = 20, kRivalDrift = 3;
inline int RivalDist(int dist, int age) { const int d = dist - (age < kRivalDrift ? age : kRivalDrift); return d > 0 ? d : 0; }
// g42-12 (ring-contest-2): a dragon this long (typically a starter; split_min_length 4 makes 2+2)
// is exempt from the no-split-while-racing gate, so it splits as the base does.
inline constexpr int kRaceSplitExempt = 5;
// g42-12: an explorer takes uncontested food only within this many steps: its own view radius
// (kView 3), a detour of at most 3 turns from the frontier walk.
inline constexpr int kExplorerNear = 3;
inline constexpr double kContestDeny = 2.0;    // food won from an approaching enemy: ours + one they don't eat
// g43-6 (ring-founder-1): founder dispatch on small starts (founder.cpp). Observed cause (arena-r17,
// Colosseum 5-25, Trophy 14-46): with 1-2 starters the queen sat at length 4 without splitting and
// the first child idled at home with no food in view, so we had 2 dragons until r38-44 v 16.
//  - A child whose first turn finds the team at most kFounderUnits strong (itself included) before
//    kFounderUntil is a FOUNDER: three or fewer right after a birth means we started with two or
//    fewer (3+ starters make 4+ with the first child). It walks to the largest bed region it has
//    seen that it reaches no later than any seen enemy head (explores the frontier while there is
//    none), makes no growth split on the way, and splits from length 4 once there.
//  - On such a start the queen, in open ground, splits whenever she reaches 4 before
//    kFounderUntil, and keeps her home pull (no forage roaming).
#ifndef FOUNDER_MODE
#define FOUNDER_MODE 1
#endif
#ifndef FOUNDER_HOME
#define FOUNDER_HOME 0
#endif
inline constexpr int kFounderUnits = 3;
inline constexpr int kFounderUntil = kContestUntil;   // the opening (r60), as the contest
// pearls within 3 are one region: a head sees 3 tiles each way (the 7x7 view), so such pearls are
// farmed from one spot. (Iteration 4: with 2, ClusterScore's radius, the sparse pearls of a fast
// bed field (Colosseum's middle, one pearl every 3-4 tiles) were all singletons, no region at all.)
inline constexpr int kRegionLink = 3;
inline constexpr int kArrive = 2;       // in a region: within 2 steps of one of its beds (its cluster radius)
// the queen doesn't box a worker in (QueenMove, BoxedBy): a worker within kBoxReach of where she'd
// stop that then reaches fewer than kBoxRoom tiles costs her two pearls' worth (24). kBoxRoom 30:
// more than a home pocket holds (Colosseum's is 6x3 = 18) and less than the open board around it;
// kBoxReach 6: a worker she could cut off is near (her home pull keeps her within 6 of home).
inline constexpr int kBoxRoom = 30, kBoxReach = 6;
inline constexpr double kBoxCost = 24.0;
// g40-6 (ring-bedlearn-1): exact bed timing from the countdowns we observed. A bed read with
// countdown c in round s grows its pearl at the tick of round s + c, and the engine redraws its
// gap at every tick, eaten or blocked alike (docs/game.md; engine Game::BeginRound). Our head takes
// its k-th step in round now + k - 1, so with `dist` steps it gets the new pearl only if
// dist >= due + 1 (due = s + c - now). Arriving earlier puts us ON the bed at the tick, which blocks
// the pearl and wastes the whole wait. 0: the parent's max(dist, due) and the fixed ripe value.
#ifndef BEDTIME_MODE
#define BEDTIME_MODE 1
#endif
#include <cstdint>
// steps-equivalent cost of collecting a bed `due` rounds from ripe (waiting is charged wait_factor)
inline int32_t BedReady(int32_t dist, int32_t due, double wait_factor) {
  if (due <= 0) return dist;
  const int32_t need = BEDTIME_MODE ? due + 1 : due;   // the first step count that arrives at or after the tick
  return wait_factor > 0 ? dist + static_cast<int32_t>(wait_factor * (need > dist ? need - dist : 0))
                         : (need > dist ? need : dist);
}
#ifndef EARLY15_MODE
#define EARLY15_MODE 6
#endif
// g35-1: a short queen (<=3) on a small start (<=2 dragons) forages and splits
// at length 4 until the team has kForageSettleUnits dragons or kForageSettleRound.
#ifndef FORAGE35_MODE
#define FORAGE35_MODE 1
#endif
inline constexpr int kForageSettleUnits = 6, kForageSettleRound = 120, kForageRoom = 20;
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
// g42-3 (ring-champfood-1): in late concentration (see kLateConcFrom in strategy.cpp) the champion
// grows from pearls, not resigns. Observed cause (server-r15 C2, g41-1): in longest-tiebreak losses
// our longest is 17-24 v 26-76 while our total is often larger, and a resign drops a pearl only on
// every second segment, so feeding by resignation throws half of each feeder away.
//  - kLastFeedFrom: feeders resign beside the champion only from about here; a far feeder sets out
//    earlier by its distance, so it arrives by then. A feeder of length L leaves ceil(L/2) pearls
//    along L tiles, and the champion sweeps at most kChampSight tiles a turn, so it needs about L/3
//    turns per feeder: two or three 10-long feeders take ~10-20 turns, hence r480. Earlier, a
//    feeder's mass is worth more as a forager that leaves the champion's food alone. (Iterations
//    1-2 used r488 with no early walk: the champion gained less from r430 to r500 on server maps
//    than with the parent's resigns, +5..7 v +4..11, while the total rose.)
//  - kChampSight: the most tiles a turn the champion's harvest sprint covers. Its sprint only steps
//    on tiles it sees now (the 7x7 view, radius 3), so its speed is min(ceil(L/4), 3); workers use
//    the same speed to decide which food the champion reaches first.
//  - kChampWorkCap: the harvest beam's work cap, half the queen's (kQueenWorkCap), so the worst
//    turn stays well under the judge's CPU cap.
inline constexpr int32_t kLastFeedFrom = 480, kChampSight = 3, kChampMaxSteps = 6;
inline constexpr int64_t kChampWorkCap = 50000;
// g42-8 (ring-champdie-1): late deaths fall next to ONE champion. Observed cause (server-r16 C2):
// in r400-499 the opponent's champion eats 13.8 pearls from its own dead, ours 5.8; their late
// deaths happen beside the champion, ours wherever the dragon is. A local log of ring-champfood-1
// (44 fresh maps, queen-dead games) agrees: 2-3 long stubs dying "trapped: no move" are most late
// deaths, only ~20% of late resigns fall within 3 tiles of the champion, and a game averages 11-18
// different champion names, because the pick stuck until death and a newborn named itself.
//  - kDieNearFrom: from here (with our queen not known alive, the lateconc gate) the team agrees on
//    the champion and doomed stubs walk to it. r400: the window the evidence measures, and after
//    every growth-split cutoff (<= r400), so a stub can no longer be grown into a worker.
//  - kDoomedLen: a stub this short is doomed: in that log 61% of the 2-3 long non-champions alive
//    at r440 resign before r500, nearly all "trapped: no move" where they stand; a 3-long stub
//    leaves 2 pearls, the most a stub gives. Only a stub that has eaten nothing for
//    p_.starve_rounds counts (iteration 2): in iteration 1 every stub fed, and on the arena's
//    custom maps, where late stubs still forage and grow, the team fell from ~50 dragons at r400
//    to ~15-20 by r460 and the champion still finished shorter (longest-decided 1-7).
inline constexpr int32_t kDieNearFrom = 400, kDoomedLen = 3;
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
  // body-wall-17 (ported g15-20): on small maps the swarm takes the central field first and
  // out-eats us in the opening; before centre_until, on maps of at most centre_max_tiles
  // tiles, a candidate's score (including the unexplored-edge score) gains up to centre_bonus
  // times itself the nearer it is to the map centre (torus distance; 0 at the farthest tile)
  int32_t centre_max_tiles = 1024;
  int32_t centre_until = 50;
  double centre_bonus = 0.3;
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
  int32_t Length() const { return length_; }   // g43-5 (debug tallies)
  int32_t FeedFrom() const { return p_.feed_from_round; }   // (gst-search-8) this game's consolidation round
  bool TryQueenAttack(const rl::Observation& o, bot::Action* out) const;
  bool IsQueen() const { return spawn_.id <= 1; }
  int Founder() const { return founder_; }   // g43-6 (debug)
#ifdef CONVEYOR_DBG
  int32_t DbgQueenDist() const {
    int32_t t = -1;
    if (const auto s = seen_.find(queen_id_); s != seen_.end() && s->second.round == round_) t = s->second.head;
    else if (const auto r = reports_.find(queen_id_); r != reports_.end()) t = r->second.tile;
    return t < 0 || head_ < 0 ? -1 : Torus(t, head_);
  }
#endif
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
  void UpdateLateChampion();                            // g40-3: late concentration (see strategy.cpp)
  bot::Action QueenMove();
  bool Corridor(int32_t t) const;
  int OpenRoom(int32_t t, const std::vector<int32_t>& body, int cap) const;
  bool BoxedBy(int32_t from, const std::vector<int32_t>& body) const;   // g43-6
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
  // g40-4 (ring-qrace-1) queen race: the last round the enemy queen was seen alive (in view or
  // relayed in bits 4-8 of the champion word), and the late walk of workers to our queen
  int32_t enemy_queen_seen_ = -1000;
  // g42-4 (ring-qgate-1): the last round we saw the enemy queen vanish beside us (death), and the
  // gate on late queen growth. arena-r16: the enemy queen was seen in r300-429 in 65 of 66 queen
  // races we won but in only 6 of 271 losses where our grown queen died after theirs (median death
  // r205), so a sighting from kQGateFrom on is the observed sign that a queen race exists.
  // The ring's sonar words (sent by the queen and the workers that know her ring, which drown out
  // champion words near her: she heard 1-15 champion words in 430 rounds in sim) carry the last
  // sighting round in bits 26-35, with no freshness limit, so the queen learns what workers saw.
  int32_t enemy_queen_dead_ = -1000, enemy_queen_heard_ = -1000;
  int32_t EnemyQueenKnown() const { return std::max(enemy_queen_seen_, enemy_queen_heard_); }
  // try 3: a queen never seen at all may be parked unseen (duels v ring-sched-17: maze and
  // around_unsw 0-4, the enemy queen alive but never in view), so only a stale or dead sighting blocks
  bool EnemyQueenRace() const {
    const int32_t k = EnemyQueenKnown();
    return k < 0 || (k >= kQGateFrom && k > enemy_queen_dead_);
  }
  bot::Action QueenRace(const Report& champ);
  bot::Action Conveyor();   // g44-7 (conveyor.cpp)
  // g45-6 (ring-sprint-racetime): race time (conveyor.cpp). The longest enemy queen and the
  // longest enemy of any id this dragon has seen (segments in view, a lower bound) or heard in a
  // ring word (bits 36-47 when it carries no call), and the round the queen race started early
  // (-1: not yet; it then starts at kRaceFrom as before). RaceTime(): the queen-race phase, the
  // gate that kRaceFrom alone was for the conveyor, her ring exit and her growth vetoes.
  int32_t enemy_len_queen_ = 0, enemy_len_any_ = 0, race_early_ = -1;
  bool RaceTime() const { return round_ >= kRaceFrom || race_early_ >= 0; }
  void UpdateRaceTime(const rl::Observation& o);
  uint64_t RaceLengths(uint64_t ring_word) const;
  uint32_t EnemyQueenField(int32_t at) const {
    if (enemy_queen_seen_ < 0 || at - enemy_queen_seen_ > kRaceFresh) return 0;
    return uint32_t(1 + std::max(0, at - enemy_queen_seen_));
  }
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
  int32_t late_champ_ = -1;    // g40-3: the agreed late champion (sticky until it dies), -1: none
  bool late_conc_ = false;     // g40-3: concentrating into late_champ_ this turn
  bool die_near_ = false;     // g42-8: the team agrees on late_champ_ and doomed stubs die beside it (from kDieNearFrom)
  bool rescue_committed_ = false;
  // gst-merge-1 (from gst-hunter-1): hot beds and farmer claims
  std::vector<int32_t> cd_last_, cd_round_;
  std::vector<uint8_t> hot_;
  std::unordered_map<int32_t, std::pair<int32_t, int32_t>> claims_;   // bed tile -> (claimer id, round)
  int32_t claim_tile_ = -1;
  int32_t avoid_tile_ = -1;   // g40-6: SafeStep never steps here (a bed before its tick)
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
  // g40-5 (ring-frontier-1): frontier exploration (rush.cpp). The two lowest-id non-queen starters
  // walk to the reachable frontier with the most never-seen ground in view behind it, eating food
  // on the way and splitting once, until a worthwhile or contested bed cluster is seen (then they
  // hold it to r60) or r30. Any worker beside a dense bed cluster before r60 contests at parity.
  int8_t rush_state_ = 0;            // 0 undecided, 1 exploring, 2 holding a cluster, 3 not a rusher / done
  int32_t rush_target_ = -1;         // the frontier tile (state 1) or the cluster held (state 2)
  int32_t rush_seen_ = 0, rush_since_ = 0;   // tiles ever seen, and the round that count last grew
  bool rush_split_ = false;          // the one growth split allowed while exploring was used
  bool contest_ = false;
  // g43-6 (ring-founder-1): 0 undecided, 1 founder (until its first split in a region), 2 done, 3 not one (founder.cpp)
  int8_t founder_ = 0;
  bool small_start_ = false;
   // the queen: we started with <= 2 dragons (her r0 unit count)
  int32_t FounderGoal(const Paths& paths, bool* here);   // *here: in the chosen region
  // g42-1 (ring-clustercall-1): the team-wide early cluster call (call.cpp). A dragon that sees a
  // bed cluster with more live or soon-due beds than our heads around it calls the team over
  // sonar (magic 0xA9); allies relay the call, and a worker whose own best goal is worth less
  // than the call's surplus per round of travel walks there.
  struct Call {
    int32_t value = 0, round = -1000;   // surplus beds (beds less our heads near), round found
  };
  std::unordered_map<int32_t, Call> calls_;   // cluster tile -> the freshest call heard about it
  int32_t own_call_ = -1;                     // this turn: a cluster we found and call ourselves
#ifdef CLAIMMID_DBG
  mutable std::vector<int32_t> dbg_detour_;
#endif
  mutable bool call_goal_ = false;            // this turn's goal is the way to a call
  void UpdateCall(const rl::Observation& o);
  uint64_t CallWord(int32_t tile, const Call& c) const;
  uint64_t CallInRing(uint64_t ring_word, int32_t tile, const Call& c) const;
  void HearCall(int32_t x, int32_t y, int32_t v, int32_t r, int32_t mask);
  bool CallToSend(int32_t* tile, Call* c) const;
  int32_t CallGoal(const Paths& paths, double beat) const;
  bool CallMine(int32_t c, const Call& k) const;          // g42-9: we rank within the call's answerer cap
  int32_t CallAnswered(double* value) const;              // g42-9: the call we answer, and its value
  std::vector<uint8_t> home_view_;   // an explorer's tiles in view at spawn (the starters' own ground)
  std::vector<int32_t> AllyHeads(bool skip_queen = false) const;             // this turn: our head is beside a dense bed cluster, early
  int32_t Torus(int32_t a, int32_t b) const;
  double BedWeight(int32_t t) const;                    // a live or soon-due bed (0/1)
  double ClusterScore(int32_t t) const;                 // bed weight within 2 of t
  bool DenseFoodInView() const;
  bool ContestWon(int32_t t, int32_t ready, int32_t* margin = nullptr) const;     // g42-6: we reach t (at `ready`) no later than a rival coming for it
  int32_t contest_goal_ = -1;                           // g42-6: the food we race for (no growth split until it's ours)                         // g42-6: a tile in our 7x7 view with ClusterScore >= kClusterMin
  bool InMiddle() const { return contest_; }            // (name kept from g39-4) contesting a cluster
  void HeadsNear(int32_t at, int32_t r, int32_t* ours, int32_t* theirs) const;
  void UpdateRush();
  int32_t RushGoal(const Paths& paths);
  bot::Action RushSprint(bot::Action a, int32_t goal) const;
  bot::Action FreeSprint(bot::Action a, int32_t goal) const;   // g43-5
  // g42-3 (ring-champfood-1): the late champion grows from pearls (champfood.cpp)
  bot::Action ChampionHarvest();
  bool Banned(int32_t t) const {
    const auto it = banned_.find(t);
    return it != banned_.end() && it->second > round_;
  }
};

}  // namespace gm2
