# ring-shield-heading (g53-12)

Base: ring-shield-bastion (arena ring-shield-3). Trace-first, from g53-9 (stripes: our first room eat r18-37 v theirs r8-12).

## Trace (step 1)
Reproduced the head's four stripes losses to ring-contest-4 move for move (275669/275873 seed 9743940328453324018,
275650/275663 seed 8148504378389391807; build/tools/match ring-shield-bastion v ring-contest-m1) with -DFIELD_DBG
(new FF/FG/FAN lines: frontier candidates with d/fwd/gain/score, goal and every known pearl with its walk length).
- Stripes: four 3-row bands split by kelp. Our worker spawns 3 rows below the queen, both facing the same way
  (A: (21,4) and queen (21,1), west). Our near bed room (A: (16-17,9-10), F/f beds) lies two bands further from the
  queen, entered only by a portal at (16|17,8); the room the walker reaches is the enemy's near one.
- Held heading = spawn facing (field.cpp). r0 candidates (A): (17,4) along the band d4 fwd4 score 1.26; band 6-8
  frontier (17,7) d7 .88; the portal (20,6) d3 .50. It walks its band; from rows 3-4 the near room (rows 9-10)
  is never in view (first seen r17 by another dragon, d-1 until r24).
- The walk ends early on a false arrival: FieldBedsInView counted the 4 pearls of a kelp-sealed 2x2 room
  ((15-16,4-5), all d-1, never enterable) as a field: "arrived" r5-8 in all four games. The ordinary agent then
  walked on along the band to the enemy's room ((6-7,1-2), eaten by them r12-14), first worker eat r24/r25/r30/r36.
  Seed 814 as A: then walked back the whole board, first eat r36, 1 eat by r30.
- Also seen (not changed): at a room door our worker dithers ("detour (goal step unsafe)"), e.g. 275873 r17-r30 at
  (16,8) (g53-9's room-entry finding).
- The queen walks her lane in this opening (her r0 forage rule: shorter than 4, no 4+ ally), so the two lanes
  3 rows apart are swept twice.

## Change (field.cpp; one behaviour: how the field walker picks its heading and decides it has arrived)
1. FieldFanOut: while our queen roams (her own r0 rule: queen and we shorter than 4, no 4+ ally in view, under
   kForageSettleUnits units) and her head is in view on a lane under 2*kView+1 from ours (the two 7x7 windows
   overlap), the held heading becomes facing + unit step away from her across it (45 degrees). FIELD_FAN=0 is off.
   Iteration 1 leaned on every map: devil (queen 4 long) walkers left the central field row, devil 0-3 locally,
   arena duel 30-38 (3+ starters 15-25). Gating on the queen's roam rule left it firing on stripes and
   queen_of_spades only among server maps (2 of 44 fresh maps).
2. FieldReach: arrival (and the r0 spawn-field/home-keeper counts) count only beds we may yet walk to (BFS through
   every edge not known kelp; known portals followed). FIELD_REACH=0 is off.
3. plugin.cpp: rollout/lookahead/lungeguard labels no longer overwritten by ProtectQueenAction's reason unless it
   vetoes (debug labels only, no behaviour change).

## Local
- Stripes (traced seeds, as A / as B v ring-contest-m1): first worker eat r17 all four (parent r25-37), eats by r30
  7-10 v 1-5. Walker (A) now goes (21,5),(20,5), through the unknown portal at (20,6) to (2,5) ("arrived gated" r6),
  back through it at r7 and eats the queen-side room (20-21,10-11) r16-18.
- Stripes 8 seeds x 2 seats: v parent 13-3 (dragons r20 3.2 v 2.2, total r40 8.8 v 6.4); v ring-contest-m1 13-3
  (parent 7-9).
- queen_of_spades 8 seeds: 8-8 (opening a little slower: dragons r20 2.1 v 2.8).
- Fresh mapgen seed 531201 (44 maps x seeds 1,2 x both seats, final version) v parent per family: anti-centre 8-8,
  crowded 8-8, maze 8-8, mixed 8-8, off-axis 8-8, open 8-8, rich-near-home 8-8, ring 9-7, rooms 8-8,
  sealed-spawn 8-8, sparse 10-6.
- check maps/official: all passed.

## Arena duels v ring-shield-3
- v1 (ungated fan, eph-d404c83f): server 30-38 (devil 1-3, around_unsw 0-4, portals 0-4, stripes 4-0), official 10-10.
- v2 (submitted, eph-8f2db57c): server 33-35 (stripes 3-1, queen_of_spades 1-3, portals 1-3, the rest 2-2),
  3+ starters 19-21, <=2 14-14; official 10-10. CPU max 35.8M (parent 35.8M).
