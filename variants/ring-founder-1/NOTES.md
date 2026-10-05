# ring-founder-1 (g43-6; final = arena ring-contest-16; earlier ring-contest-11, -14, -15; line ring-contest, parent ring-contest-4 = ring-contest-m1 code)

## Idea
On 1-2-starter maps (Colosseum 5-25, Trophy 14-46 in arena-r17) the queen sat at length 4 unsplit
and the first child idled at home, so we had 2 dragons until r38-44. Dispatch the first child as
a FOUNDER to the food and let the queen split at 4.

## Change (founder.cpp; hooks in strategy.cpp Decide/QueenMove, rush.cpp RushGoal)
- **Founder:** a child whose first turn finds at most 3 units (itself included) before r60. A child
  split off in round 0 also acts in round 0 (and has leader_ set); it is told apart from a starter
  because its tail touches its parent's tail (docs/game.md SPLIT). No sonar needed.
  - Target: the largest seen pearl region (live pearls linked within 3, the view radius) of 2+
    pearls that it reaches no later than any enemy head seen in the last 20 rounds (RivalDist).
    It holds back from regions an enemy reaches first. Without one: the nearest lone pearl it wins,
    else the frontier walk (RushGoal, ignoring the queen's share of the frontier).
  - No growth split on the way below length 5 (kRaceSplitExempt). In any eligible region it farms
    and splits at 4 ("founder: split in region"), which ends the dispatch.
- **Queen:** on a start of 2 or fewer (her r0 unit count), before r60, in open ground (not a corridor
  tile, 20 tiles of room, no threat) she splits whenever she reaches 4 ("founder: queen split at 4").
- **Not kept: "the queen stays home"** (FOUNDER_HOME, default 0). Holding a foraging queen at home
  left her at length 3 until r48 on Trophy (duel-ed6ff038 game 16), while the base's queen foraged
  and split by r15-20.

## Iterations (local, 26 fresh 1-2-starter mapgen maps, seeds 1,2, v ring-contest-m1)
1. Beds due in 30 or fewer rounds as regions: home counted as a region (Colosseum rim beds). Switched to live pearls.
2. Arrival only at the largest region: founders crossed rich ground unsplit. 41-55 / 40-56.
3. Long founders (children of 6-12 long starters) walked unsplit: 3.1 v 3.6 dragons at r10. Exempted
   length 5+: 50-46 (49-47 with stays-home).
4. Colosseum seed 11: the founder circled its corner (RushGoal flipped between -1 and far tiles; the
   queen's frontier share; lone pearls 3-4 apart never formed a region). Region link 3, a lone-pearl
   fallback, and the queen's share ignored: **54-8-42**.

## Numbers (final)
- **Local, 44 fresh maps (seed 4306121, all families), sim seeds 1,2:** 85-91. Per family: anti-centre
  8-8, crowded 8-8, maze 8-8, mixed 6-10, off-axis 8-8, open 8-8, rich-near-home 8-8, ring 5-11,
  rooms 9-7, sealed-spawn 8-8, sparse 9-7.
  - Ring and mixed rechecked on seeds 1-4: 14-18 and 16-16. That's noise.
  - The 8-8 families are mirror games where the change doesn't fire (3+ starters).
- **Local, official seeds 1,2 (iteration 2):** 31-29.
- **Light duels against ring-contest-4:**
  - server random:2: 37-31 (≤2 starters 17-11, 3+ 20-20; queen alive at end 66% v 56%);
  - official random:2: 12-8;
  - 11 small-start maps, random:3: 33-33.
- **Same-seed Colosseum and Trophy duels (seeds 11-66), ours v the base's record against the same opponents:**

  | opponent | ours | base |
  |---|---|---|
  | crossover-1 | 6-18 | 6-18 |
  | queen-3 | 5-19 | 6-18 |

  Against fast swarms we still lose the fights at r40-60.

## Arena: ring-contest-11
- **v parent ring-contest-4:** 64-0-62 (50.8%), "BEATS parent" but within noise.
  - By suite: server 41-41, custom 17-17, official 6-4.
  - Decided by: elimination 31-27, queen 22-23, longest 11-12.
- **Eval P(win) v parent:** r40 0.505, r100 0.507, r200 0.510, r300 0.517 (126 games).
- **Other opponents:** ring-sprint-1 61-70, ring-sched-17 72-60, ring-rush-1 82-47. Overall 279-239.
  By suite: server 147-125, custom 76-60, official 24-16.
- **CPU:** p99 26.3M, max 48.0M.
- **Target maps:** Colosseum 7-1 (parent 14-38 lifetime), Trophy 6-10 (parent 31-69).
  - Paired on the same opponent, seed and seat (parent games excluded): Colosseum new 5-1 v the
    parent's 3-3; Trophy new 4-8 v 5-7.
- **Replays, Colosseum, ring-sched-17 seat B (same seed and seat):**
  - Game 184321 (new, W): the queen splits at r14, r25 and r29. 3 dragons at r20, 12 at r40, 36 at r60.
  - Game 175691 (parent, L): the queen sits at 4 at r20, then grows to 8 and walks to the middle
    unsplit until r31. 2 dragons at r20, 4 at r40, 4 at r60.
  - Same pattern in 184325 v 176969 (ring-sprint-1).
  - The r0 founder itself still lingers near home for about 15-20 rounds on Colosseum: the queen
    forages in the pocket mouth. Most of the gain is the queen's splits and the second founder.
- **Trophy is not fixed.** The paired record is a little worse, and the cup is still taken by the
  enemy when it gets there first.

## Follow-up (orchestrator: the r0 founder lingers in the home pocket; make the queen stop blocking its mouth)
Arena games showed the founder leaving its Colosseum home pocket late: r10/r30 in ring-contest-11,
r23/r18 in ring-contest-14 (seat A/B). The queen foraged in the pocket's 3-wide mouth from r4 to r19.
- **Iteration 6 (ring-contest-14):**
  - The queen pays 12 for each ally head within 2 of where she'd stop.
  - The founder keeps its way-goal (sticky), and doesn't treat it as a bed to wait at.
  - Once the frontier runs out (the mirror fills memory), it walks to the stalest reachable view.
  - Rival distance to a region is walking distance, not torus distance: the two pockets touch
    across the wrap behind kelp.
  - Verdict: 55-0-57 v parent, loses within noise.
  - Paired same-seed games v the other 3 opponents on 1-2-starter maps: dragons at r20/40/60
    3.8/6.4/8.9 v the parent's 3.9/6.6/9.4. ring-contest-11 was 3.9/6.8/9.6.
  - Colosseum r40 8.5 v 5.3 (ring-contest-11: 12.2).
  - Not fixed: in 186414 the queen still sat in the mouth from r4 to r19. The give-way never fired,
    because the founder was at the back of the pocket.
- **Iteration 7 (ring-contest-15) = iteration 6 + the queen doesn't box a worker in.** Where her body
  would cut a worker within 6 off to under 30 reachable tiles, she pays 24 (BoxedBy).
  - Arena Colosseum: the founder is out of the pocket by r7-8 in all 8 games, and the queen sits
    in the mouth 2-4 turns in r0-19.
  - But 54-0-58 v parent (eval at r100/200/300: .478/.475/.478).
  - Paired 1-2-starter games: dragons at r60 8.2 v 9.4. The iteration-6 founder changes cost elsewhere.
- **Iteration 8 (final, ring-contest-16) = ring-contest-11 + the no-box rule only.** Iteration 6's founder
  changes are dropped.
  - Local Colosseum traces: the founder is out by r8-9 in 8 of 10.
  - Verdict: **58-0-54 v parent** (51.8%, "BEATS parent", within noise).
    - By suite: server 35-33, custom 17-17, official 6-4.
    - Decided by: elimination 30-26, queen 19-19, longest 9-9.
  - Eval P(win) v parent: .508 / .512 / .519 at r100 / 200 / 300.
  - Other opponents: ring-sprint-1 53-59, ring-sched-17 63-49, ring-rush-1 75-37.
  - CPU: p99 26.3M, max 48.0M.
  - Arena Colosseum games: **8-0**.
    - The founder leaves the pocket at r8 in all 8 (ring-contest-11: r10 in seat A, r30-32 in seat B).
    - The queen sits in the mouth 2-5 turns in r0-19, against 15+ before.
    - Dragons at r40: 11-19 (188252: 3/19/34 at r20/40/60).
  - Paired with the parent on the same opponent, seed and seat (the other three opponents):
    - All 1-2-starter maps: W-L 57-51 v 53-55; dragons at r20/40/60 3.9/7.1/9.5 v 3.9/6.6/9.4.
    - Colosseum: 6-0 v 3-3; dragons 3.2/16.0/20.7 v 2.0/5.3/14.2.
    - Trophy: 4-8 v 5-7. Not addressed: the cup race is next round's problem.
