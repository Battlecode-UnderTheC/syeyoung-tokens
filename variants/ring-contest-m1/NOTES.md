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
Submitted as ring-contest, parent ring-sched-17 (the report has the arena id and verdict).
