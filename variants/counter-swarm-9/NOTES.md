# counter-swarm-9

## Idea

A parameter-only tweak of counter-swarm-5: start feeding the sonar-agreed
champion earlier, on the theory (from counter-swarm-1's history, where
340 → 300 went 35-27 in sims and 260 was clearly worse) that consolidating
length onto the champion sooner outraces the length-based scoring window.

## What changed from the parents, and why

- Renamed the `cs5` namespace/files to `cs9` (cs9.hpp/cs9.cpp) so this
  variant can load alongside its parent in the same simulator process.
- `Params::feed_from_round`: `340` → `300`. Nothing else changed.

## What I measured

Built `counter-swarm-9` and `counter-swarm-5` (parent), ran head-to-head
sims both directions per map.

**feed_from_round = 300 (final) vs counter-swarm-5:**
- Official maps (`maps/official`, 22 matches): **12-0-10**, mean 340 rounds,
  counter-swarm-9 leads by 1.7 longest-dragon margin (total length margin
  -31.1, i.e. it trades length for a slightly longer champion).
- 20 fresh worldgen maps (`maps/worldgen/worldgen.py --batch 20`, 40 matches
  both sides): **19-0-21**, mean 450 rounds, leads by 1.2 longest-dragon
  margin (total length margin -29.4).
- Combined: **31-0-31** W-D-L, positive longest-dragon margin on both sets,
  slightly negative total-length margin on both — consistent with the
  champion growing longer at the cost of the swarm's total body, which is
  what should matter since the game scores on the longest dragon.

**feed_from_round = 320 (tried, not kept):**
- Official maps only (22 matches): **9-0-13**, clearly worse than 300 on
  the same maps and opponent. Did not bother running it on worldgen maps
  given the clear official-map regression relative to 300.

`build/tools/check counter-swarm-9 maps/official`: all checks passed
(slowest turn 0.1-0.2 ms across all official maps, well under budget).

## What should improve

Longest-dragon margin (the scoring metric) turns positive on both map
sets versus the parent, at the cost of some total-body length — matching
the counter-swarm-1 precedent that motivated this change. 300 clearly
beats 320 on official maps, so 300 is the setting kept.

## What didn't work

320 (a smaller step from 340) underperforms 300 head-to-head against the
parent on official maps (9-13 vs 12-10), so the counter-swarm-1 finding
that earlier feeding helps seems to want a bigger step, not a
conservative one — worth remembering for future feed_from_round tweaks
on other counter-swarm branches.
