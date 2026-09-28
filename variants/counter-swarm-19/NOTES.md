# counter-swarm-19

## Idea

Crossover counter-swarm-17 x gather-split-trade-34, done the way giant-8 did it. The whole
movement and goal layer is gst34's: SafeStep with escape levels and TurnOut, "into a dead end
worth it", contest/share goal scoring, patrol, sonar-checked crossing of unknown portals, and
the loop breaker. Counter-swarm's identity sits on top: multi-step strikes (the longest enemy
head worth a trade within 5 steps, and the champion strikes too), its trade rules, hunting
cut off at 300, and the champion guard from 340.

## What changed from the parents, and why

- The source base is gst34's `strategy.*`, renamed `cs19` (`CS19_*` macros). cs17's own
  movement, goal and step code is dropped.
- gst34's Lunge is replaced by cs17's StrikePath. It prefers the longest target at the
  nearest depth and doesn't exempt the late champion.
- The champion guard comes from cs17 (MarkThreats, plus the guarded filter in SafeStep).
  Sightings carry `cut` (any segment on the window's edge) for the guard's extra reach.
- `hunt_until = 300` from cs17. It has no effect while hunt_radius = 0, the same as in both parents.
- Endgame timing: both parents already share it (split until 400/300, champion freeze 330,
  feed 380, leader feed 380). The only timing difference was cs17's guard at 340, which is kept.
- cs17's "seek champion" (a feeder whose champion's report is unreachable heads for the known
  tile nearest to it) is ported but **off** (`seek_champion = 0`). With it on, the final total
  length was 43-65 below gst34's; with it off the gap is gone (see below).

## What I measured (sim, both seatings; official = 22 matches, worldgen = 40 each)

Final config (seek_champion 0):

| opponent | official | worldgen set 1 | worldgen set 2 (seed 777) |
|---|---|---|---|
| counter-swarm-17 | 11-11 (-0.8 longest, +72 total) | 25-15 (+9.7, +128) | 18-22 (-0.3, +66) |
| gather-split-trade-34 | 11-11 (0.0, +5.8) | 19-21 (+0.7, -1.0) | 21-19 (+0.9, -9.4) |

In total: 54-48 against cs17 and 51-51 against gst34.

With seek_champion 1, the numbers against cs17 were 13-9 / 28-12 / 15-25 (56-46), and against
gst34 13-9 / 15-25 / 21-19 (49-53), with the total 43-65 behind gst34 on every set. With the
guard off as well (seek on), gst34 went 11-11 / 17-23 / 21-19: the guard was not the cause.

Mean total length at r50 / r100 / r340 in traced matches (cs19 or cs17 seated as A, where the game lasted):

| maps | cs19 | its opponent gst34 | cs17 | its opponent gst34 |
|---|---|---|---|---|
| official | 24 / 58 / 133 | 26 / 48 / 106 | 24 / 46 / 71 | 27 / 54 / 101 |
| worldgen 1 | 62 / 109 / 156 | 64 / 101 / 147 | 62 / 93 / 134 | 66 / 109 / 156 |

Against cs17 directly on worldgen 1: 64 / 122 / 172 against 60 / 86 / 119. The mid-game growth
gap is closed: counter-swarm now grows as fast as gst34, or faster.

`check`: all checks passed, 0 avoidable deaths. The slowest turn varies with machine load, from
0.1 to 72 ms on big maps, from gst34's TurnOut. That is the same cost as gst34 and giant-8.

## What should improve

Mid-game mass, and with it the league results against the main line. Against gst34 it is now
even in wins and total. Before this change cs17 was 20-30 total length behind by r340.

## Didn't work

- seek_champion on the new movement layer: it loses total length in the surge. The feeders
  probably wander toward stale reports instead of gathering or finding the champion by sight.

## Next

The two lines are now even, so what's left is the counter-swarm identity itself. Next I'd try the
strike versus gst34's lunge in isolation (the champion striking late may cost it), and
guard_from/guard_share.
