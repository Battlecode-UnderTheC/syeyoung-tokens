# swarm-hunt-6

Crossover (task g6-4): swarm-hunt-5 (base) + gather-split-trade-23 + gather-split-trade-20.

## Idea

swarm-hunt-5 with the ghost-champion fix and cap feeding ported from the gather-split-trade line.
swarm-hunt's own style is unchanged: hunters lunge/strike, keepers stay out of range, team B plays
in a 180-degree-rotated frame (MirrorAgent).

## What changed from the parents, and why

- Namespace `swh5` -> `swh6`.
- `Champion()` (from gst23): pick max `length - report_fade * age` (report_fade 0.5) instead of max
  length. A champion that gets trapped splits its mass into a new id and the stub dies; its last
  report (only ever relayed with its last round) now fades, and the live heir takes over.
- Cap feed (from gst20): a hunter (not a keeper) of length <= 3, at UNIT_LIMIT, before round 340,
  that sees a longer ally's head within 2 steps resigns next to it ("cap: feed ally").
  Params `cap_feed_max_length = 3`, `cap_feed_until = 340`.

## Measured (native sim, both sides of every map; 20 fresh worldgen maps)

| opponent | maps | swarm-hunt-6 | swarm-hunt-5 (same maps) |
|---|---|---|---|
| swarm-hunt-5 | official | 12-0-10 (+2.0 longest, +7 total) | |
| swarm-hunt-5 | worldgen 20 | 21-0-19 (+1.0 longest, -38 total) | |
| gather-split-trade-23 | official | 8-0-14 (-3.9 longest) | 6-1-15 (-9.8) |
| gather-split-trade-23 | worldgen 20 | 19-0-21 (+3.3 longest) | 14-0-26 (-3.9) |

`check` on official: all checks passed (slowest-turn numbers were wall-clock noisy under
machine load; the parent showed the same spikes in the same run).

## What should improve

Longest-dragon margin in the late game: fewer dragons walking to a dead champion. Most of the
gain is from the Champion() fix: cap feeding fires only 1-2 times a match here, because
swarm-hunt keeps `unit_reserve = 6` slots free for normal splits and so rarely sits at 64.

## Not tried

report_fade and cap-feed thresholds untuned (short task). Cap feed with slack (units >= 58,
matching unit_reserve) might matter more for this line than for gst.
