# gather-split-trade-31

## Idea

Crossover testing which half carries late-surge's edge: gather-split-trade-27's early and mid
game (hunting, trade_margin 0, lunges, cap safety) with late-surge-6's endgame (late split,
everyone surges to the sonar champion at 380, feeds only a clearly longer champion).

## What changed from the parents, and why

Base gather-split-trade-27 (namespace `gst27` -> `gst31`). Taken from late-surge-6, endgame only:

- `split_until_round` 350 -> 400 (workers keep splitting longer).
- `feed_from_round` 340 -> 380, `leader_feed_from` 440 -> 380, `leader_feed_fresh` 10 -> 20:
  from 380 every dragon, leaders included, walks to the champion.
- `feed_distance` 2 -> 3.
- `champ_freeze = 330`: the champion stops growth-splitting from round 330.
- `feed_clear_tenths = 8`: feed only if our length < 80% of the champion's known length,
  otherwise hold and keep gathering (reason `held: peer not clearly longer`).
- `trust_visible = 1`: an ally whose whole body is in view counts its visible length.

Kept from gst27 (late-surge's quiet mid game is NOT taken): `hunt_radius 6`, `trade_margin 0`,
lunges, cap_safety / reserve logic, bed denial.

## Measurements (build/tools/sim, 2 threads; worldgen = one fresh batch of 20 maps)

| a vs b | official (22) | worldgen 20 maps (40) |
|---|---|---|
| gst31 vs gather-split-trade-27 | 12-0-10, longest -2.4 | 27-0-13, longest +8.8 |
| gst31 vs late-surge-6 | 11-0-11, longest +2.2 | 18-0-22, longest -4.8 |
| (reference) gst27 vs late-surge-6 | - | 15-0-25, longest -8.3 |

check: all checks passed (slowest turn 0.8 ms on trauma under a loaded machine; unchanged
search code from gst27, the added work is O(segments in view)).

## What should improve

The endgame half carries most of late-surge's worldgen edge: swapping it in turns gst27's
15-25 vs late-surge-6 into 18-22, and beats gst27 27-13 on worldgen while staying even-ish on
the official maps. The remaining gap vs late-surge-6 on worldgen is presumably the quiet
mid game (no hunting, trade_margin 1, no lunge).

## Tried, didn't work

Nothing else tried (short loop).
