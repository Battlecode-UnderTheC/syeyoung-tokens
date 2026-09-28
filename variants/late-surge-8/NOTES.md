# late-surge-8

## Idea

Stack three late-surge-1 descendants' changes on late-surge-6: feed only a clearly longer
champion + trust_visible (ls6, the base), the opponent-behaviour trade switch (ls4), and the
late walk towards an unreachable champion (ls7).

## What changed from the parents, and why

- Base: late-surge-6 sources, namespace renamed `ls8` / `LS8_*`.
- From late-surge-4: `WatchTrades` (a head-to-head seen next to us, or 3 enemy heads stepping
  next to ours, before the surge) sets `trader_`; while `trader_` and not outnumbered, trade
  evenly (`trader_trade_margin` 0) and hunt at radius 6. The flag spreads by sonar bit 0.
  Sonar: ls6's word leaves bits 0..9 zero (the decode checked `w & 1023`); now bit 0 carries the
  flag and decode checks `w & 1022`. No collision with the other fields.
- From late-surge-7 (only its active change; regional feeding and big-map advance were off by
  default there): from round 440, a feeder >= 10 long with no known path to the champion walks
  to the known tile nearest the champion's reported position (the old seek_champion walk,
  which ls6 had removed). A feeder that ls6 "holds" (peer not clearly longer) has no target
  and doesn't walk.

## Measurements (sim, both sides, 11 official maps + 20 fresh worldgen maps)

| variant | vs late-surge-6 off / wg | vs gather-split-trade-31 off / wg |
|---|---|---|
| full stack (this) | 11-11 / 19-21 (30-32) | 9-13 / 19-21 (28-34) |
| without trade switch | 11-11 / 22-18 (33-29) | 10-12 / 12-28 (22-40) |
| without late walk | 11-11 / 18-22 | not measured |
| late-surge-6 itself | - | 11-11 / 12-28 (23-39) |

The trade switch costs a little against the quiet ls6 (it fires by mistake on approaches)
and gains a lot against a trading opponent (worldgen 19-21 against 12-28). Summed over both
opponents the full stack is best (58-66 against 55-69), so all three stay. The late walk is
close to neutral (removing it: 29-33 against ls6).

## What should improve

Against trading lines (gather-split-trade): the swarm trades back instead of bleeding.
Late endgame on big or unexplored maps: long feeders reach the champion.

## Not tried / next

- Tighten the approach trigger (3 approaches gives false positives against quiet opponents),
  for example approaches >= 5, or require one actual trade.
