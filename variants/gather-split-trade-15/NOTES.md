# gather-split-trade-15

Crossover (task g4-6): gather-split-trade-12 (base) + gather-split-trade-9 + gather-split-trade-13.

## Idea

Stack three small gains measured separately on children of gather-split-trade-5: gst12's
contested-target discount (a target an enemy head in view reaches first is worth 0.3),
gst9's earlier feeding and gst13's later split cutoff.

## What changed from the parents, and why

- Base: gather-split-trade-12 sources, namespace/macros `gst12`/`GST12` -> `gst15`/`GST15`.
- `Params::feed_from_round` 360 -> 340 (from gst9).
- `Params::split_until_round` 300 -> 350 (from gst13). `leader_split_until` stays 300.
- Confirmed by diffing gst9 and gst13 against gst5 with the namespace normalised: each
  changes only that one line (gst13 also left its macros named `GST5_*`, which is harmless).

Note the two knobs overlap: workers now split off until 350 while feeding starts at 340.

## Measured (native sim, both sides of every map)

| opponent | maps | W-D-L | longest / total margin |
|---|---|---|---|
| gather-split-trade-12 | official (11) | 13-0-9 | +3.5 / +48.0 |
| gather-split-trade-12 | worldgen, 20 fresh | 27-0-13 | +9.3 / +61.0 |
| gather-split-trade-5 | official (11) | 13-0-9 | -0.3 / +10.4 |
| gather-split-trade-5 | worldgen, 20 fresh | 21-0-19 | +6.6 / +73.2 |

`check` on official: all checks passed, slowest turn 0.1 ms.

## What should improve

More late-game length: more workers produced (split to 350) and a longer feeding window
(from 340) for the champion. The merge beat gst12 alone on both map sets, so the
drop-one-change ablation was not run.
