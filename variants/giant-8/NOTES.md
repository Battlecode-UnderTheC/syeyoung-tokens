# giant-8

## Idea

Crossover giant-7 x gather-split-trade-34. It uses gst34's movement and goal layer: SafeStep
escape levels with TurnOut, "into a dead end worth it", the contest and share goal scoring,
sonar-checked crossing of unknown portals, and the loop breaker. On top of that sit the giant
roles: round-0 dragons split only until 100 and then only grow, and a child born length >= 8
(a giant split out of a trap) is a giant too. A surge reserve is added: helpers with
`id % 5 == 0` never feed the champion.

## What changed from the parents, and why

- Source base is gst34's `strategy.*`, renamed `giant8::GiantAgent` (and `GIANT8_*` macros). giant-7's
  own movement code is dropped. Its diagnosis was that the early mass gap is in how the dragons
  move and eat, so I replaced that layer outright instead of porting pieces into it.
- `leader_split_until` 300 -> 100: the giant role.
- `leader_ = round 0 || length >= giant_child_length (8)`: giant-7's heir rule.
- `reserve_mod = 5`: reserve helpers skip feeding and keep gathering, so a lunged king isn't a wipeout.
- The endgame keeps gst34's timing (surge at 380, helpers split until 400). giant-7's timing
  (surge 340, helpers split until 350) was the thing that lost. The first build used it and went
  33-69 against gst34 (official 10-12, worldgen 10-30 and 13-27). Changing only the timing back
  gave the numbers below.

## What I measured (sim, both seatings)

| opponent | official (22) | worldgen set 1 (40) | worldgen set 2 (40) |
|---|---|---|---|
| giant-7 | 15-7 (+1.5 longest, +142 total) | 31-9 (+5.1, +158) | 31-9 (+12.7, +130) |
| gst34 | 9-13 (-0.1, +3.7) | 21-19 (+0.1, +27.6) | 18-22 (-2.1, +38.3) |

Against gst34, reserve_mod=0 went 10-12 / 22-18 / 21-19: overall 53-49 against 48-54 with the
reserve. That is noise-level. With the reserve, total length is higher and the longest slightly
lower. I kept the reserve because the task asked for it.

Mean total length at r50 / r100 / r340 in traced matches (one seating, where the game lasted), against gst34:

| maps | giant-8 | its gst34 | giant-7 | its gst34 |
|---|---|---|---|---|
| official | 24 / 58 / 103 | 26 / 48 / 105 | 19 / 36 / 90 | 30 / 59 / 103 |
| worldgen 1 | 63 / 114 / 145 | 57 / 101 / 153 | 48 / 76 / 123 | 60 / 141 / 181 |

The early mass gap is closed: the giant line now eats as well as gst.

`check`: all checks passed, 0 avoidable deaths. The slowest turn is 7 ms on stronghold, which is
the same TurnOut cost as gst34 (22-51 ms in the same run, on a loaded machine).

## Didn't work

- giant-7's endgame (surge 340, helper splits until 350) on the new movement layer: 33-69 against gst34.
- gst34's full timing (leaders split until 300) with the heir rule and reserve: 26-36 against gst34
  (official 7-15, worldgen 19-21). The early-only giant splits are no worse, possibly better.

## Next

The giant roles are now roughly even with gst34. What's left to try: a larger or length-based
reserve, a king that avoids contact after 380, and whether a single giant (not 4) matters.
