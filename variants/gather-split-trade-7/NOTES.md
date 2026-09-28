# gather-split-trade-7

## Idea

Tweak of gather-split-trade-4 (renamed namespace gst4 -> gst7): lower `feed_from_round`,
the round after which non-leader workers stop pursuing their own growth and instead walk in
to feed the sonar-agreed team champion. Earlier work on this lineage found 400 -> 360 helped;
a sibling lineage found 340 -> 300 helped slightly with 260 clearly worse. This tries pulling
the switch earlier still, testing both 320 and 340.

## What changed from the parents, and why

Only `Params::feed_from_round` in `strategy.hpp`, nothing else. Tried both 320 and 340 as the
brief allowed, kept **340** (see measurements below) since it beat 320 in aggregate win count
and aggregate longest-margin across official + worldgen maps, while both individually stayed
at or above break-even against the parent (360).

## Measurements (vs gather-split-trade-4, threads 2)

feed_from_round = 320:
- official maps (11 maps, both sides): 14-0-8 W-D-L, longest margin +7.9, total length margin -12.3
- 20 worldgen maps (build/practice, both sides, 40 matches): 19-0-21 W-D-L, longest margin -3.9, total margin -27.7
- combined: 33-0-29 W-D-L (53.2%), weighted longest margin ~ +0.29

feed_from_round = 340 (kept):
- official maps: 12-0-10 W-D-L, longest margin +0.8, total margin -9.9
- 20 worldgen maps: 23-0-17 W-D-L, longest margin +0.3, total margin -15.3
- combined: 35-0-27 W-D-L (56.5%), weighted longest margin ~ +0.48

Both settings are at least even with the parent; 340 was the better of the two on both
win-rate and longest-dragon margin when official and worldgen results are combined, so it
was kept. `check gather-split-trade-7 maps/official`: all checks passed, 0 avoidable deaths,
slowest turn well under budget.

## What should improve

Feeding the champion slightly earlier gives the team's eventual longest dragon a few more
rounds of extra growth from teammates before the 500-round cutoff, at a small cost in total
team length (workers die earlier instead of continuing to forage). Net effect over the
parent is a modest positive on both win rate and longest-dragon margin, consistent with the
trend already seen at 400->360 and 340->300 on the sibling lineage: earlier feeding, within
this range, tends to help; the KNOWLEDGE.md report of 260 being clearly worse suggests there
is a floor somewhere below 320 where switching too early costs too much total growth.

## What didn't work / wasn't tried

Did not try feed_from_round below 320 (out of budget); the sibling lineage's finding that
260 was clearly worse suggests not to push this parameter much lower without measuring.
No other parameters were touched.
