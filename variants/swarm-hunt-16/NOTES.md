# swarm-hunt-16

## Idea

swarm-hunt-15 with gather-split-trade-39's obituaries ported in: when a known ally of length
>= 10 gets an exact, newer report cutting it to less than half (a trap-split stub keeps the
id, the child carries the mass under a new id) or is seen dying (an ally of length >= 5 beside
our head last turn, gone now, while we moved a plain step), the dragon that learns it pings
the correction on odd rounds for 20 rounds (the champion still goes out on even rounds). Also
ported gst39's `heard_split`: growth splits try n = L/2, L/2+1, L/2-1 for one where the body
runs straight through the cut (so the tail ping, opposite the facing, reaches the child's
tail on its first turn); trapped splits of a >= 8-long dragon keep one extra segment if that
makes the cut straight.

## What changed from the parents, and why

swarm-hunt-15 (base, namespace swh15 -> swh16) already carries gather-split-trade-38's
movement/goal layer (partial-sighting `lower_bound_reports`, late-report fade) and its own
keeper/hunter role split and sonar champion-relay. It did not have gst39's obituary or
heard-split fix, so late games likely still had dragons walking to a dead or demoted
"champion" the way gst38 did before gst39 (the brief's diagnosis: swh15 leads gst38 in total
length at r340 on official maps, 101 vs 94, but that's 50-52 in W-L; a leaking endgame).

Ported directly, keeping gst39's design since it was already measured there:
- `Report obit_`, `obit_until_`, `prev_head_` state; death detection and the self-split-cut
  check added to `Remember()`; the length-halving check added to `Note()` (this covers reports
  arriving both from direct sighting and from relay).
- `HeardSplit(n)` / `GrowthSplit()`: growth splits now pick a straight cut when one of
  L/2, L/2+1, L/2-1 is straight and safe; the trapped-split path keeps one extra segment on a
  >= 8-long dragon when that makes the cut straight.
- `Turn()`: sonar sends the obituary instead of the champion on odd rounds while
  `obit_until_ > round_`.
- Namespace/macros swh15 -> swh16 throughout (required rename, done first).
- Did not need extra sonar bits: the correction is encoded with the same `Report` word as the
  champion. The word has 9 unused bits above magic/team/id/x/y/length/round/lower, so there's
  room later for a heartbeat bit if a champion needs to signal "alive but far" (gst39's own
  next-step note).

## Measured (sim vs swarm-hunt-15, both seats)

| maps | W-D-L | longest margin | total margin |
|---|---|---|---|
| official (22 matches) | 12-0-10 | +2.3 | +15.0 |
| worldgen (20 maps, 40 matches, default seed) | 24-0-16 | +1.7 | +23.9 |

`check swarm-hunt-16 maps/official`: all checks passed, slowest turn 1.8 ms (big_empty; the
parent's own slowest turn on the same map is 45.4 ms, so this is not a regression -- board
memory work dwarfs the extra sonar/obituary bookkeeping).

Did not measure vs gather-split-trade-39 directly (time budget); the brief's own gst39-vs-gst38
numbers (55-47, dead-champion naming 43% -> 24%) are the evidence the mechanism works there.
The numbers above show it holds up on top of swarm-hunt-15's different movement/role layer too:
both W-L and total-length margins move the right way on both map sets.

## What should improve

Fewer dragons walking the endgame to a dead or demoted "champion"; both official and worldgen
results move the right way on W-L and total length versus the parent.

## What I tried that didn't work / next

Did not tune obit_rounds/obit_min_length/obit_champion (kept gst39's measured values) -- no
time in this loop's budget to sweep them for swarm-hunt's different role layer, a candidate for
a later task. gst39's own next-step note stands: death detection is narrow (only an ally beside
our head); a champion not heard from in a while could be demoted faster once a newer,
nearly-as-long report of someone else arrives, without needing a heartbeat bit.
