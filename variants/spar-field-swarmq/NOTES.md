# spar-field-swarmq (g52-18): the queen grows inside her swarm

- **Line:** spar-field (arena `spar-field`), parent spar-field-2 (= variants/spar-field-1).
- **Idea:** strategist-B g52 #6. evals-r13: Vibing++/Sponge queens are 14-15 long at r300 inside their
  swarm with no worse survival (win 94%/78% when she is 8+ at r200); server-r26: 47 losses on the
  longer queen with both alive, their queens 17-56 at r300-350 against our 2-4.

## The change (ring.cpp SwarmGrow/SwarmGraze/SwarmFood, RingQueen; strategy.cpp QueenMove)
- SwarmGrow (queen, after r40): >= 3 ally heads within Chebyshev 4 of her head this turn, no enemy head
  whose lunge reach + 2 reaches her head, and QueenSpace >= 2L+4. Enemy reach or low room set an alarm.
- In the swarm the ring's "pearl split three" threshold is 10 (keeps 8), else 5 as before; the r<100
  bootstrap shorten is skipped while she is in the swarm (or swarm-grown without an alarm).
- She grazes a pearl beside her head, off the ring, when no enemy head is in view, QueenRisk is 0 on the
  pearl and on a free step on from it, the pearl is no Corridor() tile, and the room rule holds.
- Iteration (local counters: the written graze fired ~1 time per 34 games: 456 of 515 swarm turns had no
  adjacent pearl and most of the rest were corridor tiles): in the swarm, with no enemy head in view and a
  non-corridor pearl within 4 free steps, she leaves the ring and the queen beam moves and eats
  (it keeps its own room/risk rules). Off the ring, in the swarm she sheds at 10 to 8 instead of 8 to 6.
- Fallback: a queen who grew in the swarm splits at 5 (keeps 2) when the alarm fires (enemy lunge + 2
  reaches her, or QueenSpace < 2L+4), before r360. Fewer allies only stops her growing (a first
  version split on every one-turn dip in the ally count: 7 -> 2 at r61 in a traced game).
- Debug: `-DSWARM_DBG` prints the gate per queen turn (SWQ/SWB/SWG/SWF lines, QL at r100/200/300/430).

## Local
- v spar-field-1, server maps seed 1: 14-0-20 (as written, before the forage: 19-0-15). Queen length
  (alive queens) r100/r200/r300: 2.7/2.9/2.9 v as-written (~base) 2.6/2.7/2.9; queens alive r200 19 v 22.
  Traced game: she grew 3 -> 7 in r50-r60 inside 4-7 allies, then the crowd alarm (allies' bodies
  around her) split her back to 2 at r64; again 4 -> 2 at r80. The crowd test is what stops the growth.
- Fresh mapgen (seed 52181, 44 maps, seeds 1-2): 87-0-89; anti-centre 9-7, crowded 8-8, maze 9-7,
  mixed 9-7, off-axis 6-10, open 8-8, rich-near-home 7-9, ring 8-8, rooms 7-9, sealed-spawn 8-8, sparse 8-8.
- v ring-sprint-guard2, server seed 1: 19-0-15 (base spar-field-1: 18-0-16); queen died 56% (base 53%).
- `check spar-field-swarmq maps/official`: passed.

## Arena duels v spar-field-2 (eph-965b4d18)
- server random:2: 35-0-33 (51.5%, noise); elimination 13-12, longer queen 16-16, longest 5-5;
  3+ starters 22-18, <=2 13-15. queen_length@end 10.3 v 9.2, queen alive@end 48.5% v 52.9%.
- official random:2: 9-0-11 (noise); queen_length@end 9.4 v 4.9, alive 35% v 40%. CPU max 40.6M.

## Arena verdict, spar-field-3 (commit 83b475f: as written + the forage hand-off to the queen beam)
- vs spar-field-2 51-0-61 (45.5%, CI 36.6-54.8, "loses to parent", noise); server 32-36; decided by
  elimination 27-28, longer queen 11-18, longest 13-15. vs ring-sealed-2 50-62, ring-field-3 51-61,
  ring-qforage-1 45-67. CPU worst p99 24.1M, max 43.5M.
- Own games v parent (112, same games): eval P(win) r100/200/300 0.494/0.508/0.480 (server 0.503/0.521/0.487,
  official 0.41/0.47/0.34). Queen peak length r50-350 3.77 v 3.03; samples with queen >= 5: 178 v 69, so she
  does grow. But queens alive r200 61 v 69, r300 43 v 51; queen deaths r100-430 48 v 42, and boxed
  ("no valid move") queen deaths 14 v 4.
- Decoded: islands 264883 — she grows 2 -> 10 on the island (x13-16, y5-9) among 6-8 of our dragons,
  sheds 8 -> 6 four times there, then a trapped-split cascade 6 -> 4 -> 2 and she resigns at r219;
  maze 264802 — grows to 8-10 four times, ends r382-386 with 10 -> 8 -> 6 -> 4 -> 2 splits in a maze lane
  and resigns; australia 264875 — grows to 8-9 seven times, falls back to 2 at r345, lunged at r414.

## Iteration 2 (not submitted): SwarmWalk instead of the beam hand-off
- She walks to a pearl within 4 steps herself over free, non-corridor, QueenRisk-0 tiles, with the room
  rule on the step. Duel v spar-field-2 server random:2 (eph-94a9dd26): 32-0-36; longer queen 11-19;
  queen alive r300 72% v 89%; boxed queen deaths 10 v 2 (decoded all 68): most follow a fallback/shed
  split (7 -> child 5, 5 -> child 2) and a resign 1-2 rounds later.
## Iteration 3 (the code here, not submitted): her splits need kForageRoom tiles with her whole body blocking
- Duel (eph-b4f65947): 29-0-39; longer queen 10-16; queen alive r300 72% v 89%; boxed deaths still 10 v 4.
  Decoded: now they are long queens (6-10) on portals/maze/islands caught in a lane: generic
  "trapped: split" cascades 8 -> 6 -> 4 -> 2, then resign (games 32, 44, 45, 46, 47 of that duel).
- Conclusion: the queen does grow inside the swarm, but a 6-10 queen in our crowded swarm on cramped maps
  is boxed far more often than the 2-3 ring queen; the crowd test (QueenSpace >= 2L+4 at her head) does
  not catch lanes and pockets. Not ready.
