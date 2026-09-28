# giant-4

## Idea

giant-2 (round-0 giants split until 100 and then only grow, fed by a helper swarm that finds
the sonar "king") with late-surge's endgame as isolated in gather-split-trade-31: feed only a
clearly longer king, trust a fully visible king's length, and let non-king giants feed it too.

## What changed from the parents, and why

Base giant-2 (namespace `giant2` -> `giant4`, macros `GIANT4_*`). From gather-split-trade-31:

- **Giants feed too** (`giant_feed = 1`): from `feed_from_round`, a giant that is not the king
  walks to it and dies within `feed_distance = 3`, if the king report is at most 20 rounds old.
- **Feed only a clearly longer king** (`feed_clear_tenths = 8`): with the king in view, feed
  only if our length < 80% of its known length; otherwise hold and keep gathering
  (reason `held: king not clearly longer`).
- **`trust_visible`**: a king whose whole body is in view (last segment not on the window
  border) counts its visible length, not the max with its old report, so a trap-split stub
  isn't taken for the old king.
- **`champ_freeze = 330`**: the king never growth-splits from 330. Mostly moot here, since giants
  stop splitting at 100 anyway.
- The king ranking was already late-surge's (`length − 0.5 × age`, 40-round expiry), so the
  reports stay compatible.

Deviations from late-surge's numbers, both measured: the helpers' split window stays at 350,
not 400, and the surge starts at 340, not 380 (giant-2's own feed round).

## Measurements (build/tools/sim, 2 threads; worldgen = one fresh batch of 20 maps, ×2 sides)

| giant-4 setting | vs giant-2 official | vs giant-2 worldgen | vs gst-31 official | vs gst-31 worldgen |
|---|---|---|---|---|
| late-surge as is (split 400, feed 380, giants feed) | 8-0-14, longest -2.0 | 23-0-17, +4.5 | | |
| same, giants don't feed (`giant_feed 0`) | 7-0-15, -5.0 | 21-0-19, +3.1 | | |
| split 350, feed 380 | 11-0-11, -2.8 | 24-0-16, +6.7 | 5-0-17, -13.6 | 7-0-33, -14.2 |
| **final: split 350, feed 340** | **11-0-11, -0.3** | **24-1-15, +10.0** | **7-0-15, -7.5** | **11-0-29, -8.3** |
| (reference: giant-2) | | | 8-0-14, -7.9 | 9-0-31, -7.9 |

check: all checks passed. The slowest turn reads 7 to 21 ms, but load average was about 90 and
giant-2 read the same (7.6 to 13.9 ms), so this is not a regression.

## What should improve

Better than giant-2 on worldgen (24-15, +10 longest) and even on official. Against gst-31 it is
about even with giant-2 (official 7-15 vs 8-14, worldgen 11-29 vs 9-31). The giant line still
loses to the gather-split-trade line by about 8 longest; the endgame doesn't close that gap.

## Tried, didn't work

- Surge at 380 in this line: clearly worse against gst-31 (longest -14 vs -8). The giant's
  helpers are spread out and need the extra 40 rounds to reach the king.
- Helpers splitting until 400: worse against giant-2 on official (8-14 vs 11-11).
- Only helpers feeding (giants hold): slightly worse on both sets.
