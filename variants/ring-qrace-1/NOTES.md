# ring-qrace-1 (arena ring-sched-11)

## Idea
Queen-race delivery (g40-4). arena-r14 #4 found 85 late losses where both queens were alive.
From r430 our queen ate 14 to their 22, and was fed 6.6 v 11.2. We ended those games with
16 dragons to their 7, so our material stayed in workers. When both queens live at r500, the
longer queen wins, so worker mass should go into our queen.

## What changed from the parents, and why
Parent: ring-sched-m1 (arena ring-sched-6). There is one change, `QRACE_MODE` (strategy.hpp, `QueenRace` in strategy.cpp):
- **Enemy queen sighting:** each dragon records the last round it saw the enemy queen
  (dragon 0 or 1 of the other team) in view. It relays this in bits 4-8 of the champion sonar
  word: 0 means unknown, otherwise 1 + rounds since the sighting. Decode now rejects only bit 9.
- **Walk and resign:** this runs from r430, the ring4 exit that lets the queen grow, while the
  enemy queen was seen within 30 rounds and our queen is known alive. Within that window, a
  worker that can reach her in at most (500-round)/2 steps walks toward a free tile within 2
  steps of her head. "Can reach in time" means it arrives with at least as many rounds left for
  her to eat its pearls as it walked. It avoids the tiles reserved next to her head. Once it sees
  her and its head is within 2 free steps of hers, it resigns, so its pearls are within her reach.
- If the enemy queen is unseen or stale, the base behaviour is unchanged. Nothing is keyed on a
  map, a board size or a position.

## Local
- **Smoke run, maps/server, seed 1:** 15-0-19 vs ring-sched-m1. The race fires rarely: about
  30 walk turns per match and about 0.3 feed resigns.
- **One traced maze game:** our queen was 37 at r480 vs 30 for the base. She then got trapped
  and escape-split 2 per turn from r487, ending at 11. A longer queen is easier to trap in a maze.
- **Fresh mapgen maps** (seed 740219, 44 maps, seeds 1,2): 86-4-86. Per family:
  - every family 8-0-8, sparse 6-4-6;
  - the queen stats are identical to the base;
  - so the change is inert there: the queens are mostly dead by r430.

## Arena
- **Duel, server suite random:2:** 34-0-34.
  - All economy metrics are identical.
  - queen_length@end is 8.72 v 9.03.
  - CPU max 32.4M.
- The targeted metric did not improve.
- **Draft verdict (complete):** 54-0-47 vs ring-sched-6 (53.5%, beats parent within noise); longer queen 24-18; ring-sched-8 54-51, ring-rush-1 52-53, ring-plus-21 51-51. CPU max 34.9M.
