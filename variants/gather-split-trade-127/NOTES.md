# gather-split-trade-127

## Idea

strategist-m P1 (map-structure region routing in the opening). A dragon sees only its 7x7 window,
so "scan the whole board at spawn" is not possible; the part of the map structure every dragon
does know at spawn is its own position and the board size. The map centre (W/2, H/2) is the fixed
point of every symmetry (x, y, xy), so it is equally far from both teams' spawns: the contested
ground. Each dragon takes it as its region anchor. Before `route_until` (60), and until its head
first comes within `route_arrive` (4) tiles of the anchor, a frontier tile's explore credit gains
up to `route_bonus` (0.4) x itself in proportion to (how much nearer the anchor it is than our
head) / (path steps to reach it). Frontiers leading out of the spawn corridor towards the middle
beat sideways/backward ones; walls are respected because the scan only covers reachable tiles with
their path distance. Pearl/bed scores are untouched (centre-claim-1 lost by overriding
pearl-chasing), and it applies to all map sizes (the parent's centre_pull is off above 1024 tiles,
i.e. on Stronghold/Trauma/Schooltime).

## What changed from the parents, and why

Parent gather-split-trade-115; namespace/macros renamed gst127. New params route_until,
route_bonus, route_arrive; `arrived_` flag set in Remember(); the route multiplier in
ChooseGoal's explore credit.

## Measurements (sim, both seats; worldgen seed 107000 x20 unless noted)

First try, route_bonus 1.0 until r100: vs gst115 official 9-0-13, worldgen 19-0-21; vs ts8
official 10-1-11 (parent 16-1-5). Too strong: rejected.

Committed, route_bonus 0.4 until r60:
- vs gst115 (parent): official 10-0-12 (Default 1-1, Devil 2-0, Stronghold 1-1, Trauma 0-2),
  worldgen 26-0-14, worldgen seed 207000 23-0-17. Total 59-43.
- vs gst96 (live v7): official 10-0-12 (Default 1-1, Devil 2-0, Stronghold 0-2, Trauma 1-1)
  [parent 13-0-9], worldgen 23-0-17 [parent 24-0-16].
- vs ts8: official 15-1-6 (Default 2-0, Devil 0-2, Stronghold 2-0, Trauma 1-1) [parent 16-1-5],
  worldgen 32-0-8 [parent 32-0-8].
- r50/r100 vs parent, official: 11.4 dragons/27.4 total v 10.6/26.4; r100 20.0/50.6 v 18.7/46.6.
  Worldgen: even (r50 67.5 v 68.0 total, r100 109.0 v 110.8).
- on-its-own-move deaths per match (official): head-to-head 26.3 v ts8 31.8, v gst96 31.9; no
  walls/self.

## What should improve

Slightly faster opening spread off the spawn edge; Devil still loses to ts8 in sim (0-2), so the
positioning gap strategist-m described is not closed by a frontier bias alone.

## What didn't work

route_bonus 1.0 to r100 (see above): pulling every dragon to the one centre point crowds it and
loses head-to-head to the parent and to ts8.
