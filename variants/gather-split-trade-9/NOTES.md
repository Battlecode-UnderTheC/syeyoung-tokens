# gather-split-trade-9

## Idea

Pure parameter tweak on gather-split-trade-5: start feeding the sonar-agreed champion
earlier. Earlier consolidation should give the champion more rounds to grow off its
teammates' bodies before the 500-round cap, at the cost of workers giving up their own
growth sooner.

## What changed from the parents, and why

`Params::feed_from_round` 360 -> 340 in `strategy.hpp` (one line). Nothing else touched.
On gather-split-trade-4, prior tuning runs found 340 beat 360 35-27 in sims, and 320 was
33-29 (worse than 340), so 340 looked like the local optimum to try next. Namespace
renamed `gst5` -> `gst9` (and macros `GST5_*` -> `GST9_*`) throughout so both variants can
load in one simulator without symbol clashes; no logic changed by the rename.

## What measured

- `build/tools/check gather-split-trade-9 maps/official`: all checks passed, 0 avoidable
  deaths on every map.
- `build/tools/sim --a gather-split-trade-9 --b gather-split-trade-5 --maps maps/official --threads 2`:
  **11-0-11** (W-D-L) over 22 matches (both sides as A and B on all 11 official maps).
  Final state: gst9 leads by +0.1 longest, +10.8 total length on average.
- `build/tools/sim --a gather-split-trade-9 --b gather-split-trade-5 --maps build/practice --threads 2`
  on 20 fresh worldgen maps (40 matches, both sides): **23-0-17** (W-D-L). Final state:
  gst9 trails by -0.7 longest, -20.7 total on average (a few worldgen maps had big
  single-match total-length swings that outweigh the extra wins).
- Combined: 34-0-28 in gst9's favor across 62 matches, roughly even to slightly ahead,
  consistent with the g3-4 sim numbers that motivated the try (35-27 there).

## What should improve

Slightly more matches won by feeding the champion earlier, especially on longer/denser
maps where the game would otherwise end near round 500 with the champion still short of
its potential length. The margin effect is noisier than the win/loss effect: a few
worldgen maps have very lopsided total-length outcomes for the *losing* side regardless of
feed_from_round, which swamps the average-margin signal. Win/loss counts look like the more
reliable metric to track for this kind of small consolidation-timing tweak.

## What didn't work / wasn't tried

Did not sweep other values (320 was already known worse from prior tuning on gst4). Did
not touch any other parameter — this was meant as a single-variable check per the brief.
