# Pre-promotion check: ring-sprint-2 (the founder port) v ring-sprint-1 (2026-10-06)

Saved by the orchestrator from the analyst's final message. Data, scripts and filmstrips: data/, met/, analysis/, img/ in this directory (158 raw replays decoded, then deleted).

## The 58-54 head-to-head is mostly mirror games
- Both bots are deterministic. On 32 of 39 maps ring-sprint-2 plays bit-identically to ring-sprint-1: each seed's seat-swapped pair splits 1-1 (45-45, P(win) 0.500 throughout).
- Only **22 games (11 seeds)** carry information: ring-sprint-2 **13-9** (Colosseum 2-0, arena 1-1, default_small 1-1, queen_of_spades 4-0, stripes 2-2, tower_defense 1-3, trophy 2-2).
- Paired with the parent (same opponent, seed, seat v ring-contest-4, ring-sched-17, ring-rush-1): 336 pairs, 276 byte-identical, 60 differ (the 7 maps above plus one each on portals and prisoners_dilemma). Paired record **200-136 v 196-139**.
- The founder fires only on 1-2-starter maps, plus twice on 3-starter maps when early losses left 3 or fewer dragons before r60 (portals r28, prisoners_dilemma r30). No effect on maze, trauma, weakhold, stronghold.

## Q1. Does the founder do what it claims? Yes, on Colosseum
- Colosseum (films 188760_col_open, 174909_col_open_s1, decoded traces): under ring-sprint-1 the r0 child circles in the home pocket with the queen through ~r26 (2 dragons; the enemy child owns the middle by r17-26). Under ring-sprint-2 the child leaves through the gate by r5-8, is mid-board at r11, eats the centre pearls, length 4 by r17, splits r20 and r23; the queen splits r14-16 (parent r29-31).
- Paired, 6 games: child distance from home at r10/r20 6-8/8-12 v 2/2; dragons r20/40/60 3.2/16.3/32 v 2.0/5.3/13.8; pearls r0-30 14.5 v 5.7; record 6-0 v 3-3. Head-to-head eval 0.95 at r40, 1.0 from r100.
- queen_of_spades: founder born r10-21; dragons r40/60/100 5.6/8.3/11.2 v 4.2/5.7/8.1; paired 9-3 v 4-8; eval +0.20 at r100, +0.35 at r300.
- arena: a wash (paired 3-3 v 3-3, decided by ~r45).

## Q2. By map class
Head-to-head (112 games; P(win) at r100/200/300): 1 starter 3-1 (.75); 2 starters 17-15 (.495/.510/.535); 3+ 38-38 (.500); cramped 18-16; corridor 7-9 (.458/.474/.457); open 33-29; sealed/pen 5-5; central cluster 4-4.

Paired v the parent (336 pairs; net and dP r100/200/300):

| class | n | rs2 | rs1 | net | dP |
|---|---|---|---|---|---|
| 1 starter | 12 | 9-3 | 6-6 | +3 | +.25 |
| 2 starters | 96 | 50-46 | 50-46 | 0 | −.020/−.014/−.003 |
| 3+ starters | 228 | 141-87 | 140-87 | +1 | ≈0 |
| corridor | 48 | 26-22 | 29-19 | −3 | −.058/−.038/−.048 |
| cramped | 102 | 56-46 | 54-47 | +2 | ≈0 |
| open | 186 | 118-68 | 113-73 | +5 | +.013/+.014/+.023 |
| sealed/pen | 30 | 16-14 | 15-15 | +1 | ≈0 |
| central cluster | 24 | 16-8 | 16-7 | 0 | −.02 |

The only negative class is corridor, all of it tower_defense (1-3 head-to-head, 3-9 v 6-6 paired; ~3 independent negative events). Real mechanism (Q3) but too small to call clear.

## Q3. Trophy and tower_defense: the founder causes it
First divergence is at or just after the founder's birth (trophy r5/r12; tower_defense r11 or r47-49).
- tower_defense (films 189000_td_found, 175551_td_found_s1, 189020_td_found/_mid, 182282_*_s1): on some seeds the first child appears at r47-49 and still counts as a founder (cutoff r60).
  - It skips the growth split: v ring-contest-4/ring-sched-17 seat B, the parent's dragon 4 splits r55 and its child takes the bottom bed block; the founder stays length 4 until r57 and its child stays top; [img] the bottom block is the enemy's by r68. Dragons r68/80/92 7/7/2 v 9/10/11; both W→L (queen dies r353).
  - It walks into enemy ground: v ring-rush-1 seat A, the founder goes south through the wrap into the top block where three enemy workers farm, splits r58, resigns r60; 4 v 6 dragons at r60; W→L.
  - Head-to-head seed 642154 lost from both seats; 159649 1-1; dragons at r100 2 v 16 and 3 v 21 in the bad games.
- trophy (films 189042_tr_open/_mid, 175573_*_s1, 189037_tr_open, 175414_tr_open_s1): the founder is the worker's r5 child. The parent's child splits r13 by its own bed patch and reaches the right half of the cup base by r26-29. The founder walks up the right side unsplit at length 5, crosses the wrap toward the enemy half (189042: one child dies there r21-27). [img] the enemy owns the cup r32-56; ours hang outside; dragons r40/60 5/5 v 13/16; W→L. Over 12 trophy pairs: dragons at r40 9.0 v 10.8; paired eval −0.07 to −0.14 from r100. Head-to-head 2-2; its losses (188999) are a midgame cup fight, not a founder failure.
- stripes: the r15-17 founder sometimes walks past the near box to a far one (188915_st_open); 3 W→L, 3 L→W, dP −0.10 at r100. default_small: +2 dragons at r40/60, net −1 (noise).
- **Common thread:** the founder ignores growth splits on its way, and nothing stops it walking into a region the enemy already holds (its rule only counts enemy heads it has seen). On tower_defense it fires far too late to be an opening move.

## Q4. Other side effects: none
- Queen safety (60 differing pairs): queen deaths before r100 13 v 17, before r200 22 v 27; any-time loss 0.65 v 0.70; queen length at r60 2.58 v 2.49.
- Churn: final deaths 56 v 69, self-deaths 33 v 42, splits 68 v 80.
- CPU: p99 25.8M v 24.5M, max 41.9M v 50.3M; no overruns, 0 errors.

## From images v numbers
Images: Colosseum gate exit; tower_defense lost bottom block and the walk into the enemy's top block; trophy wrap crossing and enemy cup ownership r32-56, and the head-to-head midgame cup fight; stripes far box. Numbers: divergence rounds, founder births, counts, queen deaths, class tables.

## Verdict
Promote with the caveat that "beats parent" rests on 22 informative games. Colosseum and queen_of_spades are real gains; tower_defense and trophy are real but small founder-caused regressions, not statistically clear.

**no class is a clear loss**
