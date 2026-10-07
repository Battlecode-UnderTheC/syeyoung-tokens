# ring-sprint-carpet (g47-3)

- **Line:** ring-sprint. **Parent:** ring-sprint-21 (variants ring-sprint-guard2).
- **One change (carpet.cpp, hook in Decide step 5):** r30-r249, non-queen idle workers hold the bed
  fields as a spread-out carpet. A worker whose goal is not food, with an ally head within 2, or whose
  food goal is nearer to another ally head while it is in a scrum, walks to ground in reach 10 that
  maximises gap to the nearest ally head (cap 5) + unheld seen beds within 2 (cap 4) + staleness
  (up to 2) - 0.25/step; no dead-end targets. A worker with no pearl in view and nothing eaten for 8
  rounds walks to an untaken exit (portal far side or corridor tile). SafeStep + lunge guard decide the step.

## Iterations (local: 44 fresh mapgen maps seed 47301, seeds 1,2, v ring-sprint-guard2; off = 88-0-88)
- v1 (no food goal within 4 -> carpet): 54-1-121; arena duel-08712fa4 server 27-0-41, official
  duel-93aed9fa 5-0-15. Ally-within-2 at r60 .45 v .52 (spacing worked) but bc r60 .485, dragons
  r100 24.0 v 26.4, board_control@200 .42 v .54: spreading cost food.
- v1 keep any food goal: 58-1-117; v1 no leave: 65-1-110.
- v2 (exploring/patrol goals kept unless crowded, stale bonus): 72-0-104.
- v3 (current; food overridden only in a scrum and only toward unheld beds): 91-0-85.
  Arena duels (eph-df59596f): server duel-3d1d0ced, official duel-d28dd198, pending at handback.
