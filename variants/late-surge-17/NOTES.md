# late-surge-17

## Idea

Faster pearl intake in the opening by fixing goal scoring: waiting on a bed is charged more
than walking, and a pearl or bed inside a field is worth more than a lone one.

## What changed from the parents, and why

late-surge-16 (namespace renamed ls16 -> ls17), `ChooseGoal`/`TileValue` only:

- (a) kept: a bed ripening in `due` rounds at `dist` steps is ready at
  `dist + wait_factor*max(0, due-dist)` (wait_factor 2) instead of `max(dist, due)`; a candidate
  pearl/bed gains `cluster_bonus` (0.5) x the value of other pearls/beds within Manhattan 2,
  each weighted by (d+1)/(its ready+1) so an unripe bed adds little. Tile values are computed once
  per `ChooseGoal` (calling `TileValue` per neighbour made turns slow: `Occupied`/`Mine` is not cheap).
- (b) written but OFF (params `frontier_radius = 0`, `open_centre_value = 0`): explore edge weighted
  by never-seen share within radius 3 (floor 0.3), and a centre pull on every map until r60.

## Measured (sim, both seats)

| config | opponent | official (22) | worldgen s1301 (40) | worldgen s1302 (40) |
|---|---|---|---|---|
| (a)+(b) | ls16 | 12-0-10, +29 total | 18-0-22, -8 | 20-0-20, +10 |
| (a)+(b) | ts4 | 14-1-7 | 29-0-11, +67 | 30-0-10, +69 |
| (a) only | ls16 | **14-0-8, +37** | **24-0-16, +65** | **24-0-16, +63** |
| (a) only | ts4 | 13-0-9, +0 | 29-0-11, +89 | 29-0-11, +85 |
| (b) only | ls16 | - | 20-0-20, -15 | 21-0-19, +5 |
| (b) only | ts4 | - | 29-0-11, +79 | 29-0-11, +74 |
| parent ls16 | ts4 | 10-1-11, +2.5 | 24-0-16, +54 | 25-0-15, +54 |

(a)+centre pull (no frontier) on official: 9-0-13 vs ls16. The centre pull hurts.
Pearls-eaten-by-r50/100/200 instrumentation was not done (time budget).

## What should improve

Mass intake from pearl fields / bed clusters; fewer turns circling a lone bed. Beats ts4 more
often than the parent (58-22 vs 49-31 on the 80 worldgen games).

## Tried, didn't work

(b) frontier-weighted explore + opening centre pull: neutral to negative vs the parent.
