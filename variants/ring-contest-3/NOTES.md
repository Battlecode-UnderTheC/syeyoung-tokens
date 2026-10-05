# ring-contest-3 (g43-3): no opening pull back to home ground

Base: ring-contest-m1 (arena ring-contest-1, now ring-contest-4: same code). Line ring-contest.
The base's own notes are in ring-contest-m1/NOTES.md.

## Evidence
arena-r17: on crowded, many-starter maps (mg39509, wg39013, wg39000, wg39001) contest-1 lost
net 40 against sched-17. It ate fewer pearls in the middle in r0-59 (11.5 v 16.0). The workers
bunched at home (167601, 167062, 167425). arena-r17/mid: on 5-starter mid-size maps the dragon
gap was -2.3 at r40 and -6.6 at r60.

## Change (one idea: home ground gets no extra opening pull)
- **AtHome(t)** (rush.cpp) is home ground: the tiles the starters see at spawn.
  - A starter stores it at r0 (`home_ground_`): its own 7x7 view plus the 7x7 view of every ally
    head it sees then.
  - A split child has no spawn view, so it uses the ground within 2*kView of the queen's last
    report (her view plus a starter's view beside her).
  - The explorers' `home_view_`, used for their rush exit, is unchanged.
- **The contest x2 boost** (ChooseGoal) now applies only to contested food that is not AtHome.
  At home the boost pulled workers home whenever a rival head appeared, while the open middle,
  which the rival reaches about as soon as we do, kept x1.
- **g42-12 explorer rule:** an explorer skips uncontested food beyond kExplorerNear only when the
  food is AtHome. Seen food away from home counts at any distance within kSchedHorizon.
- **Cluster calls** (call.cpp, CallAnswered): no worker answers a call to a cluster that is
  AtHome. Iteration 2 added this after the mid evidence. In 167888 (wg39001, seed 0x5eed5eed),
  worker 8 turned back to a home call at r1-3 and dithered between the call and home beds until
  r14. Replayed locally with calls off, it took the far field through the portal by r10, as
  sched-17 does in 151736.
- New thresholds:
  - 2*kView for children: a starter's view beside the queen's view.
  - No new constants otherwise.
  - No map keys, no board-size gates, no fixed points.

## Iteration 1 (boost and explorer only, home = own spawn view)
- server random:2 34-34 (duel-931a9f94); official 10-10 (duel-564c4957).
- Mid-size maps 18-14 (duel-7d872f01).
- Fresh mapgen (seed 430306) 84-8-84.
- Behaviour: identical to the base on the regression seeds. Dispersion at r30, mean/farthest:
  6.22/12.9 v 6.19/12.9. The explorers leave rush within 1-5 rounds, so the g42-12 rule rarely
  fires. Not enough, so iteration 2 followed.

## Iteration 2 (this version, eph-24e81ab5)
- **Duels v ring-contest-1:**
  - server random:2, duel-0fc295b3: **31-37**. Elim 10-13, queen 12-16, longest 9-8.
    3+ starters 18-22, <=2 starters 13-15. dragons@100 25.5 v 23.7, queen alive 51.5% v 58.8%.
  - official random:2, duel-45776b10: **10-10**. 3+ starters 3-1, <=2 starters 7-9.
    board_control@200 .53 v .46.
  - 5-starter and crowded maps (wg39000/1/4/5/11/13/15, mg39509, fixed-maze-wide, fixed-rooms),
    duel-a450f0ff: **19-21**. Rotating 14-18, custom 5-3.
  - All within noise.
- **Dispersion** (duel-a450f0ff replays, us v base):
  - r30 mean head distance from spawn 6.24 v 5.69; farthest 12.6 v 11.4.
  - Dragons r60 24.7 v 22.0; dragons@100 36.3 v 33.8.
  - Eats r0-59 own/mid/far: 54.6/23.0/11.6 v base 53.0/18.3/8.4 (mirrored).
  - For comparison, sched-17 v contest-1 on the same maps (duel-01952dca): r60 dragons 27.2 v
    20.8, mid/far eats 13.9/12.5 v 8.2/5.1.
- **Regression seeds** (seed 1592614637, same opponent and seat):
  - 167601 (wg39001 v queen-ring14): eats r0-59 own/mid/far 57/0/0 and heads r40 16/0/0 in
    contest-1. v2 (duel-66c5c23c g0): 92/10/37 and 20/1/5, and it wins 52-30 on longest.
    sched-17 151933: 93/13/22.
  - 167888 (wg39001 v queen-3): 76/6/2 in contest-1; v2 (duel-d9dfe5fb g0): 76/17/36, wins 48-34.
    Worker 8 takes the portal field at r10 (22,24), as in sched-17 151736. Eats r0-49: 54 →
    82 (sched-17 124).
  - 168140 (wg39004 v crossover-1): v2 duel-4193279b g0 wins on queen (contest-1 lost).
  - mg39509 (167405/167425): still lost. These are 60-70 round eliminations by the
    gst-giant/queen-ring14 swarm.
  - 167036 (default_small): unchanged. The "seen strip" is behind kelp edges: BFS 14 steps from
    (6,6) to (7,7), 11 from (6,3) to (9,5). It isn't reachable food the rule withheld.
- **Fresh mapgen** (seed 430307, 44 maps, sim seeds 1,2, v ring-contest-m1): **77-2-97**.
  - Per family: anti-centre 6-10, crowded 8-8, maze 8-8, mixed 8-8, off-axis 8-2-6, open 6-10,
    rich-near-home 6-10, ring 8-8, rooms 5-11, sealed-spawn 6-10, sparse 8-8.
  - The same maps without the call gate gave 82-2-92.
  - Lean negative; each family is within noise at 16 games.
- **CPU:** max 36.6M (server duel), 38.1M (official).

## Arena
(filled in after the verdict)
