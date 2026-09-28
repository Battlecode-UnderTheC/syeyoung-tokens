# counter-swarm-25

## Idea

Crossover: port late-surge-17's bed/cluster goal-scoring fix into counter-swarm-24. A bed is
charged for waiting, not just distance, and a pearl/bed in a field of others is worth more
than a lone one, so the gatherer spends less time circling a slow bed and more time on dense
clusters. Keeps counter-swarm-24's allies/enemies contest logic and known-portal crossing
unchanged.

## What changed from the parents, and why

counter-swarm-24 (namespace `cs24` -> `cs25`, `CS24_*` -> `CS25_*`, files renamed
`cs24.cpp`/`.hpp` -> `cs25.cpp`/`.hpp`), `TileValue`/`ChooseGoal` only, mirroring late-surge-17's
part (a) (its part (b), frontier-weighted explore + opening centre pull, was left out per the
brief: ls17's own NOTES.md found it neutral-to-harmful):

- A bed ripening in `due` rounds at `dist` steps is now ready at
  `dist + wait_factor*max(0, due-dist)` (`wait_factor = 2.0`) instead of `max(dist, due)`, so a
  round spent waiting costs twice a round spent walking.
- A candidate pearl/bed gains `cluster_bonus` (0.5) x the value of other pearls/beds within
  Manhattan `cluster_radius` (2), each weighted by `(d+1)/(r2+1)` where `r2` uses the same
  wait-charged readiness — a bed far from ripe adds little to its neighbours' value.
- Tile base values and rounds-to-ripe are precomputed once per `ChooseGoal` call (`base[]`/
  `due0[]` arrays over the board) instead of calling `TileValue` per neighbour of every tile,
  since `Occupied`/`Mine` are not cheap.
- New params `wait_factor`, `cluster_bonus`, `cluster_radius` (defaults matching ls17), wired
  into the `CS25_P=`/`CS25_TUNE` override list.
- Everything else in `ChooseGoal` (hunting, ally-share discount, enemy-contest discount,
  explore-edge scoring, known-portal crossing) is untouched; the cluster bonus is added to
  `value` before it is divided by `(ready+1)` and before the share/contest discounts, so those
  discounts still apply to the boosted score.

This is the same port already done for body-wall-12 (35-27 vs its parent) and per the brief's
own instructions (ls17 went 62-40 vs its parent, 71-31 vs trading-swarm-4).

## What I measured

`build/tools/check counter-swarm-25 maps/official`: all checks passed, 0 avoidable deaths.
Slowest turn 1.1 ms (`big_empty`); counter-swarm-24's own check on the same maps in the same
run showed 35.6 ms on `big_empty` and 15.4 ms on `default` (machine under load, noisy), so no
systematic slowdown from the precomputed cluster bonus.

`sim`, both seats, 2 threads:

| opponent | maps | W-D-L | margin |
|---|---|---|---|
| counter-swarm-24 | official (22) | 15-0-7 | +39.4 total, +6.9 longest |
| counter-swarm-24 | worldgen, 20 seeds both seats (40) | 27-0-13 | +73.2 total, +7.3 longest |
| late-surge-17 | official (22) | 12-0-10 | +19.8 total, +3.5 longest |

Only had budget for one worldgen batch (vs the parent); did not repeat it against late-surge-17,
but the official-map result against ls17 alone is a clear win on both W-D-L and margin, matching
ls17's own report that this change beats the sibling lineage it was tested against (ts4).

## Tried, didn't work

Nothing else attempted this task — budget was for the one focused port. ls17's part (b)
(frontier explore weighting + opening centre pull) was deliberately left out per the brief and
ls17's own NOTES.md, which found it neutral-to-harmful.
