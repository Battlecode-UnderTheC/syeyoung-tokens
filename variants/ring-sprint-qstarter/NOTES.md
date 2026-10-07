# ring-sprint-qstarter (g46-2; line ring-sprint, parent ring-sprint-21 = variants/ring-sprint-guard2)

## Idea
Strategist g46 A, S2: on 1-2-starter starts the queen is a starter. 78 of 125 server elimination
losses are on 2-starter maps; our queen eats 0.7 pearls by r40 v their 2.0, and at r60 her
descendants are 1.9 v 4.8 dragons. In the parent she either stopped foraging once the team had 6
dragons (FORAGE35's settle: Trophy r24), or never foraged at all when she started at length 4
beside a 4-long ally (FORAGE35's "producer" gate: weakhold, trauma, maze, stronghold, Colosseum
bootstrap the ring at r0). ring-qforage-1 did the 3+-starter version.

## Change (strategy.hpp QSTARTER_MODE, strategy.cpp QueenMove; 3+-starter play byte-identical)
- At r0, units <= 2: she forages (queen_forage_ = queen_growing_ = true) whatever her length or her ally's.
- Lead: our other starter (id + 2; map dragons alternate teams). Each turn its head is in her view
  she records it (last head, round, trail of head tiles).
- While no pearl is in her 7x7 view and the lead was seen within 12 rounds, every route endpoint
  costs 1.0 per tile of path distance (kelp/portal aware, bodies ignored) beyond 4 from the lead's
  last head, and 6 if within 2 (2-4 behind it). With a pearl in view the pull is off: she eats.
- A corridor tile the lead's head has stood on doesn't cost her the forage corridor penalty (it
  went first); other corridors still do. Open-room penalty, escape/liberty guards, threat map unchanged.
- She splits at 4 on the existing queen17 forage path (open ground, no threat, no corridor).
- Settle into the ring at r60 (kQStarterUntil = kContestUntil) instead of at 6 dragons / r120.
- Tried and dropped: also settling when fewer than 20 tiles are free around her. On weakhold it
  fired at r40 among our workers; the ring then formed on top of them and ring5 resigned 4 of them
  (weakhold 0-8 locally; without it 3-5).

## Local (sim, vs ring-sprint-guard2)
- check maps/official: passed.
- Server maps seeds 1,2: 36-32 (elimination 15-11, queen 15-15, longest 5-5).
- 1-2-starter server+official maps (11 maps), seeds 11,12: 20-24; seeds 1,2 (match): 26-18.
  Trophy/QoS/stripes/TD/weakhold seeds 1-4: 20-20.
- Fresh mapgen, 1-2-starter maps only (61 maps from seeds 46201/46202), seed 1: 66-7-49. By
  family: anti-centre 9-7, maze 8-4-6, mixed 6-10, off-axis 6-4, open 2-4, rich-near-home 10-6,
  ring 8-8, rooms 9-2-1, sealed-spawn 2-2, sparse 6-1-1. Dragons at r100 31.0 v 26.6 and 26.9 v 23.2.
- The queen on the 11 1-2-starter maps, seeds 1,2, both seats (44 games, ours v the parent's
  self-play): pearls eaten by r40 3.41 v 1.91; her splits before r60 2.36 v 1.41; dead before r100
  12 v 11. QoS 2.25 v 1.00 eats, 2.25 v 0.50 splits; Trophy 5.75 v 5.50 (she already foraged
  there locally); stronghold 5.25 v 0.50; maze 4.0 v 0.0; Colosseum 8.5 v 4.75. Trauma and weakhold
  stay at 0 eats: their home areas are barren and the lead starts out of view.
- Traces: QoS seed 1 r9-r12 she trails the lead along rows 4-5 one tile behind and across the
  torus edge; it then turns north up a corridor its own split child blocks and she turns back.
  Trophy seed 1: she is in the cup (8,10)-(8,16) by r13-r18, splits at 4 at r13, eats on to r24,
  and keeps foraging to r60 (the parent settled at r24 at 6 dragons).
- Local play is against our own home-bound bot, so the r20-40 race the server loses is not
  reproduced here (the parent's queen already eats 5.5 by r40 on Trophy in self-play).

## Arena
Not run: `tokenz ephemeral` was blocked by the permission system for this worker, so no duels,
submission or arena replay check. Ready to submit:
`scripts/make-bot.sh variants/ring-sprint-qstarter <out>` then
`tokenz --agent g46-2 submit <out> --bot ring-sprint --parent ring-sprint-21 --note "..."`.
