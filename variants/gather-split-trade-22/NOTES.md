# gather-split-trade-22

Mutation (task g5-6) of gather-split-trade-19: an heir.

## Idea

From `heir_from_round`, the dragon the team's sonar champion word names (the longest ally
known, recomputed every turn) becomes the heir: it never splits (except the trapped split),
never trades (WorthTrading false, so no kamikaze, lunge or hunt, and every enemy-adjacent
tile counts as exposed), and just gathers. Everyone else keeps gst19's economy (splitting
until 300/350, feeding the champion from 340/440). If two dragons both think they are the
heir, the shorter learns by sonar and goes back to splitting.

The brief asked for an EARLY heir (round 150-200). Measured, early is worse; the kept
setting is 300 (so it is really "the champion stops splitting/trading from 300").

## What changed from the parents

- Namespace/macros `gst19`/`GST19` -> `gst22`/`GST22`.
- `Params::heir_from_round` (300), `heir_` flag set in Decide; gates on split, lunge,
  hunting, WorthTrading.
- Turn(): on rounds 300/400/499 the heir's reason is relabelled `heir@<round> <length bucket>`
  (a probe; one turn each, so it shows as 0 in the per-match tallies).

## Measured (native sim vs gather-split-trade-19, both sides; machine load ~100)

| heir round | official (11) | worldgen A (20) | worldgen B (20) |
|---|---|---|---|
| 150 | 10-0-12 (+1.6 longest) | 15-0-25 (-5.4) | |
| 175 | 10-0-12 (-1.1) | 18-0-22 (-0.7) | |
| 200 | 12-0-10 (+1.4) | 20-0-20 (+0.5) | |
| 250 | 13-0-9 (+2.2) | 20-0-20 (+1.2) | 18-0-22 (-1.0) |
| 300 | 11-0-11 (+2.3) | 23-0-17 (+3.5) | 23-0-17 (+2.9) |

Heir length (probe widened to 20-round windows, heir at 175): at round 300 most heirs are
still under 10 long (official ~85% of heir-turns <10; worldgen half <10, half 10-19); by 499
mostly 10-29. A lone gatherer grows ~0.05-0.1/round: 125 rounds of not splitting buys a
dragon under 10 long, while its lost splits cost the swarm pearls. Total length drops with an
earlier heir (-24 to -30 on worldgen at 150/175).

## What should improve

At 300: small edge at most (57-0-45 over 102 matches, longest +2 to +3.5): the champion stops
hunting/lunging/splitting a little earlier and is agreed on before feeding starts.

## Tried, didn't work

Heir from 150/175 (clearly worse on worldgen), 200/250 (even).
