# gather-split-trade-45

## Idea

Crossover: port late-surge-17's bed/cluster goal scoring (part (a) only; (b), the
frontier/centre pull, stays off since ls17 found it neutral-to-harmful) into
gather-split-trade-42, gst42's other machinery (splitting, feeding, consolidation,
known-portal crossing) unchanged.

## What changed from the parents, and why

Namespace/macros `gst42`/`GST42_*` -> `gst45`/`GST45_*` (crossover, must not clash when
loaded alongside a parent).

`TileValue`/`ChooseGoal` (gst42's version, which is nearly identical to ls17's, both
descending from gather-split-trade-23/38):

- A bed ripening in `due` rounds at `dist` steps is now ready at
  `dist + wait_factor*max(0, due-dist)` (`wait_factor` 2.0) instead of `max(dist, due)`:
  waiting for a bed to ripen is charged double versus walking there.
- A candidate pearl/bed gains `cluster_bonus` (0.5) x the value of other pearls/beds within
  `cluster_radius` (2, Manhattan) steps, each weighted by `(d+1)/(their-ready+1)` so a bed far
  from ripe adds little: a pearl/bed in a field is worth more than a lone one.

Perf: the naive port (full board scan, cluster-scan every scored tile) showed real turn-time
regressions in `check` even after removing the double `TileValue` pass ls17's structure implies
(computing `base`/`due` once) — `big_empty`/`default`/`devil` went from ~0.1-1ms to 10-45ms+ in
back-to-back runs against the same-run parent, not explained by the machine's load alone (the
parent stayed under ~1ms in the same runs at the time). Root cause: `TileValue` calls
`Occupied`, which calls `Mine`, an O(body length) linear scan — doing that twice per tile
(once in the precompute pass, once in the main scoring pass) over the whole board is O(size x
length) x2, and gst42's dragons get much longer late-game than what a quick sanity check
exercises. Fixes, in this variant only:
1. Precompute a `mine_mask` (O(length) once per `ChooseGoal`, not O(length) per tile check).
2. Reuse the precomputed `base`/`due0` tables for the main scoring pass instead of calling
   `TileValue` a second time per tile (pearl/bed value doesn't depend on `dist`, only `ready`
   does, and `ready` is cheap to recompute from `due0[t]`).
3. Bound the O(radius^2) cluster scan to a `cluster_pool` of the top ~40 tiles by
   pre-cluster score (and their neighbours) instead of every tile with positive value: a dense
   pearl field otherwise makes the scan cost proportional to pearl count x 13, which spiked on
   maps with many pearls (`schooltime`, `stronghold`, `trophy`).

After these three fixes, `check`'s slowest turn for gst45 vs gst42 back-to-back on
`maps/official` was in the same ballpark (sometimes faster, e.g. default 3.0ms vs 11.1ms;
sometimes ~2-3x, e.g. default 29.8ms vs 12.8ms in a later, more heavily loaded run) rather than
the earlier 20-150x blowout. The machine was heavily loaded throughout (parent's own slowest
turn swung 0.1ms-107ms run to run on the same maps), so these are relative, not absolute,
readings; no avoidable deaths, all checks passed every run.

## What measured (sim, both seats)

- vs `gather-split-trade-42` (parent), `maps/official` (22 matches): **15-0-7**, leads by 6.9
  longest / +29.3 total at the end.
- vs `late-surge-17` (parent), `maps/official` (22 matches): **15-0-7**, leads by 4.2 longest /
  +23.7 total.
- vs `gather-split-trade-42`, 20 fresh worldgen maps both seats (40 matches): **23-0-17**,
  leads by 5.5 longest / +55.6 total.
- vs `late-surge-17`, the same 20 worldgen maps (40 matches): **23-0-17**, leads by 6.3
  longest / +37.7 total.

Even against both parents on both map sets, with a healthy positive length margin each time.

## What should improve

Faster, larger pearl intake (ls17's original motivation): waiting on a bed charged double
pushes goal choice toward beds that are actually close to ripe rather than ones merely close by;
the cluster bonus should pull dragons into pearl fields/bed clusters rather than circling single
distant targets, cutting wasted travel. This carries into gst42's split/feed/consolidation
machinery unchanged, so it should compound with gst42's existing strengths.

## Tried, didn't work / had to fix

- ls17's part (b) (frontier-weighted explore edge + opening centre pull): left off, per ls17's
  own finding that it was neutral-to-negative.
- The literal port of ls17's scoring loop (full O(size) cluster scan per candidate) was fast
  enough for ls17 apparently, but not for gst42: gst42's dragons run longer late-game, and the
  official maps include some with dense pearl fields, both of which this port is sensitive to.
  Bounding the cluster scan to a fixed-size pool of top candidates (see above) fixed it without
  changing the WDL/margin outcome measured above (same numbers before and after the perf fix,
  within noise).
