# ring-sprint-picket (g44-6; parent ring-sprint-1 = arena ring-sprint-1)

## Idea (strategist g44 #2)
arena-r18 A losses on 3+-starter maps: in r0-30 we eat home 11.6 / mid 6.4 / their side 1.6,
they eat home 3.0 / mid 11.7 / OUR side 12.4; dragons at r30 6.2 v 9.4. They take the contested
ground first. g43-2/g43-3 re-weighted food and home food still outbid the frontier, so this is a
ROLE, not a food weight.

## Change (one behaviour)
- Every 3rd child (spawn id % 3 == 0) born before r60 (kContestUntil) is a PICKET until r60; the role sticks.
- `PicketGoal` (rush.cpp): goal = the contested band, tiles where our arrival is within +-2
  (kPicketBand) of the rival's. Rivals: enemy heads seen in the last 20 rounds (RivalDist);
  until any are seen, the mirror images of our heads under every symmetry candidate still alive
  (our own counterpart via paths.dist of the mirrored tile).
- Eats food only inside the band, or home food within 3 steps (kPicketHome); never their side.
  No band food: walks to the band tile with the most beds / unseen ground per step (kept on
  near-ties). No fixed point, no centre target.
- Labels: `picket: band food`, `picket: home food`, `picket: to band`, `picket: sprint`.
  Debug: `-DPICKET_DBG` prints BORN / PICKET / PICKNOGOAL lines.

## Local
- check maps/official: all passed (slowest turn 8.4 ms).
- sim vs ring-sprint-1, maps/server seed 1: 13-0-21 (elim 4-9, queen 8-6, longest 1-6). r10-r40
  dragons 6.6/7.5/9.4/11.8 v 6.8/7.9/9.8/11.7 -- no opening gain. Picket labels ~97 actions per
  match (25 band food, 19 home food, 53 to band): few dragons get the role, because children
  before r60 are few and ids are shared by both teams (on default.map seed 1 the first picket
  was born at r39).
- Not run: arena ephemeral/duel/submit and fresh mapgen (blocked by the permission system).

## v2 (this run): role assignment reaches the opening, band estimate fixed
- Assignment (strategy.cpp Remember): the team's k-th dragon (1-based) is a picket when k % 3 == 0,
  counting starters (k = id/2 + 1: the map's dragons alternate teams) and children (k = first
  UNIT_COUNT). v1's id % 3 over children only gave ~2 pickets/game, the first at r39; now 2 starter
  pickets per 6-8-starter team act from r1 (default.map seed 1 debug trace).
- Band fix (rush.cpp PicketGoal, found in arena replays): rival arrival is a search over the known
  board (kelp blocks, unseen edges open) from seen enemy heads, or from the mirror of our head per
  live symmetry candidate; not the torus distance. Allies' mirrored counterparts dropped; a
  candidate that maps our r0 head onto an in-view tile with no enemy is dropped for pickets.
  Cause: devil, duel-69f43886 g28/g29 -- the enemy across the walled wrap edge was 2-4 torus steps
  from the tiles behind our home; the picket (id 5) sat there at length 2 for 60 rounds, eating 0.

## Local
- check maps/official: all passed (both versions).
- maps/server seed 1 vs ring-sprint-1: v1b (assignment only) 17-0-17.
- Fresh mapgen (seed 4406, 44 maps, seeds 1,2): v1b 93-0-83; v2 77-0-99. v2 per family (16 each):
  anti-centre 7-9, crowded 11-5, maze 8-8, mixed 7-9, off-axis 7-9, open 8-8, **rich-near-home 3-13**,
  ring 7-9, rooms 6-10, sealed-spawn 7-9, sparse 6-10.

## Arena duels vs ring-sprint-1 (random:2)
- v1b eph-ebd0e439: server 32-0-36 (duel-69f43886; 3+ 21-19, <=2 11-17; elim 7-19, queen 17-11),
  official 11-0-9 (duel-6cd35575). eval P(win) r100/200/300: 0.445/0.425/0.427 (88 games).
- v2 eph-afaba2a9: server 29-0-39 (duel-43ee16f2; 3+ 16-24, <=2 13-15), official 11-0-9
  (duel-6126160e). eval 0.441/0.455/0.445. dragons@100 23.0 v 26.4 (server). CPU max 46.6M.
- Target metric (replay decode; tile class by starter BFS: own/mid(+-2)/far), 3+-starter games, us v parent:
  v2 r0-30 mid 2.9 v 4.6, far 1.1 v 1.5; r30-60 mid 6.7 v 8.9, far 3.3 v 5.8; dragons r30 13.6 v 14.7.
  (v1b: r0-30 mid 2.7 v 4.0, far 1.1 v 1.4.) The starter pickets vs their mirror twins (same id^1,
  parent side), r0-59 over 44 games: mid 11 v 44, far 10 v 32, own 134 v 146 eats.
- Reading: the role fires now, and it does what it says (walks to band tiles, eats band food), but
  the parent's unconstrained racer/explorer (contest race, explorer food) takes MORE contested and
  far food than the picket confined to +-2 and barred from their side. The picket idles walking to
  band tiles (~2/3 of its turns have no food goal) and skips home food >3 away (rich-near-home 3-13).

## Arena verdict
Not submitted: the targeted weak spot got worse (fewer mid/far eats, fewer dragons at r30), a clear
fresh-mapgen family loss (rich-near-home 3-13), and eval < 0.5 in both versions.
