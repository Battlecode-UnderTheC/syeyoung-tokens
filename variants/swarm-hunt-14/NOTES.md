# swarm-hunt-14

## Idea

Crossover swarm-hunt-13 x gather-split-trade-38, done the way giant-8 did it: the whole
movement and goal layer is replaced by gst38's (SafeStep escape levels + TurnOut, "into a
dead end worth it", contest/share goal scoring, patrol, split safety check, sonar-checked
portal crossing, loop breaker, partial-sighting champion fix), and swarm-hunt's roles sit on top.

## What changed from the parents, and why

- Source base is gst38's `strategy.*` renamed to `swh14::HuntAgent` (`SWH14_*` macros).
  swh13's own movement code (crowd_room, ally_yield, crowd_penalty, its SafeStep) is dropped.
  Both parents had the same partial-sighting fix; gst38's copy is kept.
- swarm-hunt identity kept: keepers = round-0 dragons with no echo on their first turn;
  keepers stay out of enemy strike range (`keeper_guard` 2, threat map in `Exposed`); trade
  rule "enemy at least as long" (`outnumbered_factor` 0, gst's outnumbered rule off: setting
  it to 2 changed nothing); strikes and lunges; the unit-cap feed (short hunter near the cap
  resigns next to a longer ally, now its own `cap_feed_slots` 6); swh13's endgame (same
  timings as gst38: surge 380, keepers stop splitting 300, champion freeze 330).
- MirrorAgent for team B, ported from swh13 (it already mirrors hedge/vedge and the sonar
  directions, so gst's portal ping works in both seats; stronghold/trauma give identical
  results in both seatings).
- `hunt_radius` 0 (swh13: 6) and `split_headroom` 0 (swh13's unit_reserve 6): both cost growth.

## Measured (sim, both seats; wg1 = worldgen seed 1212, wg2 = seed 8808, 20 maps each)

W-D-L by config (official 22 / wg1 40 / wg2 40, total):

| config | vs swarm-hunt-13 | vs gst38 |
|---|---|---|
| hunt 6, headroom 6 (first build) | 13-9 / 22-18 / 22-18 = 57-45 | 6-16 / 16-24 / 15-25 = 37-65 |
| hunt 0, headroom 6 | 12-10 / 26-14 / 24-16 = 62-40 | 11-11 / 12-28 / 23-17 = 46-56 |
| hunt 6, headroom 0 | 12-10 / 24-16 / 24-16 = 60-42 | 7-15 / 18-22 / 18-22 = 43-59 |
| **hunt 0, headroom 0 (committed)** | 11-11 / 28-12 / 24-16 = **63-39** | 10-12 / 19-21 / 19-21 = **48-54** |

Reference: swarm-hunt-13 vs gst38 = 10-12 / 23-17 / 21-19 = 54-48 (non-transitive).

Mean team total length at r50 / r100 / r340 (traced, swh14 as A):

| maps, opponent | swh14 (hunt 0) | opponent | swh14 (hunt 6) | opponent |
|---|---|---|---|---|
| official, swh13 | 29 / 61 / 108 | 20 / 40 / 69 | 30 / 54 / 93 | 22 / 37 / 91 |
| wg1, swh13 | 55 / 94 / 135 | 45 / 82 / 123 | 54 / 96 / 129 | 46 / 92 / 136 |
| official, gst38 | 25 / 48 / 85 | 25 / 48 / 105 | | |
| wg1, gst38 | 55 / 106 / 125 | 50 / 90 / 141 | | |

The early growth gap against swarm-hunt-13 is closed (and against gst38 it is even or ahead to
r100); hunting heads (radius 6) costs total by r340 here too. Against gst38 we fall behind
between r100 and r340.

`check`: all checks passed, 0 avoidable deaths. Slowest turn 8-21 ms (TurnOut, the same cost
as gst38/giant-8; machine loaded).

## What should improve

Growth and total length over swarm-hunt-13 (63-39). Against gst38 near even (48-54).

## Next

Why the r100-r340 total falls behind gst38: the remaining differences are keeper_guard 2,
the cap feed, the keeper rule, and gst's lunge exclusion; try each off against gst38.
