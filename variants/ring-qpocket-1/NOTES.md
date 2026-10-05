# ring-qpocket-1 (arena ring-sched-21)

## Idea
server-r15 #4: 6 late queen trap deaths while ahead (P .77-.97, r260-481), no enemy involved.
Each was a 2-3 long queen in a 1-exit dead end, often after 3-5 split-2s in the last 8 rounds
(games 1070413, 1071898, 1075589, 1078562). The split children stay where her tail was and
seal the pocket behind her.

## What changed from the parents, and why
Base ring-sched-m3 (arena ring-sched-17). Only QueenMove in strategy.cpp changes:
- (a) From r250 (`kQueenPocketFrom`; the earliest observed trap death was r260), the queen's
  harvest/growth pick takes -3000 for an endpoint with exactly 1 free onward exit. Kelp, walls,
  bodies and her new body all count as blocked. The penalty applies only when the ground behind
  that exit holds fewer than length+4 open tiles, i.e. a pocket she can't turn in. Only the pick
  uses it, not the beam ordering.
- (b) "queen11: wait at closed corridor" splits 2 at most once per 8 rounds (`kQueenWaitGap`;
  the trap chains were 3-5 splits within 8 rounds), and only from length 6+ with SplitSafe(2).
  Otherwise she takes the move while she is still long.
- NOT changed: the "queen10: boxed legal split" chain. I tested a version that resigned unless
  the split-2 chain could reach r499: queen alive at the end fell from 54% to 40% (duel 29-39).
  Allies often move off and free her, so the chain stays.

## Iterations (duels vs ring-sched-17, server suite, random:2, 68 games each)
1. (a) penalising every 1-liberty endpoint, plus (b) including the queen10 resign: 31-37, queen
   alive 43% v 60%. Split diagnostics:
   - (a) alone: 33-35, alive 56% v 56%, queen length 9.4 v 10.0.
   - queen11 wait rule + queen10 resign: 28-40, alive 37% v 56%.
   - queen10 resign alone: 29-39, alive 40% v 54%. The queen10 resign is the harm.
2. (a) + the queen11 wait rule: 36-32 (alive 57% v 54%, length 9.6 v 8.4). A rerun went 32-36
   (alive 54% v 60%, length 7.4 v 10.0). Official random:2: 11-9.
3. (a) with the OpenRoom pocket test + the queen11 wait rule (this version): 34-34 and 34-34
   (alive 59% v 57% and 51% v 54%; length 8.9 v 8.0 and 6.3 v 8.5).

Local: fresh mapgen seed 41404, 44 maps (11 families x 4), seeds 1,2 vs ring-sched-m3: 89-87.
Every family went 8-8 except off-axis (9-7). The change rarely fires in local sims, which are
mostly eliminations.

## What should improve
Fewer late non-enemy queen deaths while ahead. That event is about 3% of games, so 136 duel
games can't resolve it; the arena's full run is the test.

## Arena verdict
Draft ring-sched-21 (not promoted; beats_parent: no, even).
- vs parent ring-sched-17: 56-0-56 (50.0%, CI 40.9-59.1). By suite: server 35-33, custom 16-18,
  official 5-5. Decided by elimination 23-23, longer queen 23-22, longest dragon 10-11.
- vs ring-rush-1 69-43, cs-queen-split-tail-risk-v1-7 70-42, ring-plus-21 62-50.
- Eval P(win) vs parent at r100/r200/r300: 0.500/0.500/0.500 (the games are identical until r250).
- CPU: p99 27.3M, max 38.4M.
