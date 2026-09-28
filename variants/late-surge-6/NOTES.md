# late-surge-6

## Idea

late-surge-1 with one rule: a dragon only feeds (resigns next to) the sonar champion when the
champion is clearly longer than itself, by its length as known this turn. A near-peer or a
long dragon that would otherwise halve itself into a stub or a tie keeps gathering instead.

## What changed from the parents, and why

- Namespace `ls1` -> `ls6` (files ls6.hpp/ls6.cpp, macro `LS6_DEBUG`).
- `trust_visible` (from late-surge-5, g7-4): an ally whose whole body is in view (its last
  visible segment is not on the window border) has exactly its visible length, instead of the
  max of the old report and what is seen. This kills the "stub ghost" (a champion that split
  out of a trap leaves a 2-long stub under its id that still counts as the old length).
- `feed_clear_tenths = 8`: at the moment of feeding (champion in view), feed only if
  `our length < 0.8 x target length`; otherwise drop the feed target this turn and gather
  normally (reason `held: peer not clearly longer`). The champion choice already breaks exact
  ties by lower id.

## Measurements (build/tools/sim, 2 threads)

Measurement in the parent's logic first (feeds tagged by feeder/target ratio): feeds into a
target not clearly longer (feeder >= 80% of target) were rare but real: official 1.1/match
(all feeders < 10 long); worldgen 2.6/match, of which 0.3 by length 10-19 and 0.2 by length
>= 20 dragons (those are the costly ones).

| a vs b | official (22) | worldgen, 20 maps (40) |
|---|---|---|
| late-surge-6 vs late-surge-1 | 14-0-8, longest +2.4 | 23-0-17, longest +1.2 |
| trust_visible only (clear rule off) vs late-surge-1 | 9-0-13, longest -9.7 | 20-0-20, longest -5.5 |
| late-surge-6 vs gather-split-trade-27 | 9-0-13, longest -3.5 | 23-0-17, longest +3.8 |
| late-surge-1 vs gather-split-trade-27 | 8-0-14, longest -3.5 | 23-0-17, longest +4.3 |

`held` fires ~11 (official) / ~14 (worldgen) dragon-turns in total per sim set; feeds per
match fall only slightly (16 vs 18 official). check: all checks passed.

## What should improve

Fewer late self-inflicted losses of the second-longest dragon; slightly better longest margin.
The effect is small and within noise on 62 matches; the clear rule is what carries it
(trust_visible alone looked worse, also within noise).

## Tried, didn't work

- trust_visible alone (without the clear rule): 9-13 / 20-20 vs late-surge-1.
