# gather-split-trade-32

## Idea

gather-split-trade-31 with a quieter mid game: workers no longer hunt enemy heads
(hunt_radius 6 -> 0). This is the second of late-surge's two "quiet" traits (g7-7); the first
(trade_margin 0 -> 1) was measured and dropped.

## What changed from the parents, and why

- Namespace gst31 -> gst32 (macros too).
- `hunt_radius = 0` (was 6). Workers still take head-to-head trades that come to them
  (WorthTrading, trade_margin 0), they just don't walk towards enemy heads to force them.
- `trade_margin` stays 0: tried 1, it lost.

## Measurements (sim, both sides, W-D-L; worldgen = two fresh batches of 20)

| variant | opp | official | worldgen A | worldgen B | total |
|---|---|---|---|---|---|
| gst-31 (baseline) | late-surge-6 | 11-0-11 | 17-0-23 | 19-0-21 | 47-55 |
| trade_margin 1 | gst-31 | 10-0-12 | 15-0-25 | 19-0-21 | 44-58 |
| trade_margin 1 | late-surge-6 | 9-0-13 | 17-0-23 | 25-0-15 | 51-51 |
| **hunt_radius 0 (kept)** | gst-31 | 15-0-7 | 17-0-23 | 22-0-18 | 54-48 |
| **hunt_radius 0 (kept)** | late-surge-6 | 12-0-10 | 21-0-19 | 27-0-13 | 60-42 |

Small crowded official maps (arena, Colosseum, devil, default_small) vs late-surge-6: 4-4,
same as gst-31; vs gst-31: 5-3. So dropping the hunt didn't cost the crowded maps.

## What should improve

Worldgen vs late-surge (38-42 -> 48-32 over 80 matches) and official even-or-better.
Head-to-head vs the parent is mixed on worldgen A (17-23), so gains are modest/noisy.

## Tried, didn't work

- trade_margin 1 (only strictly longer enemies): lost to the parent everywhere (44-58).
  Not tested combined with hunt_radius 0 (budget).
