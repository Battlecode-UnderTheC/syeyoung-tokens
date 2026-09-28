# late-surge-24

## Idea

Crossover: late-surge-23 (the base) plus body-wall-15's two endgame/portal fixes, ported
near-verbatim since both lines share the same movement layer (gather-split-trade-38/39
descent). No new local design.

## What changed from the parents, and why

Namespace `ls23` -> `ls24`, macros `LS23_*` -> `LS24_*` (no clash with either parent).

1. **Champion self-coil** `champ_no_trap = 1` (new param): in `ChooseGoal`, once frozen
   (`round_ >= champ_freeze` and we are the champion) a dragon now skips dead-end (trap)
   candidates, same as the cap-safety rule does for a long dragon near the cap. Before, the
   frozen champion could dive into a dead end inside its own coils ("into a dead end worth
   it"), coil, and trap-split, leaving a ghost champion for many rounds.
2. **Sealed-exit drops** `cross_min_length = 4` (new param): the unknown-portal explore
   crossing now needs `length_ >= cross_min_length`, applied in all three places body-wall-15
   gated: the explore value an unknown portal gives (`ChooseGoal`), the pre-crossing ping
   (`Turn`), and the crossing step itself (`Decide`, also requires `CanSplitNow()` when the
   gate is 4). A crosser landing in a 1-tile box can then split out (the child walks out the
   entry side) instead of dying.
3. Left out the `never_cross` sonar mark (kNever ping, `Never()`, `MarkNever()`): body-wall-15's
   own NOTES.md says it did almost nothing measurably (293 vs 292 crosser deaths in its
   ablation) and it adds sonar-word and state complexity for no measured benefit. The length
   gate is what saves the dragons.
4. late-surge-23's lunge-reach guard for long dragons and the champion is unchanged (kept).

## What I measured (vs late-surge-23, both seats, `sim --threads 2`)

| maps | W-D-L | longest margin (at end) | total margin |
|---|---|---|---|
| official (11 maps, 22 matches) | 8-0-14 | -4.5 | -6.9 |
| worldgen set 1 (20 maps, 40 matches) | 27-0-13 | +4.7 | -6.4 |
| worldgen set 2 (20 maps, 40 matches) | 19-0-21 | (~even) | (~even) |

Worldgen combined: 46-0-34, positive. Official is negative but small (22 matches) and, per
the task brief and prior notes, official maps are mostly seat-decided; the worldgen sets are
the more reliable signal and weighted accordingly. Death/action-reason counts show the port
took effect: "portal: explore" crossings dropped from ~20/match (late-surge-23) to ~0/match
(late-surge-24) on both worldgen sets, matching body-wall-15's own ~98% cut. "into a dead end
worth it" turns were roughly flat overall (287 vs 284, 226 vs 226) since most such turns
belong to non-champion dragons, which the fix doesn't touch.

## What should improve

Fewer dragons lost stepping through unknown portals into sealed 1-tile exits (the crossing
count and its fatal-turn share both collapsed, as in body-wall-15). Some reduction in the
frozen champion dying by coiling into its own dead ends late in a match, though this variant's
champion rarely reached round 330 in short local matches, so the effect is hard to see
directly in these numbers; it should still show up in longer matches against strong opponents
in league play.

## Tried / didn't work

Nothing else tried (20-minute loop, single focused port). Didn't re-run body-wall-15's
ablations locally; trusted its NOTES.md numbers since the ported code is verbatim.
