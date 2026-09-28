# late-surge-32

## Idea

late-surge-30 plus gather-split-trade-64's `split_announce`: when a dragon of length >= 10
trap-splits or doom-splits at least half its length off, it names itself at the new stub length
(not its old full length) in all four directions that same turn, and starts relaying its own
obituary immediately. Before, the split turn's ping still carried the old full-length report,
so the child (born right behind the parent's tail, hearing that ping first) and neighbours took
the stub's fresh report as champion and kept feeding it after it died (`league/KNOWLEDGE.md`
"Ghost champions"). Growth splits are excluded (they are routine and their obituaries would eat
sonar turns for no benefit, per gst64's own measurement).

## What changed from the parents, and why

- Namespace `ls30` -> `ls32`, macros `LS30_*` -> `LS32_*`, files `ls30.{cpp,hpp}` -> `ls32.{cpp,hpp}`
  (renamed first so this variant can't clash with either parent when loaded into one simulator).
- New param `split_announce` (1 by default, 0 under `LS32_OFF`), in `strategy.hpp` beside
  `obit_champion`/`obit_rounds`.
- In `Turn()`, right after the champion/obituary sonar pings (before the portal-probe pings,
  keeping late-surge's ping order and trader bit untouched): on a split action whose reason
  starts with `t` or `d` (`"trapped: split"`, `"doomed: split"` — never `"split"`, the growth
  case) that sends at least half our current length off, and only once we've been known at
  least `obit_champion` rounds, set `reports_[self]` and `obit_` to (self, head, new stub
  length, this round), start the obituary relay (`obit_until_`), and ping that report in all
  four directions this turn. This is a verbatim port of gst64's block onto late-surge's `Report`/
  `reason_`/`obit_` machinery, which late-surge already shares with the gst line.

## Measured

`build/tools/check late-surge-32 maps/official`: all checks passed, 0 avoidable deaths, slowest
turn 0.1-0.2ms (noise from a busy machine showed up to 7.8ms on one run but vanished on rerun,
and matched late-surge-30's own noise, so it isn't from this change).

`sim --threads 2`, both seats, vs late-surge-30 (the parent, this change's control since the
port only touches the split-turn ping):
| opponent | maps | W-D-L | longest margin | total margin |
|---|---|---|---|---|
| late-surge-30 | official (22) | 11-0-11 | +0.9 | +1.2 |
| late-surge-30 | worldgen, one fresh batch of 20 (40 seats) | 21-0-19 | +2.4 | -3.8 |

At least even on both sets, slightly ahead on longest (the metric that decides matches) on both,
consistent with gst64's own finding and the body-wall-24 port's (32-30 vs parent, 49-13 vs
trading-swarm-5). Did not additionally re-run vs trading-swarm-5 given the 20-minute budget;
gst64's and body-wall-24's numbers against it, plus late-surge-30's already-established
strength, made that check lower priority than confirming no regression against the parent.

## What should improve

Fewer of late-surge's dragons should keep naming (and feeding) a dead trapped-split stub as
champion in the mid-late game, the same effect gst64 measured (live-champion share at r450 rose
67% -> 83%, longest-dragon share of team length at r499 rose .32 -> .385). Not independently
re-measured here (would need the CDBG probe build, which the budget didn't cover); relying on
gst64's and body-wall-24's numbers plus the win-rate check above.

## What I tried that didn't work / skipped

Nothing else attempted — the brief asked for exactly this one port, and the numbers already
clear "at least even," so no parameter sweep or extra measurement beyond check + two sims.
