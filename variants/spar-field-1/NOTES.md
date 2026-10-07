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
