# ring-clustercall-1 (g42-1, arena ring-sched-27)

## Idea
g41: on centre-cluster maps (trophy, devil) ring-sched-8 grew 5.8 -> 12 dragons in r20-40 by
piling onto rich bed clusters before any contact; ring-sched-17's frontier explorers find such
clusters but only the finder farms them. A dragon that FINDS a rich cluster should call the team.

## What changed from ring-sched-17 (variants/ring-sched-m3), and why
One change, `call.cpp` plus hooks in `strategy.cpp`/`strategy.hpp`:
- **Find (r < 40):** any dragon scores the tiles in its 7x7 view by ClusterScore (live or
  soon-due beds within 2). A find needs >= 3 beds and >= 2 more beds than our heads within 4
  (2 spare pearls bring one 2-long worker to its growth split).
- **Word:** 0xA9 | team | x | y | surplus | round found, on two opposite directions (alternating;
  the finder uses all four). Every dragon that knows the queen's ring relays the ring word (0xBA)
  on all four directions, which overwrote every other word after ~r2. So the call also rides in
  the ring word's spare bits 26-52 (the ring readers use bits 0-25). Without that, almost nobody
  heard a call.
- **Answer:** a worker (not the queen, not a frontier explorer) further than 2 from the cluster,
  which hasn't seen it since the call, values it as ChooseGoal values a known bed in a field
  ((1 + 0.5 * (surplus - 1)) / (eta + 1), where eta = known path + torus remainder). It walks to
  the known tile nearest the cluster when that beats its chosen goal's score.
- Observation only: no board-size gates, no fixed points, no map keys.

## Numbers
- Local smoke, vs ring-sched-m3 on maps/server, seeds 1 and 2: 39-29 (22-12, 17-17). Sending
  without answering is neutral (33-35, identical opening).
- Fresh mapgen (seed 42142, 44 maps, 11 families, seeds 1 and 2) vs ring-sched-m3: 95-81.
  - anti-centre 10-6, crowded 7-9, maze 8-8, mixed 8-8, off-axis 9-7, open 8-8;
  - rich-near-home 11-5, ring 6-10, rooms 10-6, sealed-spawn 7-9, sparse 11-5.
- Arena duels vs ring-sched-17 (eval = mean P(win) at r100/200/300):

  | duel | W-L | eval |
  |---|---|---|
  | server random:2 | 38-30 | .51/.52/.52 |
  | official random:2 | 12-8 | .55/.61/.65 |
  | trophy+devil random:6 | 13-11 | .55/.57/.60 |

  On trophy and devil, dragons at r100 were 25.0 v 22.4. Dragons at r40 were LOWER (8.7 v 10.0):
  answering costs early travel, and the gain comes later (board control, r100+).
- Rejected iterations:
  - surplus >= 4 and idle-only answering: server 37-31, eval .48/.47/.45;
  - the call valued as one bed: server 35-33, trophy/devil eval about .50 (the gain was gone).
- **Arena verdict (ring-sched-27):** 59-53 vs ring-sched-17, "beats parent" within noise.
  - By suite: server (fixed seeds) 33-35, custom 19-15, official 7-3.
  - vs ring-rush-1 73-39; vs cs7 67-45; vs ring-plus-21 71-41.
  - Eval vs parent .52/.53/.49.
  - CPU p99 27.4M, max 38.4M.
  - vs parent, trophy 3-1 and devil 1-3 (4 games each).

## What should improve / next
- Dragons at r100 and board control on cluster maps. The early count (r40) does not rise:
  answerers travel instead of eating.
- Next: let the answerer keep eating on the way (score the call as a detour bonus on food goals
  toward it, not a replacement goal). Or cap the answerers per call by the surplus, e.g. with the
  sched matching over relayed head positions.
