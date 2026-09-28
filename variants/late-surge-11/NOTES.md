# late-surge-11

## Idea

Combine the two late-surge-8 improvements. From late-surge-10: unknown portals get probed with
sonar and crossed when the ping comes back clean. From late-surge-9: the reactive "trader" flag
needs 5 enemy approaches instead of 3.

## What changed from the parents, and why

- Base: late-surge-10's sources. Namespace `ls10` -> `ls11` (files `ls11.*`, macros `LS11_*`).
- From late-surge-9: its only code change relative to late-surge-8 (checked by diffing
  ls8 against ls9 with the namespaces normalised): `trader_approaches` 3 -> 5 in `ls11.hpp`.
  One head-to-head trade seen still sets the flag, as before. Quiet enemies brush past our
  heads while they feed, and at 3 approaches that set the flag by mistake.
- Portal probe/cross is as in late-surge-10, with the loop breaker left off (`loop_ban` 0).

## Measured (sim, --threads 2, both sides of each map; W-D-L)

Official = 11 official maps (22 matches). A and B are 2 fresh worldgen sets of 20 maps each
(40 matches each).

| opponent | official | worldgen A | worldgen B | total |
|---|---|---|---|---|
| late-surge-10 | 15-0-7 | 20-0-20 | 19-0-21 | 54-0-48 |
| late-surge-9 | 10-0-12 | 21-0-19 | 26-0-14 | 57-0-45 |
| gather-split-trade-34 | 10-0-12 | 19-0-21 | 17-1-22 | 46-1-55 |
| (baseline: late-surge-10 vs gst-34) | 9-0-13 | 17-0-23 | 19-1-20 | 45-1-56 |

check: all checks passed (official maps).

## What should improve

It should be even with or slightly ahead of both parents. It gains the most against late-surge-9,
which suggests the portal crossing carries over. Against gather-split-trade-34 it is only as
good as late-surge-10, since it doesn't change anything about trading lines.

## Tried / not tried

This is only the crossover; no parameters were swept. Next: something aimed at gather-split-trade-34,
which still leads the late-surge line 55-46 here.
