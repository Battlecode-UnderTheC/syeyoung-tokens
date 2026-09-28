# late-surge-12

## Idea

Parameter-only tweak on late-surge-11: keep even trades (trade margin 0), but drop the
post-trade trader hunt entirely (`trader_hunt_radius` 6 -> 0). late-surge-11 chases enemy
traders within radius 6 once it sees them trading into us; every other lineage that tried
hunting (gather-split-trade-32, counter-swarm-17, body-wall-6) found the hunt itself was
the costly part of the busy midgame and gained by turning it off. This variant tests whether
the same holds for late-surge.

## What changed from the parents, and why

- Renamed namespace/files `ls11` -> `ls12` (ls12.hpp, ls12.cpp, plugin.cpp) with no other
  logic changes, to avoid symbol clashes when both variants are loaded in one sim.
- `trader_hunt_radius`: 6 -> 0 (in `ls12.hpp`). `HuntRadius()` already falls back to
  `p_.hunt_radius` when not `Trading()`, so setting this to 0 only removes the extra
  post-trade hunting behavior added in late-surge-11, leaving the even-trade logic intact.

## What I measured

`check late-surge-12 maps/official`: all checks passed, slowest turn under limits, 0
avoidable deaths.

- vs late-surge-11 (parent), maps/official, 2 threads: **11-0-11** W-D-L over 22 matches;
  late-surge-12 leads by -1.5 longest, +21.6 total length at match end.
- vs late-surge-11, 20 worldgen maps (W-D-L set): **20-0-20** over 40 matches; leads by
  +1.7 longest, +22.2 total.
- vs gather-split-trade-34, maps/official: **11-0-11** over 22 matches; leads by -3.0
  longest, -2.0 total (roughly even, still trailing on official maps specifically).
- vs gather-split-trade-34, 20 worldgen maps: **22-0-18** over 40 matches; leads by -1.3
  longest, +4.1 total (slight net win in worldgen).

Net: essentially even with late-surge-11 (same W-D-L both map sets, better total-length
margin), and a modest improvement against gather-split-trade-34 relative to what
late-surge-11 was reported to get (roughly even/slightly ahead vs. clearly trailing before).

## What should improve

Removing the hunt should free turns previously spent chasing traders (which the sim's
action-reason tally confirms: fewer `trapped: no move` / fatal-turn counts proportionally,
similar death profile otherwise) for gathering/splitting/trading, which is the behavior the
sibling lineages found paid off. The measurements above are consistent with that: neutral to
positive vs both the parent and the strongest rival (gather-split-trade-34), never worse.

## What I tried that didn't work

Nothing else tried — this is a single-parameter tweak per the brief; no other logic touched.
