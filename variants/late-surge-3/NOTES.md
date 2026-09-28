# late-surge-3

## Idea

late-surge-1 (quiet swarm, one champion from round 380) plus gather-split-trade-20's cap
feeding: at the 64-dragon unit cap, before the surge, a worker of length <= 3 that sees a
longer ally's head within 2 steps dies next to it ("cap: feed ally"). The ally eats its pearls
and the freed slot goes to a productive split.

## What changed from the parents, and why

- Namespace/macros ls1/LS1 -> ls3/LS3 (files ls3.hpp/ls3.cpp).
- New step 2c in Decide (after kamikaze/lunge, before survival/growth), ported verbatim from
  gst20 (diffed against gst15: that is the only logic change). Params `cap_feed_max_length = 3`,
  `cap_feed_slack = 0`. Gated on `round_ < feed_from_round`, so it stops when the surge starts.
- Surge round unchanged at 380 (swept 380/390/400, see below).

## Measured (native sim, both sides of every map; machine load ~100, so timing figures from
`check` are noisy for every variant, parent included)

| opponent | maps | surge 380 (kept) | 390 | 400 |
|---|---|---|---|---|
| late-surge-1 | official (11) | 11-0-11 (+0.0 longest, +9.5 total) | 9-0-13 (-2.1) | 13-0-9 (-0.5) |
| late-surge-1 | worldgen, 20 fresh | 22-0-18 (+0.8, +12.9) | 20-0-20 (+4.0) | 21-0-19 (+1.1) |
| gather-split-trade-23 | official | 10-0-12 (+2.2, +34.7) | - | 9-0-13 (-4.8) |
| gather-split-trade-23 | same worldgen 20 | 24-0-16 (+7.1, +62.2) | - | 24-0-16 (+3.6) |

Parent on the same sets vs gst23: official 10-0-12 (-0.9 longest), worldgen 24-0-16 (+5.8).

Cap feeds: ~37 per match on official, ~127 on worldgen. Deaths per match on worldgen go up
(no action 425 vs 306) because of the feeds, but 'trapped: no move' is unchanged (258 vs 267):
the feeding does NOT reduce the trapped deaths the focus was aimed at; it only recycles slots.
Splits rise 345 -> 448 per match.

## What should improve

Slightly more mass funnelled to longer dragons pre-surge; margins vs gst23 a little better
(longest +2.2 vs -0.9 on official, +7.1 vs +5.8 on worldgen). All differences are within noise
at 22/40 matches.

## Tried, didn't work

- Surge at 390 or 400 with the cap feed: no clear change (390 slightly worse on official).
