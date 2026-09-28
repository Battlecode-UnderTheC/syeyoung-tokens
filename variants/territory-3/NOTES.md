# territory-3

## Idea

Territory archetype (a third of the workers tend a bed cluster until r380), with late-surge-17's
goal scoring and a stricter cluster definition so tenders pick real hot spots, and tenders
claiming their cluster over sonar so two don't farm the same one.

## What changed from the parents, and why

Base territory-2 (namespace `ter2` -> `ter3`, macros `TER2_*` -> `TER3_*`).

- From late-surge-17: a bed ripening in `due` rounds at `dist` steps is ready at
  `dist + 2*max(0, due-dist)` (param `wait_factor`); a candidate gains `field_bonus` (0.5) x the
  value of pearls/beds within `field_radius` 2, each weighted by (d+1)/(its ready+1). Tile values
  and dues are computed once per `ChooseGoal` (cheap filter on seen/pearl/bed before `TileValue`,
  so `Occupied` runs only on pearl/bed tiles); the main loop reuses them instead of calling
  `TileValue` again. ls17's `cluster_bonus/cluster_radius` renamed `field_*` (territory-2 already
  used those names).
- Clusters: `cluster_radius` 3 -> 2, `cluster_min_beds` 3 -> 4 (>= 4 beds within 2 steps).
- Claims: a tender with a home pings a claim word (magic 0xA8 | team | id | x | y | round) on E and
  W before r330 (champion word stays on N and S, as KNOWLEDGE says early pings are free). Receivers
  keep the newest claim per id; `ChooseHome` skips any bed within 3 of a claim from a lower id heard
  in the last 30 rounds. Not relayed.

## Measured (sim, both seats)

| opponent | official (22) | worldgen s2612 (40) | worldgen s2613 (40) | total |
|---|---|---|---|---|
| territory-2 | 11-0-11, longest -3.4, total -3.9 | 20-0-20, -5.2, -12.5 | 19-0-21, -5.3, -2.5 | 50-0-52 |
| late-surge-17 | 9-0-13, -9.4, +10.3 | 21-1-18, +2.1, +10.3 | 21-1-18, +2.9, +8.2 | 51-2-49 |
| territory-2 (ablation: old 3-within-3 clusters, rest as above) | 11-0-11, -1.3, -6.7 | 17-0-23, -4.0, -2.1 | 18-0-22, -2.3, +7.8 | 46-0-56 |

territory-2's own self-play noise band was +-6 games (45-57 vs itself), so everything here is
even. `check`: all checks passed. Slowest turn is not meaningful in this run: the machine had
load ~120, and in the same run territory-2 showed 86 ms on big_empty and territory-3 up to 100 ms
on several maps, while unloaded values were 0.1-0.6 ms for both.

## What should improve

Tenders farm denser spots (farm-cluster turns fell 1031 -> 500 per 22 official matches, outside
turns rose 603 -> 892: fewer beds qualify, so homes are rarer and further). Expect even W-D-L.

## Tried, didn't work / next

The stricter cluster definition alone is noise-level (+4 games). Next: beds weighted by their
gap range (fast-ripening beds), relaying claims, and fall back to the 3-within-3 definition when
no 4-within-2 cluster is known (tenders currently act as plain workers then).
