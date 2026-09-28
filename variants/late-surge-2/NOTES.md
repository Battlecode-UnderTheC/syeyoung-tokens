# late-surge-2

## Idea

late-surge-1's quiet swarm and round-380 merge into one champion, plus one switch: in a crowded
game, trade like the league's trading swarms. There, workers trade evenly (trade_margin 0) and
hunt enemy heads within 6 steps, as gather-split-trade-23 does. Outside a crowded game, play is
the same as late-surge-1.

## What changed from the parents, and why

- Namespace and macros renamed `ls1` -> `ls2` (files ls2.hpp/ls2.cpp).
- A dragon marks the game crowded, for good, when any of these holds:
  - the map has at most 512 tiles (`crowded_area`: arena, Colosseum, default_small, devil,
    16x32 worldgen);
  - before round 300, it has seen at least 3 distinct enemy heads in the last 10 rounds
    (`crowded_heads`, `crowded_window`, `crowded_until`);
  - an ally's sonar word carries the crowded flag.
- The flag is bit 0 of the sonar word, which was always 0 before. Decode now accepts it.
  Children start with no memory, so the flag spreads to them by sonar.
- When crowded, the helpers `TradeMargin()` and `HuntRadius()` return
  `crowded_trade_margin` (0) and `crowded_hunt_radius` (6) instead of 1 and 0. The lunge and
  the kamikaze use the same WorthTrading, so they also take even trades. Leaders and the
  outnumbered rule are unchanged.

## Measured

`sim`, both sides of each map. Worldgen = 20 fresh maps (`--batch 20`), the same set for every row.

| opponent | maps | late-surge-2 | late-surge-1 (same maps) |
|---|---|---|---|
| late-surge-1 | official | 12-0-10 | |
| late-surge-1 | worldgen | 22-0-18 (longest 0.0, total -12) | |
| gather-split-trade-23 | official | 14-0-8 | 10-0-12 |
| gather-split-trade-23 | worldgen | 26-0-14 (+7.8 / +110) | 24-0-16 (+6.7 / +82) |
| counter-swarm-5 | official | 9-0-13 | 13-0-9 |
| counter-swarm-5 | worldgen | 27-0-13 (+8.6 / +94) | 30-0-10 (+13.5 / +112) |

Small official maps (A / B sides):

| map | vs gst23 | vs cs5 |
|---|---|---|
| arena | 1-1 (late-surge-1: 1-1) | 1-1 (late-surge-1: 1-1) |
| Colosseum | 1-1 (1-1) | 0-2 (2-0) |
| default_small | 1-1 (2-0) | 0-2 (1-1) |
| devil | 2-0 (0-2) | 1-1 (1-1) |

Area-only trigger (`crowded_heads` off), same maps: vs gst23 11-11 official and 24-16 worldgen;
vs cs5 10-12 official and 31-9 worldgen; vs late-surge-1 11-11 official and 20-20 worldgen. It
is close to late-surge-1 everywhere, so the gains and losses come mostly from the head-count
trigger.

## What should improve

Against the trading swarms (gst23), devil flips to 2-0, and the worldgen margins grow.
Against counter-swarm-5 this is worse: on its home ground (Colosseum, default_small) even
trading loses to it, and the quiet parent beat it there. Overall, against gst23 and cs5
combined, it is about even with the parent: 23-21 official for both, 53-27 against 54-26 on
worldgen. It beats the parent head to head, 12-10 and 22-18.

## What didn't work / open

- An area-only switch is neutral (see above).
- counter-swarm-5 punishes even trades on small maps. The switch could key on the opponent's
  style (do their dragons walk into our heads?) rather than on crowding alone.
- Small maps are nearly deterministic pairs of matches, so the single-map rows above are noise-prone.
