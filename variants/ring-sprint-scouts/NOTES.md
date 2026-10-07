# ring-sprint-scouts (g48-1)

- **Line:** ring-sprint. **Parent:** ring-sprint-21 (= variants/ring-sprint-guard2, byte-identical sources).
- **Brief:** ../syeyoung-tokens.league/briefs/g48-1.md (strategist-g48 #1, "mouths per field").

## Idea
arena-r22: P1 economy is the top loss cause (303). Our dragon count sits at 5-6 from r20 to r100
while the opponent's grows 7 → 34; per-dragon eating is half of theirs. Film 212730 (Gilded Shoals v
gst-search-12): all our heads scrum one field r10-r95 while a 12-bed room 6-10 tiles away stays full.
In the code the explorers stop at the first cluster (rush.cpp) and the cluster call ends at r40.

## Change (rush.cpp, call.cpp, strategy.cpp/.hpp)
- `UpdateScout` (rush.cpp): a non-queen worker in r30-249 (not founding, not in late concentration or
  the cap latch) counts its field: live or due-within-10-rounds beds within Manhattan 3 of its head,
  and our / enemy heads there (all inside the 7x7 view). With 3+ of our heads and more heads than beds
  for 3 rounds running, it re-enters the explorer role (rush_state_ 1, `scout_`) if fewer than
  `excess` of our heads there have a higher id (last arrived). excess = min(heads - beds,
  heads - enemy heads, max(1, workers/3)). Off when the whole board is seen.
- A scout walks to RushGoal's frontier (most unseen tiles per step, existing ally fan-out), skips food
  within 3 of the field it left, refuses frontiers within a seen enemy head's reach (ceil(L/4)+1),
  takes FreeSprint (length 5+) instead of RushSprint, and splits like any worker.
- A trip ends on a worthwhile or contested find away from the field left (kOriginR 5), a 6-round
  stall, 15 rounds, r260, or no frontier; then 15 rounds cooldown. A scout's worthwhile find is called
  over sonar (call.cpp) for 8 rounds, to r250; other calls still end at r40.

## Iterations (local + light duels vs ring-sprint-21, server suite, random:2 seeds, 68 games)
1. v1 (no scrum minimum, unlimited trip, every call to r250): duel 25-0-43. Bug: a lone worker on
   bare ground (1 head, 0 beds) counted as over capacity; 10 workers re-armed at r32 on one map.
2. v2 (+3-head minimum and a bed required, 15-round trip): duel 35-0-33. Fresh maps 92-2-82.
3. v3 (+late call only for scouts' finds; calling every surplus to r250 cost r100 dragons on server
   smoke, 24.4 v 26.3 against 25.7 v 25.3 with the call ending at r40): duel 31-0-37; fresh 87-2-87.
4. v4 = submitted (no bed required: on the film map 246 of 527 over-capacity worker turns before r120
   were 3+ heads on 0 beds): duel server 37-0-31 (3+ starters 22-18, <=2 15-13; elimination 13-13,
   queen 19-11, longest 5-7), official 9-0-11.

## Local (v4)
- `check ring-sprint-scouts maps/official`: passed.
- Server smoke seed 1 v ring-sprint-guard2: 18-0-16; r100 dragons 25.4 v 25.9.
- Fresh mapgen (seed 48117, 44 maps, seeds 1,2) v ring-sprint-guard2: 85-2-89. Per family:
  anti-centre 7-9, crowded 8-8, maze 7-9, mixed 9-7, off-axis 11-5, open 8-8, rich-near-home 6-10,
  ring 8-8, rooms 5-11, sealed-spawn 7-9, sparse 9-2-5 (family results swing by +-3 between
  near-identical builds, e.g. rooms 11-5 in v3).
- Scout moves: about 100 per match on server maps (v1 331; v3 33).
- Duel metrics (v4 server duel, 68 games, arena-r22-style measures; us v parent): eats per dragon per
  10 rounds r20-60 0.94 v 0.92; bed share r60 0.50 v 0.50; dragons r60 15.8 v 15.5, r100 24.1 v 24.3.
  Against the parent nothing in the economy moves: the targeted gap is against non-ring bots.

## Arena
- ring-sprint-33 (draft, parent ring-sprint-21): verdict pending at time of writing.
