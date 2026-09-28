# gather-split-trade-52

## Idea

Small crowded maps are decided by regrowth after one-for-one trades. Port late-surge's
outnumbered mode, gated to small maps (<= 800 tiles, before r200): when the enemies seen in
the last 30 rounds clearly outnumber us (>= 4 and > 1.5x our dragons, or more than us after
contact by r25), stop giving even trades and keep two steps off enemy heads we won't trade
with, so the team gathers and splits back instead of trading down.

## What changed from the parents, and why

gather-split-trade-50 plus `UpdateOutnumbered` / `ExposedNear` / the WorthTrading rule from
late-surge-21 (`om_*` params; `GST52_OFF` turns it off). Big maps are untouched (SmallMap gate;
ls21 runs it on every map).

Diagnosis (sim --trace default_small, both seats, gst50 v ls21):
- Splitting is not the bottleneck: instrumented, dragons of length >= 4 before r400 that did
  not split were ~26 turns a match across 5 small maps (vs 80 splits); SplitSafe/cap rarely block.
- Deaths are exactly paired (kamikaze/lunge head-to-heads, ~21-31 a side) and the trapped
  L2-3 deaths are equal too. The losing side's total length goes flat (e.g. default_small
  gst50 as B: 20 -> 15 over r146-200 while ls21 22 -> 34): it stops eating, not splitting.
- All 5 small official maps went to the favoured seat in every pairing I ran (gst50 v ls21,
  gst52 v gst50, gst52 v ls21: 5-0-5 each). Here default_small went to A in both seats
  (KNOWLEDGE says B in self-play), so the seat depends on the pairing.

## Measured (sim, both seats)

| opponent | official | worldgen set 1 (20) | worldgen set 2 (20) |
|---|---|---|---|
| gather-split-trade-50 | 11-0-11, +0.8 total | 20-0-20, -2.3 | 21-0-19, +5.0 |
| late-surge-21 | 10-0-12, -1.5 | 23-0-17, +6.6 | 22-0-18, +12.1 |
| (parent gst50 v ls21) | 10-0-12, -6.3 | 22-0-18, +1.4 | 21-0-19, +5.2 |

Small official maps only, vs ls21: total margin -0.9 (gst50: -11.4); the unfavoured-seat
losses are smaller (Colosseum B win 102 vs 82, trophy B loss -136 vs -192, default_small B
loss -48 vs -63). The favoured seat still wins everything.

`check`: all passed; slowest turn 38 ms on stronghold (parent: 54.5 ms there; inherited from the parent's crowded
scan, not this change: the new code only runs on small maps).

## What should improve

Slightly better against trading swarms (ls21 +1 to +2 wins per set, +5..+7 total), even
against the parent. Not a fix for the seat effect.

## Didn't try / next

- The flat length of the losing side is an eating problem: check whether corpse pearls after a
  trade (the collision tile and every 2nd segment) are eaten by the passive side, and whether
  our goal scoring skips pearls next to enemy heads (Exposed) that are actually safe.
- om_dodge=0 (only the no-even-trades rule) was not measured separately.
