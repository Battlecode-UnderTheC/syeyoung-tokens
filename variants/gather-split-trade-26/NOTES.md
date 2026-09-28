# gather-split-trade-26

## Idea

gather-split-trade-25 (gst15 + bed denial + cap feeding) with gather-split-trade-23's ghost-champion
fix: `Champion()` picks the sonar report with the highest `length - report_fade * age`
(report_fade 0.5) instead of the longest report within the 40-round memory.

## What changed from the parents, and why

- Namespace/macros renamed gst25 -> gst26.
- `Champion()` scoring and the `report_fade = 0.5` parameter copied verbatim from gst23 (the only
  diff between gst23 and gst15). Without it, a trapped champion that splits length-2 leaves a stub
  under its id that dies; its last report stays the longest for up to 40 rounds, so allies feed
  (and cap-feed) a dead dragon. With the fade, the heir, reporting every round, takes over.

## Measurements (sim, both sides, 2 threads)

| opponent | maps | W-D-L |
|---|---|---|
| gather-split-trade-25 | official (11) | 12-0-10 (longest +2.7) |
| gather-split-trade-25 | 20 worldgen | 23-0-17 (longest +1.7) |
| gather-split-trade-23 | official | 12-0-10 |
| gather-split-trade-23 | 20 worldgen | 18-0-22 (longest +2.5) |
| counter-swarm-5 | official | 7-0-15 (gst25: 7-0-15) |
| counter-swarm-5 | 20 worldgen | 26-0-14 (gst25: 26-0-14; longest margin +8.8 vs +5.7) |

check: all checks passed, slowest turn 0.2 ms.

## What should improve

Late-game feeding goes to a live dragon: slightly higher longest-dragon margin. Against
counter-swarm-5 the W-D-L did not move on this map set (unlike gst23's 32-8 vs 26-14 over gst15),
only the longest margin; cap feeding in gst25 may already mask part of the ghost problem.

## Tried, didn't work

Nothing else tried (short loop).
