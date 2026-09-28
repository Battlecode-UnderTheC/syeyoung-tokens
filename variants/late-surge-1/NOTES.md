# late-surge-1

## Idea

Stay many, small and quiet for most of the game, then merge everything into one dragon at the
end. Workers split at length 4 until round 400 and never trade evenly: no hunting, and the
kamikaze and the lunge only hit an enemy head at least one longer than us. From round 380 every
dragon, leaders included, walks to the sonar-agreed champion and dies within 3 steps of its head.
The champion stops splitting from round 330 so the merge target stays put.

## What changed from the parents, and why

Fresh, but the code starts from gather-split-trade-23 (sonar champion scored by
`length - 0.5 * age`, room/escape checks, trap splits). Namespace `ls1`; the getenv tuning
block is gone. Changes (all in `Params` plus three guards in ls1.cpp):

- hunt_radius 6 -> 0 (the hunting code is guarded by `hunt_radius > 0`), trade_margin 0 -> 1
  (trade only for a longer enemy).
- Lunge gated by `use_lunge` (1). It uses the same strict WorthTrading, so it only strikes
  longer enemies.
- The surge: feed_from_round 340 -> 380, leader_feed_from 440 -> 380 (leaders merge too),
  leader_feed_fresh 10 -> 20, feed_distance 2 -> 3.
- champ_freeze 330: whoever the champion is (by the sonar score) stops splitting.
- split_until_round 350 -> 400: workers keep splitting right up to the surge.

## Measured

`sim`, both sides of each map. Worldgen A = 20 maps from seed 818, B = seed 919.

| opponent | maps | W-D-L | longest / total margin |
|---|---|---|---|
| gather-split-trade-23 | official | 10-0-12 | -0.9 / +24.8 |
| gather-split-trade-23 | worldgen A | 25-0-15 | +8.6 / +97 |
| gather-split-trade-23 | worldgen B | 28-0-12 | +12.2 / +96 |
| counter-swarm-5 | official | 13-0-9 | +1.0 / +60 |
| counter-swarm-5 | worldgen A | 26-0-14 | +14.7 / +128 |
| anchor-greedy | official / worldgen A | 21-0-1 / 40-0-0 | |
| anchor-random | official / worldgen A | 22-0-0 / 40-0-0 | |

Surge timing vs gst23, before the lunge was turned back on (feed_distance 2; worldgen A / official):
350: 17-23 / 11-11, 380: 24-16 / 9-13, 400: 22-18 / 9-13, 420: 21-19 / 9-13, 450: 20-20 / 9-13.
380–400 is best; 350 is too early (a dragon walking to the champion stops gathering).
feed_distance 3 more than doubled the feeds (19 -> 45 a match on worldgen) at about the same
W-L (22-18), and raised the longest margin.
Lunges at longer enemies only: 22-18 -> 25-15 worldgen, 8-14 -> 10-12 official.

## What should improve

On worldgen maps, total length (+100) and longest dragon beat gst23 and cs5: the swarm loses less
to even trades and the surge moves more of that mass into the champion. On small crowded official
maps (arena, Colosseum, devil) the quiet swarm loses the attrition to the lunging swarms and gets
eliminated before round 260.

## What didn't work / open

- Surging from 350 or earlier (see above).
- No lunges at all: 3 fewer wins in 40 on worldgen.
- `trapped: no move` is still ~340 deaths a match on worldgen (gst23: ~270): more dragons alive at
  the unit cap. Many of them could feed a nearby longer ally instead of resigning.
