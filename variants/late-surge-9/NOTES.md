# late-surge-9

## Idea

late-surge-8 (late-surge-6 + reactive trading + late walk) with a stricter trigger for the
reactive "trader" mode, so the flag fires less often against enemies that don't really trade.

## What changed from the parents, and why

- The namespace is renamed from `ls8` to `ls9` (files `ls9.cpp`/`ls9.hpp`, macros `LS9_*`).
- `trader_approaches` goes from 3 to 5. The flag is now set by one head-to-head trade seen
  (both heads gone, next to each other, close to us), unchanged, or by 5 enemy heads stepping next to
  ours (was 3). Quiet enemies brush past our heads while they feed. At 3 they set the flag and
  pulled us into even trades and radius-6 hunts. Everything else is as in late-surge-8, including
  the sonar bit 0 spread.

## Measured (sim, both sides of each map; W-D-L)

| opponent | official (22) | worldgen set A (40) | worldgen set B (40) | total |
|---|---|---|---|---|
| late-surge-8 (ls9 vs) | 12-0-10 | 19-0-21 | 20-0-20 | 51-51 |
| late-surge-6: ls9 | 14-0-8 | 23-0-17 | 22-0-18 | 59-43 |
| late-surge-6: ls8 (baseline) | 11-0-11 | 24-0-16 | 21-0-19 | 56-46 |
| gather-split-trade-31: ls9 | 12-0-10 | 19-0-21 | 19-0-21 | 50-52 |
| gather-split-trade-31: ls8 (baseline) | 9-0-13 | 17-0-23 | 16-0-24 | 42-60 |

check: all checks passed.

## What should improve

Slightly better than late-surge-8 against both the quiet (late-surge-6) and the trading
(gather-split-trade-31) opponents. Against gather-split-trade-31 the stricter trigger even helped.
The early flag, set by approaches alone, seems to have made us trade while outnumbered in view.
Head-to-head against late-surge-8 it is even.

## Tried / not tried

Only the one setting (5). Not tried: trades only (approaches off), or needing both a trade and approaches.
