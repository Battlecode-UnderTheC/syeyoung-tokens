# ring-sprint-founder (g44-1; arena ring-sprint-2; line ring-sprint, parent ring-sprint-1)

## Idea
Port the proven founder dispatch from ring-founder-1 (arena ring-contest-16, commit f45879b) onto
ring-sprint-1, which is ring-contest-m1 plus free sprints. arena-r18: Colosseum was 4-26 for
ring-sprint-1. The starters idled and the queen walked into the swarm.

## Change
- The exact `diff -ruN variants/ring-contest-m1 variants/ring-founder-1` (founder.cpp; the hooks in
  strategy.cpp Decide/QueenMove/BoxedBy, rush.cpp AllyHeads/RushGoal, strategy.hpp, plugin.cpp
  FOUNDER_DBG), applied to ring-sprint-1.
- The only conflict: plugin.cpp's `#include <cstdio>`, which both sides add. Both behaviours are
  kept. A founder of length 5+ walking "toward goal" may also take free sprint steps.
- Nothing else was added. See variants/ring-founder-1/NOTES.md for the mechanism.

## Local
- Port fidelity, on the 9 fresh 1-2-starter maps (mapgen seed 4419031) with sim seeds 1-4:
  - ring-sprint-founder v ring-sprint-1 went 34-2-36.
  - ring-founder-1 v ring-contest-m1 also went 34-2-36.
  - The opening numbers and label counts are identical ("founder: dispatch" 30), so the port is faithful.
- Fresh mapgen, 44 maps (seed 4419031, all families), sim seeds 1,2: 85-2-89. That's noise.
  - The 3+-starter maps are mirror games where the change doesn't fire.
- Light duels v ring-sprint-1 (eph-d781b113):
  - server random:2, duel-5a693149: 35-33 (≤2 starters 16-12, 3+ 19-21; queen alive at end 60% v 54%; CPU max 32.6M).
  - official random:2, duel-605557db: 10-9 (1 game errored).

## Arena: ring-sprint-2
- **v parent ring-sprint-1:** 58-0-54 (51.8%, CI 42.6-60.8), "BEATS parent", within noise.
  - By suite: server 35-33, custom 17-17, official 6-4.
  - Decided by: elimination 29-27, queen 19-17, longest 10-10.
  - By starters (server/official maps): ≤2 20-16, 3+ 17-17.
- **Other opponents:** ring-contest-4 63-49, ring-sched-17 66-46, ring-rush-1 71-41.
- **Eval P(win) v parent:** r40 .509, r100 .508, r200 .503, r300 .507 (112 games).
- **CPU:** p99 25.8M, max 41.9M.
- **Paired with the parent** (same opponent, seed and seat; the 3 other opponents):
  - All maps: 200-136 v 196-140.
  - ≤2 starters: 59-49 v 56-52; dragons at r20/40/60 3.9/7.3/10.3 v 3.9/6.8/9.6.
  - Colosseum: 6-0 v 3-3; dragons 3.2/16.0/31.0 v 2.0/5.3/15.8. Colosseum overall 8-0.
  - queen_of_spades 9-3 v 4-8.
  - Not fixed or worse: trophy 4-8 v 5-7 (r40 dragons 8.9 v 11.2); tower_defense 3-9 v 6-6
    (identical dragon counts through r40, so this is later play); default_small 1-5 v 2-4.
  - 3+ starters: 60-42 v 59-43, with identical opening counts (the change doesn't fire).
- **Replays (decoded, Colosseum):**
  - 188757 v ring-sched-17 seat A: the r0 child walks out of the corner, at (0,1) r0 → (6,2) r9 →
    (8,5) r18. The queen splits at r14. Dragons 3/19/34 at r20/40/60.
  - Paired parent game 174877: the child circles (0-1, 0-1) until r21, and the queen first splits
    at r29. Dragons 2/8/28.
  - 188760 v ring-rush-1 seat B: the founder is at (9,8) by r12 and the queen splits at r16.
    Dragons 4/13/35.
  - Paired parent game 174909: the child stays within 2 tiles of home through r24, and the queen
    never splits again. Dragons 2/2/2.

---

# ring-sprint-1 (g43-5; parent ring-contest-m1 = arena ring-contest-1)

## Idea
Use free sprint steps. In a MOVE the first ceil(L/4) steps are free, so a dragon of length 5+
gets a free second step and length 9+ a free third. devtest1 (server #1) takes the free second
step on 42-54% of eligible moves; ring-contest-1 on 2-5%, so our long dragons walk at half speed.

## Change (one behaviour)
- `FreeSprint` (rush.cpp): a non-queen dragon of length 5+ on a plain goal walk ("toward goal",
  "contest: race/explorer food") appends up to ceil(L/4)-1 extra FREE steps (never a paid one).
  Each extra step passes the single step's checks: a free tile in this turn's view, not beside an
  enemy head we won't trade with (Exposed), room >= RoomNeeded(), escape level >= 1 and 2
  liberties for the body as it will be after the step; and it stays on a shortest known path to
  the goal (onto the goal only when it is a free tile in view).
- Not when the goal is food not ready on arrival or a bed before its tick (arriving early only
  waits or blocks the tick), not on bed waits/skips, not walking to an ally (feeding), not for the
  rush explorer (it keeps RushSprint), never for the queen (her rules are unchanged).
- v2 gate (SPRINT_MODE 2): no sprint while an enemy head is in view this round. Observed cause: in
  v1 our 10+ dragons were lunged 9.8 times a match (160 length) v 6.8 (101) for the base on fresh
  maps; near an enemy the old single-step logic and the rollout review decide as before.
- Body memory fix (strategy.cpp Observe): after a multi-step move the base remembered only the new
  head (gap in body_). It now rebuilds the body front from the in-view segments up to the old head.
- Debug: `-DSPRINT_DBG` prints `SPR r<round> len<L> steps<n> <reason>` per move (plugin.cpp).

## Local (sim, vs ring-contest-m1)
- Fresh mapgen maps (seed 43517, 44 maps, 11 families): v1 93-6-77 (seeds 1,2); v2 94-6-76
  (seeds 1,2), total length lead at the end +22.5/+28.6, longest +1.8/+2.9, longest-decided 37-20.
  Per family (v2, seeds 3,4, 16 games each): anti-centre 8-0-8, crowded 8-0-8, maze 7-4-5,
  mixed 8-4-4, off-axis 10-0-6, open 10-0-6, rich-near-home 10-0-6, ring 11-0-5, rooms 8-2-6,
  sealed-spawn 8-0-8, sparse 8-0-8. No family lost.
- Share of length-5+ non-queen moves with 2+ steps: base 1.7%; v1 33.6%; v2 28.9%
  (5-8 long: 24-37% by phase; 9-12: 16-35%; 13+: 17-33%; highest after r350).
- v2 lunge deaths of 10+ dragons: 8.7/match (138 length) v 7.1 (108) for the base: still up, but
  we also have more and longer dragons.
- maps/server seed 1 (v1 smoke): 16-0-18.

## Arena duels vs ring-contest-1 (random:2 seeds)
- v1 (eph-64eb7dd9): server 38-0-30, official 11-0-9; longest-decided 13-1; longest@400 12.9 v
  10.4 (server), 17.6 v 14.6 (official).
- v2 (eph-4e4cde2e): server 36-0-30 (2 errored); longest@400 11.6 v 12.5; queen-decided 19-13.
  official 12-0-8; longest-decided 4-0; longest@400 22.5 v 17.5; CPU max 41.1M.

## Arena verdict
Not submitted by the worker (the permission system blocked `tokenz submit`); bot dir /tmp/g43-5-final.

# ring-contest-m1 (g42 merge step; first bot of the arena line ring-contest)

Base: ring-sched-m3 (= arena ring-sched-17, the live server bot). Operator: merge. The line is
named after its defining feature: workers contest the food they can reach first in the opening.
(The brief called it ring-claimmid; the user renamed it, since it contests reachable food and
doesn't aim at the middle.)

## Stacked (each as its exact diff against ring-sched-m3, in this order)
1. **ring-contest-2** (core; includes contest-1): in r<60 food a worker reaches no later than a
   recently seen enemy head is worth x2; explorers take contested or near food; no split while
   racing, except length 5+; explorers may re-split on dense food. Labels "contest: race",
   "contest: explorer food".
2. **ring-qgate-1**: the r430 Ring4Exit and Ring4Supply only while EnemyQueenRace() (the enemy
   queen seen alive since r300, not seen to die; never seen at all still exits). The ring sonar
   word (0xBA) carries the last sighting in bits 26-34 + valid bit 35. Debug: QGATE_DBG.
3. **ring-champdie-1** (includes champfood-1): one agreed champion from r400 (queen not known
   alive); starving stubs (<=3, nothing eaten for starve_rounds) resign beside it; champion
   harvest sprint ("champion: harvest"). Debug: CF_DBG.
4. **ring-clustercall-2** (includes clustercall-1): the early cluster call, detour bonus on food
   goals, answerers capped by the surplus. Label "call: toward cluster".

Patches 1-3 applied with no conflicts (they touch disjoint hunks). Build + check passed after each.

## Conflicts resolved by hand (step 4)
- **Ring-word bits.** clustercall-2 put the call in bits 26-52 of the 0xBA word and cleared them,
  which would wipe qgate's sighting (26-35). The call moved to the 20 free bits 36-55 and was
  packed: x (6) | y (6) | surplus (4, capped at 15; 0 = no call) | round found mod 16 (4).
  A call is never relayed once older than kCallFresh = 8, so mod 16 recovers the round exactly.
  HearCall takes the round mask (511 for the 0xA9 call word, 15 for the ring word).
- **Contest vs call** (the -27/-29 review: the two fought over the same r<60 workers):
  - CallAnswered() returns none while the worker had a contest goal last turn (contest_goal_).
  - In ChooseGoal the contest test is computed once per tile (`contested`), before the detour
    bonus; contested food gets no call bonus, only the contest boost. Contest behaviour without
    calls is unchanged.
  - Verified with a CLAIMMID_DBG build (maps/official, seed 1, v ring-sched-m3): 2114 worker-turns
    with a contest goal, 2188 with a call answer (direct or detour), **0 with both**.

## Numbers
- **4 in or out:** arena duel duel-da2ecd35, stack 1-4 vs stack 1-3 (both ephemeral), server
  random:2: **38-30** (55.9%; elim 14-11, queen 18-13, longest 6-6; <=2 starters 17-11, 3+ 21-19).
  Local maps/server seeds 1,2: 39-29. Not worse, so 4 stays in.
- **Light duels vs ring-sched-17** (eph-049ca075 = stack 1-4):
  - server random:2, duel-0e326b2f: **38-30** (55.9%; elim 16-12, queen 18-13, longest 3-5;
    <=2 starters 17-11, 3+ 21-19). Queen alive at end 57.4% v 52.9%.
  - official random:2, duel-83ae8a8f: **10-10**.
  - custom fixed-maze-wide random:2, duel-89580874: 3-1.
- **Fresh mapgen** (seed 1791200825, 44 maps, sim seeds 1,2, v ring-sched-m3): **104-72**.
  Per family: anti-centre 8-8, crowded 11-5, maze 10-6, mixed 7-9, off-axis 7-9, open 12-4,
  rich-near-home 11-5, ring 8-8, rooms 11-5, sealed-spawn 9-7, sparse 10-6. No clear loss.
- **CPU:** max 41.9M (official duel), 32.4M (server duel). Under 80M.
- Local official seed 1 (debug build) v ring-sched-m3: 18-12. Labels firing: "contest: race" 54,
  "contest: explorer food" 2, "call: toward cluster" 39, "champion: harvest" 32.

## Replays (arena duel games, rendered with server-r16/analysis/render.py)
- **big_empty opening, duel-83ae8a8f g6 (W, longest 72-36):** our pack stays tight in its corner
  through r24 (36 v 42 total at r24), and one worker wraps north. In this seed the explorers do
  NOT visibly cross the board by r15, unlike the review seed, and the opening is slightly behind.
- **fixed-maze-wide opening, duel-89580874 g0 (W, longest 17-11):** our two long starters split
  by r5 (5 -> 9 dragons), and the children walk the corridors north and to the centre by r10-20.
  9 v 9 dragons at r40.
- **Enemy queen dies, ours stays parked, duel-0e326b2f g38 weakhold (W, queen 2-0):** the enemy
  queen dies at r354 (head to head); our queen stays at length 2 in the same spot through r499,
  with no r430 exit (film r398-462). The gate does what it's meant to.
  Also g67 islands: the enemy queen dies at r383, our queen grows 5 -> 46 by r499 and survives.
- **Queen-dead late game, champion eats, duel-83ae8a8f g6 big_empty:** both queens dead by r305.
  Our champion (id 486) goes 22 (r430) -> 41 (r480) -> 65 (r496) -> 72, eating 66 pearls in
  r400+, 13 of them on our own dead's tiles and 10 on the enemy's. That's the intended
  behaviour. Counter-example: g7 (seats swapped), where our champion stayed at 39, ate 5 late
  pearls, and lost on longest 39-40.

## Arena
Submitted as **ring-contest-1** (line ring-contest, parent ring-sched-17); not promoted.
- vs ring-sched-17: **66-0-46** (58.9%, CI 49.7-67.6): BEATS parent, within noise.
  By suite: server 43-25, custom 19-15, official 4-6. Decided by: elim 33-18, queen 22-17, longest 11-11.
- vs ring-plus-21 68-44, ring-rush-1 71-1-40, cs-queen-split-tail-risk-v1-7 73-39.
- Worst maps over all opponents: server/trophy 3-13, official/Colosseum 2-6.
- CPU: max 48.0M, p99 26.3M.
