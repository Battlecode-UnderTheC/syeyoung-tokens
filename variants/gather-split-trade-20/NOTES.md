# gather-split-trade-20

## Idea

Unit-cap management for gather-split-trade-15. At UNIT_LIMIT (64) nobody can split, neither a
growth split nor a trapped escape. A short worker (length 2-3, too short to split anyway) that is
at the cap and has a longer ally's head within 2 steps dies on the spot ("cap: feed ally"): the
ally eats its pearls and the freed slot goes to a productive split. Only before feed_from_round
(340), only workers, only when the ally is seen this turn.

## What changed from the parents, and why

- Namespace gst15 -> gst20.
- New step 2c in Decide (after kamikaze/lunge, before growth splits), params
  `cap_feed_max_length = 3`, `cap_feed_slack = 0`.

## Measurements (instrumented build, reasons tagged by UNIT_COUNT band, vs gst15)

The cap is hit a lot. Share of our dragon-turns with UNIT_COUNT = 64: ~26% on official maps,
~30% on worldgen. Trapped "no move" deaths per match:
- official: ~43 total; ~4 at the cap (2.5 of them length >= 4, i.e. would have split out).
- worldgen (20 maps): ~305 total; 58 at the cap with length >= 4 (cap-caused), 39 at the cap
  short, 28 at 60-63. So ~1/3 of worldgen trapped deaths happen at or near the cap.

Results vs gather-split-trade-15 (W-D-L):
- split_headroom = 3 (reserve slots for trapped escapes): official 13-0-9, worldgen 19-0-21.
  Trapped deaths went UP (377 vs 305): the churn of escape splits just makes more stubs.
  Neutral (as gather-split-trade-1 found). Not kept.
- cap feed (this variant): official 13-0-9 (+3.5 longest, +11 total), worldgen batch 1
  19-0-21 (+0.1 longest), fresh worldgen batch 2 25-0-15 (+4.7 longest). 180 cap feeds a match
  on worldgen, 31 on official. Trapped deaths unchanged.

## What should improve

Longest dragon: the slot churn turns idle 2-3 length workers into mass for bigger allies and
lets 4+ dragons keep splitting. Death counts rise (the feeds), but deaths at the cap are cheap.

## Tried, didn't work

- split_headroom 3 (above).
