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

## Arena: ring-sprint-23 (commit 8463d58, submitted by the orchestrator), BEATS parent (within noise)
- vs ring-sprint-21 85-0-81 (51.2%); server maps 54-52. Decided by: longest 28-30, longer queen 31-25,
  elimination 26-26. vs ring-qforage-1 79-89, ring-contest-4 90-82, ring-sched-17 112-60.
- CPU worst p99 24.0M, max 44.4M.
- The 112 games listed v the parent: 3+-starter games are mirror-identical (eval exactly 0.500; server
  20-20, custom 17-17, official 1-1). 1-2-starter games 17-19 (server 14-14, official 3-5); eval
  P(win) r60/100/200/300 .454/.499/.471/.466.
- 1-2-starter games, our queen v the parent's queen in the same games (36): pearls eaten by r40
  2.67 v 1.44 (>=2 in 21 v 12 games); queen descendants alive at r60 4.92 v 5.39; queen dead before
  r100 6 v 7. Per map W-L: QoS 4-0, maze 2-2 (eats 5.0 v 0), stronghold 1-1 (8 v 0), trophy 2-2
  (2.5 v 2.25, descendants 7.25 v 8.0), weakhold 1-3 (0.25 v 0; descendants 3.75 v 4.75), trauma 2-2
  (0 v 0), stripes 1-3, TD 2-2, Colosseum 0-2, default_small 1-1, arena 1-1.
- Films r0-40:
  - Trophy 215257 (1.2.7 map: queens in side rooms, cup in the middle): she leaves her room by its
    west exit at r15-20 and roams the top edge outside the cup, eating 5 by r40; she is NOT in the cup
    by r20-25. The lead starts in the bottom corner, never in her view, so the follow never engages.
    The parent's queen sits in its room. Same in 215230.
  - QoS 215245: she follows the lead north 1-2 tiles behind (r5-r15) and is beside the top bed room
    by r20-25, but not inside (its entrance is a corridor the lead never took); ate 3 by r40.
  - weakhold 215270: our queen stays in the top-left corner inside a crowd of our own workers (two of
    them die beside her at r30); team 6 v 9 dragons at r58. The parent's queen parks in its room.
- Not fixed by this change: Trophy-like starts where the lead is out of view (no lead position is
  relayed by sonar); 4-long-queen barren-home starts (weakhold) where she crowds the workers.

## v2: the lead's head relayed by sonar (after ring-sprint-23)
- Why: on Trophy (215257, 215230) the lead starts 13 rows away and never comes into her view, so
  the follow never engaged.
- Change: new sonar word 0xBC (team bit 55, lead id 43-54, x 37-42, y 31-36, round 0-8). The lead
  (map dragon id 2/3 counting 2 of us at r0) sends its head on all four directions before r60 in
  place of its champion reports 0xA7; any non-queen ally that heard it within 12 rounds relays it on
  two rotating directions, only over a 0xA7 word or an empty slot (never over ring words 0xBA, so
  the ring relay and the call in it are untouched). The queen takes a relayed head as the lead's last
  seen head (no trail marks from relays); her pull BFS treats edges she hasn't seen as open.
- 3+-starter games stay identical (devil, dilemma, portals, schooltime, default seed 1: same result,
  totals and deaths). A 4-long queen that splits at r0 (weakhold, trauma) makes the lead count 3:
  no relay there (left alone on purpose).
- Reception, Trophy seeds 1-3, both seats: one direction a turn gave 0-1 words in 60 rounds; all
  four from the lead: 1-11 distinct words (seed 1 still none). QoS 2-12, TD 5-24.
- Local v guard2: check passed; server maps s1,2 36-32 (elim 14-10, queen 14-16); 11 1-2-starter
  maps s1,2 26-18, queen eats by r40 3.36 v 1.91, splits <r60 2.30 v 1.41, queen dead <r100 15 v 11
  (v1: 26-18, 3.41, 12); fresh 1-2-starter mapgen (61 maps) 66-7-49, dragons r100 29.3 v 25.4.
