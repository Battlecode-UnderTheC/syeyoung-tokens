# territory-2

## Idea

Archetype `territory`, sharpened to bed clusters. A third of the workers (split children
whose id is a multiple of 3) become *tenders*: every 10 rounds each picks, from its own board
memory, the richest reachable bed cluster (a bed with at least 3 beds within 3 steps,
scored beds / (path distance + 15)) and farms it until round 380 (feeding starts): targets
inside the cluster score x2, outside x0.5, and with nothing worth taking the tender walks back
to the cluster centre and waits there instead of patrolling the map. Leaders, the champion
protocol, feeding and the endgame are untouched; the other workers gather as the base does.
Assignment is by id + nearest cluster, no sonar change.

## What changed from the parents, and why

Base: gather-split-trade-39 (namespace renamed `gst39` -> `ter2`, macros `GST39_*` -> `TER2_*`).
Added `Params::territory` and cluster params, `Tender()`, `ChooseHome()`, `InHome()` in
strategy.cpp; hook in `ChooseGoal` (score multiplier, wait-at-home fallback) and new action
reasons `tend: farm cluster / go home / outside`. `-DTER2_OFF` compiles the parent behaviour
back (must be defined for plugin.cpp too: `Params{}` is the default argument, built there).

## Measurements (vs gather-split-trade-39, both seats)

Headroom (`TER2_HEADROOM`, compiled out; distinct (bed, round) pairs our team saw holding an
uneaten pearl, 22 official matches): parent 98.8k pearl-rounds (~4.5k a match, 96% of them on
beds with >= 3 beds within 3 steps -- almost all beds on these maps are "cluster" beds, so the
cluster filter barely selects). A bed holding a pearl does not regrow, so these are lost ticks.
With tenders the count went *up* (111k): they stand near clusters and see more pearls; the
metric is exposure-biased and is not a clean waste measure.

| maps | territory-2 | identical parent code (TER2_OFF) |
|---|---|---|
| official (22) | 12-0-10, longest +5.5, total +11.0 | 9-0-13, -3.9, -14.0 |
| worldgen set 1 (40) | 21-0-19, -1.3, -0.4 | 19-0-21, -1.4, -9.3 |
| worldgen set 2 (40) | 17-0-23, -3.7, +18.8 | 17-0-23, -1.4, -27.0 |
| total | 50-0-52 | 45-0-57 |

The parent against itself loses 45-57 here (seat/order noise), so the self-play noise band is
about +-6 games; territory-2 is even with the parent, a little better on total length in all
three sets. `check`: all checks passed.

## What should improve

Total length (fewer idle bed ticks near the tenders). Expect even W-D-L against the pool.

## Not tried / next

No parameter sweep (short loop). Ideas: a stricter cluster definition (radius 2, 4+ beds, or
beds weighted by their gap range) so it selects real hot spots; tenders from the champion's
side; share the chosen cluster by sonar so two tenders don't pick the same one; guard the
cluster (body-block) against enemy heads.
