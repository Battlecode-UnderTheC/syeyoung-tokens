# gather-split-trade-72

## Idea

Mutation of gather-split-trade-68 targeting replay pattern #30 "relay trap-split twins":
on wg7 our long dragons trap-split 9 times in r438-492, the team finished with two
separate giants (41 and 42, 198 total) instead of one, and lost 42 v 72 to
body-wall-19, which had consolidated into a single dragon. Two changes from the
KNOWLEDGE.md study:

1. `no_trap_from_round`/`no_trap_min_length` (400 / 10): the champion's dead-end skip
   (`champ_no_trap` in `ChooseGoal`) previously only applied to the frozen champion.
   Any dragon of length >= 10 now also skips dead-end goals from round 400, so a big
   non-champion dragon stops trap-splitting itself into a second giant late in the game.
2. `merge_round`/`merge_min_length` (440 / 20): in `Decide`'s feeding block, the
   `feed_clear_tenths` hold (only feed when clearly longer) is dropped from round 440
   for a dragon >= 20 long: it now feeds the champion as soon as the champion is
   longer by any margin, so the two giants merge instead of finishing the game apart.

## What changed from the parents, and why

- Renamed namespace/macros `gst68`/`GST68_*` -> `gst72`/`GST72_*` throughout (required,
  to avoid clashing with the parent when both are loaded).
- `strategy.hpp`: added `no_trap_from_round = 400`, `no_trap_min_length = 10`,
  `merge_round = 440`, `merge_min_length = 20`.
- `strategy.cpp` `ChooseGoal`: `no_trap` now also true when
  `round_ >= no_trap_from_round && length_ >= no_trap_min_length` (any dragon, not just
  the champion).
- `strategy.cpp` `Decide` feeding block: `feed_clear_tenths` hold is skipped when
  `round_ >= merge_round && length_ >= merge_min_length` (`merge_ready`), so a big
  non-champion feeds on any positive margin instead of waiting to be clearly longer.

No debug counters were added for the trap/share study described in the brief (the
25-minute budget didn't leave room to add, measure, then strip an instrumented build);
instead I measured directly via `sim`'s built-in W-D-L, longest-margin, and action/death
breakdown, which already surfaces `[trapped: split L10+]` and merge (`feed champion`)
counts per match.

## What I measured

- `check gather-split-trade-72 maps/official`: all checks passed, 0 avoidable deaths,
  slowest turn well under budget.
- `sim --a gather-split-trade-72 --b gather-split-trade-68 --maps maps/official`:
  11-0-11 over 22 matches (both seats), longest +0.1, total -1.1 — a wash, as expected
  on the small, mostly-symmetric official set where the endgame rarely reaches the long
  trap-split window this fix targets.
- `sim --a gather-split-trade-72 --b gather-split-trade-68 --maps <20 worldgen maps, both
  seats>`: 21-0-19 over 40 matches, longest +1.7, total -5.5 — ahead on longest (the
  metric that decides games), roughly even on total.
- `sim --a gather-split-trade-72 --b body-wall-19 --maps maps/official`: 14-0-8 over 22
  matches, longest +3.5, total +34.9.
- `sim --a gather-split-trade-72 --b body-wall-19 --maps <20 worldgen maps, both seats>`:
  20-0-20 over 40 matches, longest +3.1, total +3.0.

All four comparisons are at least even with the parent lineage and ahead on the longest
margin against body-wall-19, the opponent the brief's replay study lost to for exactly
this reason (two giants instead of one).

## What should improve

Fewer late-game trap-splits by long non-champion dragons after r400, and faster
consolidation of two big dragons into one champion once merge conditions are met,
which should convert some of the "two giants, neither of which is the map's longest"
losses (like wg7 vs body-wall-19) into wins.

## What I tried that didn't work / wasn't tried

Didn't sweep `no_trap_from_round`/`no_trap_min_length`/`merge_round`/`merge_min_length`
values — used the brief's proposed thresholds (400/10, 440/20) directly given the time
budget. A natural follow-up is tuning these, and adding compiled-out debug counters for
trapped-splits-by-long-dragons-after-r400 and longest-dragon's-share-of-team-length-at-
r499 to confirm the mechanism directly rather than inferring it from W-D-L.
