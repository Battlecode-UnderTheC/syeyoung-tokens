# qg-narrow-1

## Idea
A short queen avoids 1-wide tiles. g35-5 found every non-doomed queen resign had zero legal
non-fatal actions, typically an L2 queen in a 1-wide corridor boxed by her tail and an ally head.
Keep a short queen out of corridors in the first place (user rule: the queen plays safe).

## What changed from the parents, and why
Parent qg-board-1 (arena queen-guard-7). One change, in `queen_guard.hpp` `QueenGuard::Guard`:
when the queen's length is <= 4 (`kNarrowMaxLen`), move candidates whose destination is 1-wide
(both neighbours on one axis are kelp or a body; her current head counts as body, it becomes her
neck) are dropped, provided at least one move candidate onto a non-1-wide tile exists. Splits are
untouched. The rest of the guard (risk, room, rollouts) then picks among what remains.

## What should improve
Fewer short-queen corridor traps; fewer queen deaths before r200; no economy cost.

## Local (sim vs qg-board-1, seeds 1-4, both seats, 2 threads)
| maps | W-D-L | decided by (W-L) | queen died | queen deaths < r200 (ours / base) |
|---|---|---|---|---|
| server (80) | 37-0-43 | elim 19-18, queen 3-6, longest 15-19 | 79% vs 82% | 52 / 55 |
| official (120) | 60-2-58 | elim 25-17, queen 15-19, longest 20-22 | 60% vs 63% | 45 / 48 |
| tournament-sprint (56) | 32-0-24 | elim 7-5, queen 13-10, longest 12-8 | 66% vs 68% | 15 / 8 |
| total (256) | 129-2-125 | | | 112 / 111 |

Guard step overrides rise about 20% (server 65 vs 52, official 101 vs 77, sprint 159 vs 135), so it
fires, but "trapped: no move L2-3" fatal turns did not fall (server 134 vs 123). Early queen deaths
are unchanged: most short-queen traps are not entered via a 1-wide tile the guard could refuse
(pockets wider than 1, or traps closed by other bodies later). Neutral locally.

## Arena
(pending)
