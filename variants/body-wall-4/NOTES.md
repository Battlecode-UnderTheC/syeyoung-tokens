# body-wall-4

## Idea

body-wall-3 with walling given its own cutoff. Porting late-surge's endgame (feed from 380)
into body-wall-3 had moved the stop of the wall squeeze and the waller/cap-waller chase from
340 to 380, because both were keyed on `feed_from_round`. body-wall-4 adds
`wall_until = 340`: the squeeze (`Wall`) and the waller chase goal stop at 340, feeding and
hunting still stop/start at 380. Namespace renamed `bw3` -> `bw4` (and macros).

## What changed from the parents, and why

- `Params::wall_until = 340` (new); `ChooseGoal`'s waller chase and `Wall`'s squeeze test it
  instead of `feed_from_round`. Hunting (`hunt_min_units`) still uses `feed_from_round`.
- Why: walling burns the waller's time late in the game, when growing for the surge matters more.

## Measured (sim, 2 threads; official = 11 maps x 2 sides; worldgen = 20 fresh maps x 2 sides)

| config | vs body-wall-3 official | vs bw3 worldgen | vs gst-31 official | vs gst-31 worldgen |
|---|---|---|---|---|
| wall_until 340 (kept) | 12-0-10 | 21-0-19 | 10-0-12 | 19-0-21 |
| wall_until 300 | 12-0-10 | 19-0-21 | 13-0-9 | 16-0-24 |
| body-wall-3 itself (380) | - | - | 11-0-11 | 15-0-25 |

Squeezes ("wall: squeeze" actions, 40 worldgen matches vs bw3): 380 -> 228, 340 -> 200, 300 -> 171
(about 5.7 / 5.0 / 4.3 per match). Official: 26 / 24 / 21 over 22 matches.

## What should improve

Slightly better vs gather-split-trade-31 on worldgen (19-21 vs bw3's 15-25), even vs parent.
All differences are within noise at these sample sizes; 340 and 300 are indistinguishable
overall (29-33 vs gst-31 either way), 340 kept for being a bit better vs the parent.

## Tried, didn't work

- wall_until 300: better on official vs gst-31 (13-9) but worse on worldgen (16-24); no net gain.
