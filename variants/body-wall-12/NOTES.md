# body-wall-12

## Idea

Crossover: port late-surge-17's bed/cluster goal-scoring fix into body-wall-11. A bed is
charged for waiting, not just distance, and a pearl/bed in a field of others is worth more
than a lone one, so the gatherer spends less time circling a slow bed and more time on
dense clusters. Keeps body-wall-11's enemy-head threat pings unchanged.

## What changed from the parents, and why

body-wall-11 (namespace `bw11` -> `bw12`, `BW11_*` -> `BW12_*`), `TileValue`/`ChooseGoal` only,
mirroring late-surge-17's part (a) (its part (b), frontier-weighted explore + opening centre
pull, was left out: ls17's own measurements found it neutral-to-harmful):

- A bed ripening in `due` rounds at `dist` steps is now ready at
  `dist + wait_factor*max(0, due-dist)` (`wait_factor = 2.0`) instead of `max(dist, due)`,
  so a round spent waiting costs twice a round spent walking.
- A candidate pearl/bed gains `cluster_bonus` (0.5) x the value of other pearls/beds within
  Manhattan `cluster_radius` (2), each weighted by `(d+1)/(r2+1)` where `r2` uses the same
  wait-charged readiness — a bed far from ripe adds little to its neighbours' value.
- Tile base values and rounds-to-ripe are precomputed once per `ChooseGoal` call (a
  `base[]`/`due0[]` array over the board) instead of calling `TileValue` per neighbour of
  every tile, since `Occupied`/`Mine` are not cheap and `check`'s slowest turns for
  body-wall are already high.
- New params `wait_factor`, `cluster_bonus`, `cluster_radius` (defaults matching ls17),
  wired into the `BW12_P=`/`BW12_TUNE` override list.

## What I measured

`build/tools/check body-wall-12 maps/official`: all checks passed, 0 avoidable deaths.
Slowest-turn numbers were noisy run to run (machine under load, seen from 0.1 ms up to
~90 ms on `default`/`big_empty`/etc across repeated runs) but tracked body-wall-11's own
noise on the same maps in back-to-back runs — no systematic slowdown from the change; the
precompute keeps the cluster bonus at O(board size x cluster area) per `ChooseGoal`, not
per tile-neighbour.

`sim`, both seats, 2 threads:

| opponent | maps | W-D-L | margin |
|---|---|---|---|
| body-wall-11 | official (22) | 13-0-9 | +36.8 total, +3.6 longest |
| late-surge-17 | official (22) | 13-0-9 | +24.9 total, +1.0 longest |
| body-wall-11 | worldgen, 20 maps both seats (40) | 22-0-18 | +20.6 total, +1.1 longest |
| late-surge-17 | worldgen, 20 maps both seats (40) | 21-0-19 | -15.8 total, +0.1 longest |

Wins in every set against both parents; worldgen vs late-surge-17 is basically even on
total length (win count still favours body-wall-12) despite a strong length lead on
official maps, likely map-mix noise from only 20 seeds.

## What should improve

Faster pearl/bed intake early, especially in bed-cluster-rich maps, without changing
body-wall's wall/threat/hunt behaviour.

## Tried, didn't work

Nothing else attempted this task — budget was for the one focused port. ls17's part (b)
(frontier explore weighting + centre pull) was deliberately left out per the brief and
ls17's own NOTES.md, which found it neutral-to-harmful.
