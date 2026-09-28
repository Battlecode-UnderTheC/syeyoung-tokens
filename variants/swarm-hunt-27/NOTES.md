# swarm-hunt-27

## Idea

Crossover of swarm-hunt-26 (base) and gather-split-trade-72: port gst72's late-game
consolidation fix into swh26. Replay pattern #30 saw a team finish with two 41-42-long
giants and lose 42 v 72 to a consolidated opponent; gst72 fixed this in the
gather-split-trade lineage (32-30 vs gst68, 34-28 vs body-wall-19) with two changes.
This variant ports the same two changes into swh26's `ChooseGoal`/`Decide`.

## What changed from the parents, and why

- Renamed namespace/macros `swh26`/`SWH26_*` -> `swh27`/`SWH27_*` throughout (required
  so the variant can't clash with the parents when loaded into one simulator).
- `hunt.hpp`: added `no_trap_from_round = 400`, `no_trap_min_length = 10`,
  `merge_round = 440`, `merge_min_length = 20` (also wired into the `SWH27_SET` tuning
  macro list for consistency with the rest of the parameter block).
- `hunt.cpp` `ChooseGoal`: swh26 already had the frozen-champion dead-end skip
  (`champ_no_trap`, from body-wall-15). `no_trap` now also true when
  `round_ >= no_trap_from_round && length_ >= no_trap_min_length`, so **any** dragon of
  length >= 10 (not just the champion) skips dead-end goals from round 400, stopping a
  big non-champion from trap-splitting itself into a second giant late in the game.
- `hunt.cpp` `Decide` feeding block: the `feed_clear_tenths` hold (only feed when
  clearly longer) is now skipped (`merge_ready`) when
  `round_ >= merge_round && length_ >= merge_min_length`, so a >= 20-long non-champion
  merges into the champion as soon as the champion is longer by any margin, instead of
  waiting to be clearly longer.

Both changes are lifted verbatim (same thresholds, same gating logic) from gst72's
diff against gst68, per the brief.

## What I measured

- `build/tools/check swarm-hunt-27 maps/official`: all checks passed, 0 avoidable
  deaths on all 11 official maps.
- `sim --a swarm-hunt-27 --b swarm-hunt-26 --maps maps/official --threads 2` (both
  seats, 22 matches): 12-0-10, swh27 leads by +0.6 longest, -0.1 total — even/slightly
  ahead.
- `sim --a swarm-hunt-27 --b swarm-hunt-26 --maps <20 worldgen, both seats>` (40
  matches): 20-0-20, +0.3 longest, +10.8 total — even, slightly ahead on both metrics.
- `sim --a swarm-hunt-27 --b body-wall-19 --maps maps/official` (22 matches): 7-0-15 —
  identical to swarm-hunt-26 vs body-wall-19 on the same maps (also 7-0-15), so this
  loss record is pre-existing to the lineage, not caused by this change.
- `sim --a swarm-hunt-27 --b body-wall-19 --maps <20 worldgen, both seats>` (40
  matches): 26-0-14, +3.5 longest, +35.6 total, vs the swarm-hunt-26 baseline on the
  same maps of 25-0-15, +3.7 longest, +35.1 total — essentially the same, a hair ahead
  on win count.

At least even with both parents/baselines on every comparison; no regression found.

## What should improve

Fewer late-game trap-splits by long non-champion dragons after r400, and faster
merging of two big dragons into one once the merge condition is met — the same
mechanism gst72 confirmed helps against body-wall-19's consolidation strategy. The
swh26 lineage's existing 7-0-15 deficit against body-wall-19 on the official maps looks
structural (present identically in the parent) rather than caused by the giants-split
pattern this task targets, so this port likely won't close that gap by itself; the
worldgen numbers (26-0-14) suggest it's roughly neutral-to-helpful there.

## What I tried that didn't work / wasn't tried

Used the brief's/gst72's thresholds directly (400/10, 440/20) without sweeping, per the
short budget. Didn't add trace-based confirmation that fewer trap-splits/faster merges
actually occurred (relied on W-D-L/longest-margin deltas as gst72's report did); a
natural follow-up is instrumenting `[trapped: split L10+]`-after-r400 and
`[feed champion]`-after-r440 counts directly to confirm the mechanism, and checking
whether the official-map deficit against body-wall-19 has a different root cause worth
a separate task.
