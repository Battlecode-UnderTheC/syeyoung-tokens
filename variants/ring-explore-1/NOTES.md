# ring-explore-1

## Idea
Top teams fill the map early; our workers sit in the start quadrant (REPORT.md s6/s8:
0.2 of the map visited by r100 v 0.6 in ring17's cramped losses). explore-3's sim ablation
found only the hunger multiplier helped. Port that multiplier to the ring line, workers only.

## What changed from the parents, and why
Parent: ring-forage-1 (arena ring-plus-1). One change in ChooseGoal (`EXPLORE_HUNGER`,
strategy.cpp/.hpp): before r150 (`kHungerUntil`), a non-queen dragon's unexplored-edge
explore score is multiplied by 1 + min(1, rounds since it last ate (or was born) / 20),
i.e. up to x2 after 20 hungry rounds. explore-3's unseen-share term is not included.
The queen is untouched (IsQueen() gate).

## What should improve
More dragons and pearls early on maps where workers are stuck at home.

## Local (vs ring-forage-1, seeds 1-4, both seats)
- maps/server: 40-0-40. Decided: elimination 14-23, queen 14-8, longest 10-9, total 2-0.
  r50 12.8 v 12.8 dragons (total 31.7 v 32.2); r100 19.6 v 20.1 (48.8 v 49.9).
- maps/tournament-sprint: 31-0-25. Elim 11-7, queen 19-17, longest 1-1.
  r50 15.3 v 13.3 dragons (37.6 v 32.7); r100 29.6 v 24.4 (73.3 v 59.5). Queen died 32% v 43%.
- maps/official: 63-0-57. Elim 28-27, queen 23-24, longest 12-6.
  r50 12.4 v 12.9 (30.3 v 31.7); r100 21.3 v 21.9 (54.3 v 55.6).
- Total 134-122. Clear economy gain only on the Sprint maps; slightly behind early on
  server/official maps (more elimination losses on server maps).
- check maps/official: all passed.

## Arena note
- Hypothesis: hungry workers chasing unexplored ground spread the team earlier (more
  pearls before r60, more dragons at r100/r200).
- Change: before r150, non-queen explore score x(1 + min(1, hungry rounds/20)).
- Local numbers: 134-122 vs ring-forage-1 (server 40-40, sprint 31-25, official 63-57);
  Sprint dragons at r100 29.6 v 24.4; server/official -0.5 dragons at r100.

## Arena verdict
