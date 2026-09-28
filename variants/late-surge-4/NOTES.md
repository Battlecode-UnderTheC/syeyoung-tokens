# late-surge-4

## Idea

late-surge-1 (quiet swarm, surge into one champion from round 380), but workers switch to
even trades (trade margin 0, hunt radius 6) only when the enemy is itself trading into us and
we are not outnumbered (UNIT_COUNT >= distinct enemy ids seen in the last 60 rounds).

## What changed from the parents, and why

- `WatchTrades` (every turn, before round 380): a head-to-head is "seen" when last turn an
  ally head and an enemy head were adjacent, well inside our window (Chebyshev <= 2), and
  both ids are gone now. An "approach" is an enemy head that moved and is now next to one of
  our heads when it was next to none before. 1 trade or 3 approaches set the flag `trader_`.
- The flag is sticky and shared by sonar, bit 0 of the champion word (as late-surge-2 did).
- `Trading()` = flag && not outnumbered; it picks trade margin 0 / hunt radius 6, else
  late-surge-1's 1 / 0. Leaders unchanged (they trade only for strictly longer enemies).
- late-surge-2's switch was map size / enemy density, which also fired against
  counter-swarm (which punishes even trades). This one reacts to what the enemy does.

## Measurements (sim, 2 threads; official = 11 maps x 2 sides, worldgen = 20 fresh maps x 2)

| opponent | ls4 official | ls1 official | ls4 small* | ls1 small* | ls4 worldgen | ls1 worldgen |
|---|---|---|---|---|---|---|
| late-surge-1 | 10-12 | - | 4-4 | - | 21-19 | - |
| gather-split-trade-23 | 10-12 | 10-12 | 4-4 | 5-3 | 24-16 | 22-18 |
| counter-swarm-12 | 14-8 | 11-11 | 5-3 | 5-3 | 23-17 | 22-18 |

*small = Colosseum, arena, default_small, trophy (8 games).

Kamikaze deaths per official match went 4.5 (ls1) -> 12.5, so the flag does fire. Slowest
turn in `check` 0.8 ms on stronghold under machine load (others 0.1-0.3 ms).

## What should improve

Slightly better vs counter-swarm (official) and on worldgen; the gst23 small-map problem is
NOT fixed (4-4 vs ls1's 5-3 there; both 10-12 official). All differences are within noise.

## Next

The small crowded maps are decided in the first ~100 rounds; the detector needs a trade seen
in view first, so it may fire too late there. Try firing on the first approach, or trading
evenly from round 0 only on maps <= 512 tiles when trades are seen.
