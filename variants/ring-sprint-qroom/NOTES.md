# ring-sprint-qroom (g44-10; parent ring-sprint-6 = variants/ring-sprint-conveyor)
Arena: ring-sprint-12 (v3), then ring-sprint-16 (v4, this code).

## Idea
server-r19 S2: late in the game the queen walks into a 1-wide corridor or a shrinking pocket,
then splits herself to death (7 -> 5 -> 3 -> 2) or resigns boxed. g44-5 showed the trap closes
5-10 rounds before any split and that changing the boxed split does nothing. The user's rule:
the queen plays safe and never enters corridors or pockets. Make her keep that rule while her
space shrinks, before she is boxed.

## What changed from the parent, and why (strategy.cpp QueenMove, QueenSpace; strategy.hpp)
One mechanism: every route the queen's beam scores (outside the opening forage, which keeps its
own room rule) is charged for the room it leaves her.
- **Room** = free tiles she reaches from the route's end (her new body, every dragon's body and
  kelp block). An unseen edge counts 4 tiles, not open ground; a portal whose far end she hasn't
  found counts 0 (she never explores one).
- **Need** = 3 x her length, at least 20 (the forage rule's room), at most 90. 4 per missing tile,
  plus the missing share of need taken off the route's pearls and food pull (no growth in a pocket).
- **Can't fit** (room < length + 2): a sure bleed, weighed 1e6 like "no escape", so it outranks
  the risk of a lunge (10000 x QueenRisk).
The observed cause behind each value is in strategy.hpp (kQueenSpace*). Diagnostic label
"queen: keep room" when the charge changed her route (approximate; local sims only).

## Iterations (light server duels v ring-sprint-6, 68 games, both bots in every game)
| version | W-L | our queen boxed/bled | parent's | queen len at end |
|---|---|---|---|---|
| v1 own-first room v enemy heads, unseen = open | 33-35 | 3 | 5 | 8.7 v 9.9 |
| v2 + unseen edge 4, unfound portal 0 | 36-32 | 2 | 6 | 12.3 v 10.4 |
| v3 + no growth in a pocket (= ring-sprint-12) | 33-35 | 0 | 2 | 10.1 v 11.5 |
| v4 physical room (no enemy heads) + can't-fit 1e6 (= ring-sprint-16) | 36-32 | 4 | 6 | 9.1 v 8.3 |

Official random:2: v2 9-10, v4 9-11. "bled" = queen death with 2+ own splits in its last 30 rounds.
- v1's misses: duel-93c5c033 game 66 (weakhold), a 7-tile strip whose far end she hadn't seen,
  entered for 3 pearls, then 7 -> 2; game 57 (around_unsw), a 16-tile pen of our own bodies at
  length 22, then 22 -> 11. Both read as open through unseen edges.
- v2's miss: duel-87737336 game 15 (weakhold), a 3-long queen held in a 13-17-tile pocket by enemy
  heads at its mouth ate 6 pearls there (3 -> 9), split 9 -> 2.

## Arena verdict, ring-sprint-12 (v3)
- vs parent 55-0-57 (49.1%, loses within noise); server 34-34, custom 17-17, official 4-6.
  Decided by elimination 24-32, longer queen 19-15, longest 11-10.
- vs ring-contest-4 56-56, ring-sched-17 69-43, ring-rush-1 72-40.
- CPU p99 33.4M, max 41.9M. Eval P(win) v parent r100/200/300: 0.500/0.495/0.491.
- Step 6, paired with the parent's games (same opponent, map, seed, seat; contest-4 and sched-17,
  224 games): 180 identical; wins 125 v 136; boxed-or-bled queens 14 v 13. **Not fixed.**
  199911 (portals) = parent's 191524 move for move: at r477 a 19-long queen, enemy heads 4-5 off,
  ate a pearl into a 9-tile strip (its far end unseen, so the escape test read it as open), then
  split 23 -> 2. The room cost (hundreds) lost to QueenRisk (10000 per unit), g44-5's finding.
  Four late long queens (31-44) were lunged where the parent's lived (200156, 199980, 200052,
  199887): the enemy-head part of v1-v3 pushed her off every nearby head (a flee).
- v4 answers both: can't-fit weighed like no escape, enemy heads out of the room count.

## Local
- check maps/official: pass.
- v4, 5 S2 maps (portals, weakhold, islands, slithery, unsw) x 2 seeds x 2 seats v the parent:
  12-8 (v3 11-9).
- v2, fresh mapgen (22 maps, seed 4410, sim seed 3) v the parent: 24-20.
- Local games don't reproduce arena games move for move (checked on 191533), so arena replays
  are the evidence.

## Arena verdict, ring-sprint-16 (v4)
(pending)
