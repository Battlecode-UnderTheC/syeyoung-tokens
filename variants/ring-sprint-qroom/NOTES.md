# ring-sprint-qroom (g44-10; parent ring-sprint-6 = variants/ring-sprint-conveyor)

## Idea
server-r19 S2: late in the game the queen walks into a 1-wide corridor or a shrinking pocket,
then splits herself to death (7 -> 5 -> 3 -> 2) or resigns boxed. g44-5 showed the trap closes
5-10 rounds before any split and that changing the boxed split does nothing. The user's rule:
the queen plays safe and never enters corridors or pockets. Make her keep that rule while her
space shrinks, before she is boxed.

## What changed from the parent, and why (strategy.cpp QueenMove, QueenSpace; strategy.hpp)
One mechanism: every route the queen's beam scores (outside the opening forage, which keeps its
own room rule) is charged for the room it leaves her.
- **Space** = free tiles she reaches from the route's end strictly before any visible enemy head
  (her new body, every dragon's body and kelp block). A pocket closes because enemy heads take its
  mouth (g44-5's 190320: 40 free tiles ahead -> 4 in four rounds).
- **Need** = 3 x her length, at least 20 (the forage rule's room), at most 90.
- **Cost**: 4 per missing tile (a pearl per 3 tiles), plus the missing share of need taken off the
  route's pearls and food pull, so she doesn't grow where she lacks room.
- **Unseen edges** count 4 tiles, not open ground; a portal whose far end she hasn't found counts 0
  (she never explores one).
The observed cause behind each value is in strategy.hpp (kQueenSpace*). Diagnostic label
"queen: keep room" when the charge changed her route (approximate).

## Iterations (light server duels v ring-sprint-6, 68 games each; both bots in every game)
| version | W-L | our queen boxed/bled | parent's | queen len at end |
|---|---|---|---|---|
| v1 (unseen = open) | 33-35 | 3 | 5 | 8.7 v 9.9 |
| v2 (+ unseen edge 4, unfound portal 0) | 36-32 | 2 | 6 | 12.3 v 10.4 |
| v3 (+ no growth in a pocket) = submitted | 33-35 | 0 | 2 | 10.1 v 11.5 |

Official random:2 (v2): 9-10. "bled" = queen death with 2+ own splits in its last 30 rounds.
- v1's misses, decoded: duel-93c5c033 game 66 (weakhold), a 7-tile strip along the kelp whose far
  end she hadn't seen, entered for 3 pearls, then 7 -> 2; game 57 (around_unsw), a 16-tile pen of
  our own bodies at length 22, then 22 -> 11. Both read as open through unseen edges.
- v2's remaining miss: duel-87737336 game 15 (weakhold), a 3-long queen held in a 13-17-tile pocket
  by enemy heads at its mouth (their QueenRisk outweighs any room cost), ate 6 pearls there
  (3 -> 9), split 9 -> 2. v3 stops the growth in such a pocket.
- Queen deaths overall are unchanged (92 v 90 over the three duels); boxed deaths fall, lunge
  deaths rise slightly (87 v 77). Lunges (S1) are g44-9's lane.

## Local
- check maps/official: pass.
- v3, 5 S2 maps (portals, weakhold, islands, slithery, unsw) x 2 seeds x 2 seats v the parent: 11-9.
- v2 without the unseen-edge rule, v ring-sprint-1, seeds 11-13 on portals, weakhold, islands,
  slithery, australia: 18-12 (parent 17-13). Local portals seed 13: the parent's queen split
  7 -> 3 and resigned at r329; ours lived (28 at the end).
- v2, fresh mapgen (22 maps, seed 4410, sim seed 3) v the parent: 24-20.
- Local games don't reproduce arena games move for move (checked on 191533), so arena replays
  are the evidence.

## Arena verdict
(filled in after the verdict)
