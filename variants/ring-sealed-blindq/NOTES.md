# ring-sealed-blindq (g53-3, arena ring-sealed-5)

Base: ring-sealed-wallin (arena ring-sealed-2). Strategist B g53 proposal #2.

## Idea
arena-r27 S1a: 61 deaths of our 13+ queen after their queen was already dead (median 180 rounds);
the killer was outside her view at the start of her turn in 45 of 58, she then took 3-6 free steps
and ended inside its reach. So the late big queen pays for ENDING where she can't see.

## Change (QueenMove beam score only)
- cost = w * blind(end), blind = tiles within Chebyshev 3 of the end tile outside this turn's 7x7
  window = 49 - (7-|dx|)(7-|dy|) (torus offsets, clamped at 7: a portal jump counts as fully blind).
- w = 12/7 per tile x min(1, L/20): "about 7 blind tiles outweigh one pearl" with the beam's real
  pearl gain (+12; the proposal assumed 3 and wrote w ~ .4, the stated intent is kept).
- Active from r400 at length 12+ when the enemy queen was last seen/heard > 60 rounds ago or never
  (BLINDQ_GATE 1). BLINDQ_GATE 2 (her length >= 2x the longest enemy she saw in the last 50 rounds;
  seen_ is per dragon, so "seen by the team" is her own sightings) was built and tested, not kept.
- Label "queen: blind cost" when the pick differs from the pick without the cost; -DBLINDQ_DBG
  prints a BQ line per gated turn.

## Local
- server maps seed 1 v ring-sealed-wallin: 17-17 (mirror), term on 125 turns / 34 matches, changed
  the pick on 79 (label ~2/match).
- fresh mapgen seed 53301, 44 maps x seeds 1,2: 87-2-87, exact mirror in every class; term on 88
  turns / 176 matches, changed 65. Local queens are rarely 12+ at r400 (end length 1.4 mean).

## Arena
- Duels v ring-sealed-2 (random:2): gate 1 server 35-32 (longer queen 19-16; queen len@end 10.99 v
  10.58, alive 58.2 v 56.7%), official 10-10 (queen len@end 12.45 v 15.1, alive 40 v 40%).
  Gate 2 server 31-37 (longer queen 11-18, queen len 9.40 v 9.43): rejected.
- ring-sealed-5 verdict v ring-sealed-2: 55-0-57 (49.1%, loses within noise); server maps 33-35;
  elimination 26-26, longer queen 18-19, longest 10-10, total 1-2. v ring-field-3 62-50,
  v ring-qforage-1 58-54, v ring-sprint-21 55-57. CPU p99 22.7M, max 47.4M.
- The per-game arena analysis (S1a count, alive-at-end when 12+ at r400) could not be run: the
  game listing (tokenz games) was denied by the permission system in this session.
