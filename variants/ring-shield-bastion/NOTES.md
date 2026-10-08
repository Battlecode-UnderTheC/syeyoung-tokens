# ring-shield-bastion (g53-7, arena ring-shield-3)

Base: ring-sealed-shield (arena ring-shield-1). Strategist A g53 #4 / B g53 #3 (walled bastion entry, trace first).

## Step 1: the holding term (traced)
- Arena 261181 (wg40004 "Hollow Bastion", seed 1592614637, ring-contest-4 as A v ring-sealed-2 as B): reproduced
  move for move by `build/tools/match ring-contest-m1 ring-sealed-wallin` (ring-contest-m1 sources == ring-contest-4);
  the base (ring-sealed-shield) plays the same opening. The bastion (x 10-19, rows 2-24) holds 146 of the map's
  204 fast beds and opens only by 2-tile doors at rows 13-14 on each side.
- Orange's first head in: rush explorer id4 at r16 through the west door (its path (6,8),(9,9)..(9,13)). Ours (decoded
  with -DWALLIN_DBG plus BASTION_DBG unreachable-food dumps): no head inside x 10-19 at any round to r60.
- Have we seen it? Only its east column through the kelp: heads in the x=21-22 corridor saw beds (19,15)-(19,21),
  id13 saw (19,2)-(19,8). Its inside, x 10-18, was never seen.
- What held them:
  1. ChooseGoal (every worker): with the door unseen, those beds have no known walk (paths.dist -1), so they are
     not candidates at all. The frontier that leads to them earns only the flat explore credit .25/(d+1)
     (.03-.04), against home-room beds at .2-2.5. E.g. r5 id3 at (22,20): best home bed (27,22) .246; the
     frontier (21,15) beside the door 6 steps away scores .044.
  2. The field walker (id5, our mirror of orange's id4): FieldFrontier saw the door frontier (20,13)/(20,14) at r4-6
     but its heading was the spawn facing (south), so the door was "forward" by 1-2 and the corridor south by 4-5.
     At r12-14 at (22,17)-(22,19) it had 2 live pearls and 6 soon-due beds in view (the bastion column).
     Arrival needs 4 live, so it walked on south and round the board to the enemy's side.
  The mirror match (base v base) confirms it: one side never enters, the other enters at r45.
- No parity or discount issue: the wall-in exemption never got a candidate to act on.

## Change (ChooseGoal only, strategy.cpp/.hpp; BASTION_MODE=0 is the parent)
Before kWallInUntil (r60), for a worker (not queen or champion): food seen behind kelp has no known walk
round kelp, bodies ignored (that kelp-only BFS keeps body-blocked rooms out). A frontier tile within kSeekReach (6)
of such food scores it as reachable food is scored, at the walk's lower bound (frontier steps + straight line on):
the best value plus cluster_bonus times the rest. The term only sits on frontier tiles, so it stops once the
ground is seen. It is skipped over kSeekMax (98) walled tiles, as a CPU bound.
- First try: best single value only. On 261181 it fired (.08-.13 at the door) but never won. Sum/cluster form kept.
- Also tried (not shipped): field walker targets a frontier near 4+ walled beds. It never fired on 261181 (for id5 the
  door was known, so the column was reachable, not walled).

## Local
- 261181 map/seed, the change v the parent:
  - our team as A (orange's spot) v ring-contest-m1: first head in r34, 24 heads inside at r60. Parent: r53, 4.
  - As A v the parent, mirror: r34, 34. Parent mirror: never.
  - As B (the arena seat): still no entry. The cluster call to our home rooms (call.cpp) outbids the door at r36-39
    (id14 picks (20,14) and the call overrides it), and home beds at .6-.8 beat it at r33-35.
- Fresh mapgen seed 53707, 44 maps x seeds 1,2 x both seats v ring-sealed-shield: 89-2-85. By family: anti-centre
  8-8, crowded 8-8, maze 7-2-7, mixed 8-8, off-axis 8-8, open 9-7, rich-near-home 9-7, ring 9-7, rooms 7-9,
  sealed-spawn 8-8, sparse 8-8. Opening r10-40 dragons/total match the parent's within 0.2 (the term fires rarely).
- check maps/official: all passed.

## Arena duels v ring-shield-1 (eph-8d30dc4c)
- server random:2: 31-37 (3+ starters 17-23, <=2 14-14; elim 9-15, queen 17-16)
- official random:2: 8-12
- P1 maps (wg40004, fixed-seam-meet, stripes, mg40504; server, custom and rotating; random:4): 17-15,
  dragons@100 22.2 v 19.5
All within noise. CPU max 43.5M (same as the parent).

## Arena verdict (ring-shield-3, 448 games)
- v ring-shield-1 (the parent): 61-0-51 (54.5%), BEATS the parent within noise.
  - By suite: server 37-31, custom 18-16, official 6-4. No rotating games in the draft suite, so wg40004 was not played.
  - Decided by: elimination 25-24, longer queen 23-10, longest dragon 11-16, total 2-1.
- v the others: ring-sealed-2 58-54, ring-field-3 60-52, ring-qforage-1 60-52. CPU p99 23.3M, max 50.4M.
- Eval P(win) v the parent: r100 .510, r200 .500, r300 .502, r450 .563.
- Paired with the parent's games (same opponent, map, seed and seat; 336 pairs): 41 L->W, 43 W->L, 142 identical.
  The flips spread over server maps.
  - around_unsw went 1-11 (parent 11-1) on its 2 fixed seeds. Reproduced locally (unsw.map) as a chaotic
    divergence: the term drives about 30 turns there and the queen dies at r128-135. On unsw seeds 1-8 x both seats
    v ring-sealed-wallin, the change and the parent are both 10-6, so it is not systematic.
- The targeted weak spot, in paired arena games (pearls eaten r60 / dragons r60, us v the parent on the same setups):
  - stripes: 21.1/4.9 v 16.7/3.8, wins 8 v 7.
  - trophy 51.3 v 51.6, devil 77.5 v 77.6, islands 88.4 v 88.7.
  - fixed-walled-centre, fixed-rooms, fixed-seam-meet, fixed-ring-doors: identical (the term never fires there:
    their walled food is seen with its door, or not at all).
- Decoded/traced, stripes 272460 (seed 9743940328453324018, v ring-sealed-2, seat A), reproduced locally:
  - The parent loses (elimination r211). The change wins (r168): 9 dragons, length 24 at r60, against the
    parent's 5 and 12.
  - Seek-driven goals: id2 walks at r8-14 to the frontier beside the walled beds at (10,4)/(7,3). id10 at r36-40
    walks toward the 2x2 kelp rooms at (11-14,9-11).

## Next
- On the arena seat of 261181 (B), the door still loses to the home cluster call (call.cpp) and to home beds. The
  first head through the door must be an explorer. The field walker's held heading (spawn facing) passed the door
  at r4-6 and saw 6 soon-due field beds at r12-14 (2 live; arrival needs 4 live). That is the other half of the
  trace, for its own change. The tried "walled field" rule for the walker never fired, because the walker knew
  the door.
