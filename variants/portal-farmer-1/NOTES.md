# portal-farmer-1

## Idea

First cut of the `portal-farmer` archetype on gather-split-trade-39's code (namespace `pf1`).
Each dragon finds *pockets* in its board memory: flood fill over known tiles through plain edges
(not kelp, not portals); a region that never touches an unknown edge or unseen tile, is at most
1/4 of the board, has a portal on its boundary and holds a bed is a portal-only store. Pearls and
beds in a pocket are worth 2x (`pocket_bonus`), and a dragon inside one explores at 0.3x
(`pocket_explore`), so it stays and farms the beds on their countdowns. Recomputed every 4 rounds.
Reason tag `pocket: farm` when the goal is in a pocket.

A second mechanism, the *lure*, is in the code but OFF (`lure_mod = 0`): a farmer (id % lure_mod
== 0) that has seen >= 4 beds/pearls it knows no way to reach values tiles beside unknown
portals (lure_value / (d+1)) and crosses them even with pearls in reach.

## What changed from the parents, and why

- `FindPockets`, `InPocket`, pocket bonus in `TileValue`, lower explore value inside a pocket.
- Lure (off). Debug counters under `PF1_DEBUG` (pocket pearls eaten, lure funnel); `PF1_NOPOCKET`
  builds the parent's behaviour with the counters (measurement baseline only).
- Did not do: guarding the portal entrance.

## Measured (sim, 11 official maps x 2 seats; 20 fresh worldgen maps x 2 seats)

| opponent | official | worldgen |
|---|---|---|
| gather-split-trade-39 | 11-0-11 | 19-0-21 |
| late-surge-15 | 10-0-12 | 20-0-20 |
| trading-swarm-4 | 9-0-13 | 25-0-15 |

gst39 itself scores exactly the same W-D-L vs late-surge-15 and trading-swarm-4: match outcomes
are almost unchanged. Pocket pearls eaten (one trace match, as A, vs gst39; baseline = the same
code with the pocket bonus off): default 25 -> 255 (total eaten 155 -> 492), queen_of_spades
90 -> 172, stronghold 31 -> 70, trauma 40 -> 21. Pockets almost never trigger on worldgen maps.

## What should improve

Default-style maps (small kelp compartments with portal walls): dragons that land in a
compartment stay and farm it instead of leaving to explore.

## What didn't work

- Trauma's store is one big maze (26x24, bounded by kelp, 4 entry portals from the strip); it is
  never fully seen, so it is never a "pocket". The lure was meant to handle it, but lured dragons
  rarely reach an entry portal: in ~3400 lured dragon-turns on trauma only ~50 stood on a
  pinged portal tile; the unknown portals they know are mostly maze-internal ones seen through the
  wall (known 2160 / reachable 547 lured turns). Lure at id%2 (value 3, explore x4): 8-14 official,
  16-24 worldgen vs gst39; at id%4 (value 3): 9-13, 16-24. Off.
- Next: find the entry portals of a seen-but-unreachable store directly (the strip-side portal
  tiles along the store's kelp wall), send one or two dragons with known body through, and
  hold them in by treating the store as a pocket once inside (relax "closed" to "reached
  only through portals from the head's region").
