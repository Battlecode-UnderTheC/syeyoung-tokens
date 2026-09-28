# gather-split-trade-25

## Idea

Crossover of three gst15 descendants: gst19 (bed denial, the base) + gst20's cap feeding
(at the 64-dragon cap, a length<=3 non-leader worker within 2 steps of a longer ally's head
resigns next to it before round 340: "cap: feed ally"). gst22's champion freeze (the sonar
champion never splits/trades/lunges/hunts from round 300) is ported but switched off
(`heir_from_round = 100000`), because the three-way merge measured worse than the two-way.

## What changed from the parents, and why

- Sources start from gst22 (= gst19 + freeze, code kept), renamed to `gst25`.
- Added gst20's block 2c (cap feed) after the lunge, also skipped for the heir.
- `heir_from_round` 300 -> 100000 (freeze off). Set it back to 300 to get the full merge.

## Measurements (sim, maps/official 22 matches + 20 fresh worldgen maps 40 matches)

| variant                        | vs gst19 (off + wg)   | vs gst20 (off + wg)   | total   |
|--------------------------------|-----------------------|-----------------------|---------|
| full merge (cap + freeze)      | 9-13, 21-19 = 30-32   | 12-10, 19-21 = 31-31  | 61-63   |
| **cap only (this commit)**     | 12-10, 22-18 = 34-28  | 11-11, 20-20 = 31-31  | 65-59   |
| gst22 (freeze only), reference | 11-11, 19-21 = 30-32  | 12-10, 20-20 = 32-30  | 62-62   |

No draws. All within noise (+-8 on 62 matches); the cap-only combo is the best of three.
check: all checks passed; slowest turn is dominated by machine load (gst19 measured the same).

## What should improve

Fewer wasted short workers when the team is at the unit cap: their slot goes to a split of a
longer dragon. Cap feed fires ~140 times per 40 matches on worldgen maps.

## Tried, didn't work

Stacking the champion freeze on top: slightly worse vs gst19 (30-32 vs 34-28).
Note: the HeirProbe reasons from gst22 always tally 0 (the later Because() overwrites them).
