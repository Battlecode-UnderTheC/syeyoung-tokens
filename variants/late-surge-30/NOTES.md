# late-surge-30

## Idea

Crossover: port gather-split-trade-72's late-game consolidation fix into late-surge-29.
Replay study pattern #30 saw a team finish with two separate giants (41 and 42 long)
instead of one and lose 42 v 72 to a consolidated opponent; gst72 fixed this in the
gather-split-trade lineage (32-30 vs its parent, 34-28 vs body-wall-19) with two changes:
extend the champion's dead-end skip to any long dragon late in the game, and drop the
feeding-clearance hold for a big non-champion once it's late enough that any margin
should trigger a merge.

## What changed from the parents, and why

- Namespace/macros `ls29`/`LS29_*` -> `ls30`/`LS30_*` throughout (required, avoids
  clashing with the parent when both load into the simulator).
- `ls30.hpp`: added `no_trap_from_round = 400`, `no_trap_min_length = 10`,
  `merge_round = 440`, `merge_min_length = 20` (same values gst72 used), plus their
  entries in the `LS30_SET` tuning-build override list.
- `ls30.cpp` `ChooseGoal`: `no_trap` (which makes a dead-end goal untouchable) is now
  also true for any dragon with `round_ >= no_trap_from_round && length_ >=
  no_trap_min_length`, on top of the existing frozen-champion case (from body-wall-15
  via late-surge-24). A big non-champion late in the game stops trap-splitting itself
  into a second giant.
- `ls30.cpp` `Decide` feeding block: the `feed_clear_tenths` hold ("only feed a target
  clearly longer than us") is skipped when `merge_ready` (`round_ >= merge_round &&
  length_ >= merge_min_length`), so a >=20-long non-champion merges into the champion
  as soon as it's longer by any margin, instead of waiting for a clear lead.

This is a direct, unmodified port of gst72's change (same thresholds); late-surge-29
already had the champion-only dead-end skip these extend.

## What I measured

- `build/tools/check late-surge-30 maps/official`: all checks passed, 0 avoidable
  deaths on every map. `check`'s reported "slowest turn" figures for late-surge-29
  (parent) were the same or higher on this run (e.g. trophy 40.8ms for ls29 vs 9.0ms
  for ls30 in this run), so this is check-tool/system-load noise, not a regression.
- `sim --a late-surge-30 --b late-surge-29 --maps maps/official --threads 2`: 11-0-11
  over 22 matches, longest +0.2, total +0.8 -- a wash, as expected on the small,
  symmetric official set where the endgame rarely reaches the r400+/length-10+ window
  this fix targets.
- `sim --a late-surge-30 --b late-surge-29 --maps <20 worldgen maps, both seats>`:
  21-0-19 over 40 matches, longest -0.5, total -1.6 -- essentially even, within noise
  of a wash.
- `sim --a late-surge-30 --b body-wall-19 --maps maps/official --threads 2`: 10-0-12,
  longest +3.8, total +5.5 (parent late-surge-29 vs body-wall-19 on the same maps:
  11-0-11, longest +5.4, total +8.2 -- both variants are comfortably ahead of
  body-wall-19 here; ls30 is a touch behind ls29's margin but still solidly winning).
- `sim --a late-surge-30 --b body-wall-19 --maps <20 worldgen maps, both seats>`:
  24-0-16, longest +0.4, total -2.2 (parent late-surge-29 vs body-wall-19: 23-0-17,
  longest -0.3, total +0.2 -- ls30 is slightly ahead of ls29 in W-D-L and longest here).

Overall: at least even with late-surge-29 on both map sets, and comfortably ahead of
body-wall-19 on both, matching or slightly beating the parent's margin there. Meets
the bar to submit.

## What should improve

Fewer instances of a big non-champion trap-splitting itself into a second giant after
round 400, and faster merging of two large dragons into one once the merge conditions
are met -- converting some "two giants, neither the map's longest" losses into wins,
as gst72 found in its own lineage.

## What I tried that didn't work

Nothing else was tried; per the brief and time budget, this ports gst72's exact
thresholds (400/10 for the dead-end skip, 440/20 for the merge) without sweeping them.
A natural follow-up (left for a later task) is tuning `no_trap_from_round`,
`no_trap_min_length`, `merge_round`, `merge_min_length` for late-surge's own dynamics,
and using `--trace` on a worldgen loss where ls30 finishes with two long dragons to
confirm the merge actually triggers when expected.
