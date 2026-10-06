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

## Arena
Not submitted (blocked). Never tested in the arena.
