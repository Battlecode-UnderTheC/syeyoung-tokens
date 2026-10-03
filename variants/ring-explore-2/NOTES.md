# ring-explore-2

## Idea
ring-explore-1 (arena ring-plus-4) ported only the hunger part of explore-3's explore
multiplier and beat its parent 57-26. Top teams fill the map early (REPORT.md s8: 52 dragons
over every quadrant by r92). Add explore-3's second term: the unseen share of the board.

## What changed from the parent, and why
Parent: ring-explore-1 (arena ring-plus-4). One change in ChooseGoal (`EXPLORE_UNSEEN`,
strategy.cpp/.hpp): before r150 (`kHungerUntil`), a non-queen dragon's explore multiplier
gains + (tiles never seen in its board memory) / (board tiles), exactly explore-3's
kUnseenWeight = 1 term, additive with the hunger part (so up to x3 at the very start).
The queen is untouched (IsQueen() gate).

## What should improve
More dragons at r100 and more pearls before r60, especially on big, empty maps.

## Local (vs ring-explore-1, seeds 1-4, both seats)
- maps/server: 38-0-42. Decided: elimination 20-15, queen 8-17, longest 10-10.
  r50 12.9 v 12.2 dragons (total 32.4 v 31.1); r100 20.6 v 19.3 (50.9 v 48.0).
  Queen died 65% v 61%; queen at end 3.6 v 5.5.
- maps/tournament-sprint: 33-0-23. Elim 11-6, queen 20-17, longest 2-0.
  r50 14.2 v 13.8 (34.9 v 34.2); r100 28.0 v 26.4 (68.4 v 64.1).
- maps/official: 55-0-65. Elim 24-28, queen 23-24, longest 8-13.
  r50 12.7 v 12.5 (31.4 v 30.8); r100 22.0 v 22.2 (56.4 v 55.2).
- Total 126-130 (noise). Economy gain at r100 on server/sprint maps; flat on official.
  Weak spot: server queen-decided 8-17.
- check maps/official: all passed.

## Arena verdict
Arena id ring-plus-7 (draft, parent ring-plus-4), not promoted (orchestrator decides).
- vs parent ring-plus-4: 47-0-31 (60.3%, CI 49.2-70.4) -> beats parent, within noise;
  server maps only 42-26 (61.8%). Decided by longer queen 24-12, elimination 17-14,
  longest dragon 6-4, total 0-1.
- vs queen-ring17-first-then-grow-5 36-42, queen-ring14-portal-safe-1 45-33,
  queen-guard-7 59-19. Overall 187-125 (59.9%).
- Economy (mean dragons alive, all games): r50 12.9 v 12.4, r100 24.4 v 21.3.
- Worst maps: Colosseum, default_small, server/default, prisoners_dilemma, big_empty (3-5/6-10).
