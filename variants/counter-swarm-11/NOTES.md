# counter-swarm-11

Crossover (task g6-3): counter-swarm-5 (base) + gather-split-trade-23 + gather-split-trade-20.

## Idea

Keep counter-swarm-5's style (multi-step strikes, its economy) and stack the two
champion-feeding fixes found on the gather-split-trade line: stop feeding a ghost champion,
and at the unit cap let short workers give their slot to a longer ally.

## What changed from the parents, and why

- Namespace/macros `cs5`/`CS5` -> `cs11`/`CS11`; files cs11.hpp/cs11.cpp.
- `Champion()` (from gst23): pick the report with the highest `length - report_fade * age`
  (`report_fade = 0.5`) instead of the longest under 40 rounds old. cs5 carried the
  ghost-champion bug (a trapped champion splits, the stub keeping its id dies, and its
  stale report keeps winning, so the team feeds a dead dragon).
- Step 2c (from gst20), after strikes and before survival: at `units >= unit_limit`, before
  `feed_from_round` (340), a non-leader of length <= 3 that sees a longer ally's head (this
  round) within 2 steps (via a free tile) resigns next to it ("cap: feed ally"), freeing a
  slot for a split. Params `cap_feed_max_length = 3`, `cap_feed_slack = 0`.
- Both diffs checked against gather-split-trade-15 with namespaces normalised: each is only
  that change.

## Measured (native sim, both sides of every map; 20 fresh worldgen maps)

| A vs B | official (11) | worldgen (20) |
|---|---|---|
| cs11 vs counter-swarm-5 | 12-0-10 | 25-0-15 |
| cs11 vs gather-split-trade-23 | 16-0-6 | 12-0-28 |
| cs5 vs gather-split-trade-23 (baseline, same maps) | 12-0-10 | 8-0-32 |

`check` on official: all checks passed (slowest turns 1-7 ms measured under machine load
~45-60; the changes add only a small loop over seen dragons at the cap).

~85-90 "cap: feed ally" deaths per 40 worldgen games.

## What should improve

More late length from feeding the live champion; more splits at the cap. Still behind
gst23 on worldgen (12-28), though less than cs5 (8-32).

## Not tried

No parameter sweep (short task). Next: check whether cs5's strikes cost the champion; try
the champion-trap avoidance from gst's "Next" list.
