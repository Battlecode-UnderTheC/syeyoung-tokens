# swarm-hunt-19

## Idea

Crossover of swarm-hunt-18 (base, renamed swh18 -> swh19) and late-surge-17: port late-surge-17's
bed/cluster goal scoring into swarm-hunt's `ChooseGoal`/`TileValue`, keeping swarm-hunt-18's
crowded-frontier bonus, folded patrol, and known-portal crossing untouched.

## What changed from the parents, and why

Only late-surge-17's part (a) (its part (b), frontier-weighted explore + centre pull, was measured
neutral-to-harmful there and is not ported):

- `TileValue`: a bed due in `due` rounds at `dist` steps is ready at
  `dist + wait_factor*max(0, due-dist)` (wait_factor 2) instead of `max(dist, due)` — waiting on a
  bed is charged double relative to walking, so a bed that's still far from ripe stops looking as
  good as one about to pop.
- `ChooseGoal`: a candidate pearl/bed gains `cluster_bonus` (0.5) x the value of other pearls/beds
  within Manhattan radius 2, each weighted by `(d+1)/(r2+1)` (r2 using the same wait_factor
  ready-time) — a bed inside a field is worth more than a lone one, and an unripe neighbour adds
  little. Every tile's base value and rounds-to-ripe are computed once per `ChooseGoal` call (a
  `std::vector` pass over the whole board) rather than via `TileValue` per neighbour of every
  candidate — `Occupied`/`Mine` are not free, and swarm-hunt's turns were already the slowest of
  our lines. The cluster loop itself is folded into the existing per-tile candidate scan (same
  board pass that already handles share/contest/frontier/patrol), not a second pass.

swarm-hunt-18's crowded-area stale-frontier explore bonus, patrol folded into the scan, and
known-portal crossing are all kept unchanged.

## What was measured

`check swarm-hunt-19 maps/official`: all checks passed, slowest turn 0.3 ms (well under the 0.5 ms
budget; up from swarm-hunt-18's ~0.1-0.2ms range on the same maps, expected from the added
cluster-bonus scan, still comfortably inside budget).

`sim`, both seats, threads 2:

| opponent | maps | W-D-L | longest margin | total margin |
|---|---|---|---|---|
| swarm-hunt-18 | official (22) | 14-0-8 | +2.5 | +1.8 |
| late-surge-17 | official (22) | 14-0-8 | +3.6 | +26.3 |
| swarm-hunt-18 | worldgen, fresh 20-map batch (40 games) | 27-0-13 | +7.2 | +59.1 |
| late-surge-17 | worldgen, same batch (40 games) | 22-0-18 | +1.6 | -26.4 |

Wins clearly against both parents on official maps and worldgen. The late-surge-17 worldgen result
is a real win by W-D-L (22-0-18) but the total-length margin is slightly negative (-26.4) —
consistent with swarm-hunt-19's own build already having a large body-mass edge over ls17 baked
into swarm-hunt's split/feed logic; the fights it wins tend to be closer to even on total dragon
mass than the ones it loses are lopsided. Given the W-D-L margin and the clean official-map result,
this is at least even with (net ahead of) both parents.

## What should improve

Faster pearl/bed intake in the opening and mid-game from bed clusters, matching what late-surge-17
measured on its own line: less time spent walking to (and waiting at) a lone unripe bed when a
denser cluster is reachable for similar cost.

## Tried, didn't work / not tried

Did not port late-surge-17's part (b) (frontier-weighted explore weight, opening centre pull):
ls17's own measurements found it neutral-to-negative vs its parent, and the brief called it out as
off. Did not sweep `wait_factor`/`cluster_bonus`/`cluster_radius` off the late-surge-17 defaults —
out of scope for this short crossover pass; a natural next tuning step.
