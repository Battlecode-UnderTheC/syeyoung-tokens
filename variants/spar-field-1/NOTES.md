# spar-field-1 (g46-6): a field-style SPARRING bot for the arena pool

- **Line:** spar-field (arena `spar-field`), parent ring-sprint-21 (= variants/ring-sprint-guard2).
- **Purpose:** strategist-g46-A S3. Every arena opponent is a home-bound relative of ours, so opening
  changes can't be judged against a team that rushes the bed fields the way the top server teams do
  (docs/analysis-2026-10-07/STYLE.md: aAah atomises to L2 by r6, one head per column by r12-18, its
  2-4-long queen inside the swarm). This bot plays that opening; W-L against the parent is not the goal.

## The one change: spar.cpp (hooked at the top of StrategyAgent::Decide, before QueenMove)
Until a dragon "arrives" or r40 it owns the turn; everything after is ring-sprint-21 unchanged.
1. Atomise: length 4+ splits off a 2-long child (SplitSafe(2)).
2. Committed heading, one per lane: first-turn heading away from the ally heads it knows (a split child
   takes its parent's way turned 45 degrees by id parity instead of doubling back). Ally heads within 4
   push it sideways only. It walks to the reachable frontier (known tile beside a never-seen edge) or
   seen bed that lies most along the heading per step; frontier nearer another ally head is that
   ally's lane. Known portals are crossed by the path search. The heading slowly follows the way
   actually taken (room doors, corridors). No fixed board points.
3. Food on the way: a pearl within 2 steps (the queen: 3).
4. Arrival: a cluster in view (3+ live or soon-due beds within 2 of a tile within 3 of the head) with no
   more of our heads around it than beds: the base farms it (splits at 4, so children carpet it).
5. Queen on a 3+-dragon start: splits to 2 and walks to the middle of the ally heads she knows; on a 1-2
   dragon start she keeps the base's forage (fresh off-axis 1-2-starter maps went 2-14 otherwise).
6. An enemy head whose lunge (ceil(Le/4)+Le-2 steps; a body leaving the view counts as 6 long) could
   reach our next step hands that turn to the base; the queen keeps 2 more (fresh sparse maps went
   3-13 before: L2 travellers and the L2 queen walked into lunge reach).
7. (iteration 2, after the first arena games) a dragon that sees no new tile for 6 rounds hands over
   to the base (a child circled the dead-end corner of the Devil spawn room r8-r30, chasing frontier
   behind kelp); arrival needs no other ally head within 3 of the field (one dragon per field); the
   queen aims at the distance-weighted middle of the heads she knows (the unweighted middle kept her
   circling the spawn room).
Debug: `-DSPAR_DBG` prints per-turn state for r<45 (plugin.cpp).

## Local (sim v ring-sprint-guard2)
- server maps seed 1: 17-0-17. Fresh mapgen (seed 46061, 44 maps, seeds 1-2): 96-1-79; per family
  anti-centre 8-8, crowded 12-4, maze 14-2, mixed 9-7, off-axis 10-6, open 8-8, rich-near-home 8-8,
  ring 4-12 (re-run seeds 3-4 8-8, second ring set 12-12), rooms 7-9, sealed-spawn 7-9, sparse 9-1-6.
- `check spar-field-1 maps/official`: passed.

## Arena duels v ring-sprint-21 (eph-9b407de1, server suite random:2; official random:2)
- server 30-0-38 (44%, noise): elimination 13-11, longer queen 11-21, longest 6-6; 3+ starters 16-24, <=2 14-14.
- official 12-0-8 (60%, noise).
- Opening from replays (r40, us/them, server duel): eats 26.9/27.9, board control at r30 0.52/0.48;
  Devil eats 43.8/25.8 at r40 (bc 0.57), Islands 56/39.8 (bc 0.61), Trophy 22/21.3 (bc 0.57).
  Behind on rich home maps (around_unsw 51/102, Australia 6.5/16.8): travellers leave food for frontier.

## Arena verdict, first submission (spar-field-1, iteration 1)
- vs ring-sprint-21 60-0-52 (53.6%, beats_parent, noise); server suite 36-32; vs ring-qforage-1 59-53,
  ring-contest-4 62-50, ring-sched-17 62-50. CPU worst p99 25.8M, max 43.4M.
- Own games v parent (arena metrics): board control r30 0.487 v 0.455, eats r40 31.3 v 30.2; server
  25.4 v 27.0, custom 45.3 v 38.1. Eval P(win) r100/200/300 0.535/0.516/0.508.
- Devil (fixed server seeds) lost the opening (eats r40 16 v 26): our dragons settled on the first bed
  columns by the spawn while the parent reached the centre; one child stalled in the spawn room corner.

## Iteration 2 (local and duel)
- Duel v ring-sprint-21 server random:2: 36-0-32; r30 board control 0.52/0.48; r40 eats 28.0/28.6;
  Devil eats r40 35.8 v 17.8 (bc 0.56), Islands 55.3 v 39.8 (bc 0.58), Trophy 18.8 v 13.
- Fresh mapgen 46061 seeds 1-2: 93-1-82; families 10-6, 10-6, 10-6, 8-8, 11-5, 7-9, rich-near-home 5-11
  (re-run seeds 3-4 7-9, second set 11-13), ring 9-7, rooms 9-7, sealed 7-9, sparse 7-1-8.

## Arena verdict, iteration 2 (spar-field-2)
- vs ring-sprint-21 67-0-45 (59.8%, CI 50.6-68.4, beats_parent); server 42-26, official 7-3, custom 18-16;
  elimination 32-16, longer queen 19-17, longest 14-12. vs ring-qforage-1 57-55, ring-contest-4 69-43,
  ring-sched-17 65-47. CPU worst p99 25.8M, max 44.8M.
- Own games v parent: board control r30 0.480 v 0.463 (custom 0.50 v 0.44); eats r40 31.4 v 30.3
  (server 25.2 v 27.3, custom 45.7 v 37.6). Eval P(win) r100/200/300 0.537/0.560/0.588.
- Films: Islands 219644 — heads fanned over x 8-43 by r12 and eat 52 v 40 at r40 (bc 0.53 v 0.38);
  Trophy 219683 (2-starter, queen keeps base forage) — queen at (19,11) in the cup r12-r30 with 4-6
  of ours around her; Devil 219336 (fixed seed) — our 6 L2s settle on the bed columns by the spawn
  (x 5-11) while the parent reaches x 21-26 by r30: on these seeds the opening is lost (eats 20 v 25).
- Next: on Devil-like maps, arrival should prefer fields away from the spawn (strategist S1's "away
  from spawn"), so the carpet reaches the central fields first.
