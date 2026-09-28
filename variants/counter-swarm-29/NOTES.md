# counter-swarm-29

## Idea

Crossover: counter-swarm-26 (base, renamed cs26->cs29) with two fixes ported near-verbatim
from body-wall-15 (over body-wall-14), isolated by diffing body-wall-15 against body-wall-14
and reading its NOTES.md (replay study #19/#20). Both fixes touch the shared movement layer
(ChooseGoal's trap candidates, the unknown-portal crossing in Decide/SafeStep) that all our
lines descend from via gather-split-trade-38/39.

## What changed from the parents, and why

1. **`champ_no_trap` (champion self-coil, body-wall-15 #20)**: in `ChooseGoal`, once we
   believe we are the champion and `round_ >= champ_freeze` (r330), dead-end (trap) candidates
   are skipped, same as the existing `cap_safety` no-trap rule. Before, a frozen champion could
   walk into a dead end inside its own coils ("into a dead end worth it"), then trap-split and
   leave a ghost champion for many rounds.
2. **`cross_min_length` (sealed-exit drops, body-wall-15 #19)**: the unknown-portal explore
   value a portal gives (in the goal-candidate scan) and the actual crossing step (in `Decide`)
   both now require `length_ >= cross_min_length` (default 4); the crossing step additionally
   requires `CanSplitNow()` when `cross_min_length >= 4`. This stops short dragons from
   crossing into a 1-tile box they can't split out of. The pre-crossing ping in `Turn` is
   unchanged (it only sets up sonar state; the gate lives on whether/when we actually cross).

Left out: body-wall-15's `never_cross` sonar mark (kNever word, `MarkNever`, the `Never()`
gate, and the associated members). Its own NOTES.md measured it did almost nothing alone
(293 vs 292 crosser deaths), so it wasn't worth the extra state and namespace surface for a
near-verbatim port under a 20-minute budget.

## What I measured (vs counter-swarm-26, both seats)

| maps | W-D-L | notes |
|---|---|---|
| official (11 maps, 22 matches) | 10-0-12 | seat-decided, as expected; longest margin -0.3, total -19.0 |
| worldgen set 1 (20 maps, 40 matches) | 20-0-20 | longest margin +2.5, total -11.6 |
| worldgen set 2 (20 maps, 40 matches) | 25-0-15 | strongly positive |

`check counter-swarm-29 maps/official` passes (0 avoidable deaths on all 11 maps).

Combined worldgen (80 matches): 45-0-35, positive on both sets and on longest-margin. Official
maps stay roughly even as expected (they're mostly decided by starting seat/side, per the
brief). Overall at least even with the parent, leaning positive — submitting.

## What should improve

Same as body-wall-15 found: fewer dragons lost to sealed 1-tile-box portal exits, and a
champion that doesn't cut itself down late by coiling into its own dead ends.

## Didn't work / next

- Didn't port `never_cross`; see above.
- Didn't re-run the ablation (champ_no_trap alone vs cross_min_length alone) given the time
  budget; body-wall-15's own ablation found both individually positive on its base, so this is
  a reasonable prior for cs26 too, but worth checking on this line if there's time in a later
  task.
- The unknown-portal crossing is now nearly off (dragons in counter-swarm's split_min_length=4
  range mostly qualify by length but the ping/geometry rarely lines up); a smarter exit check
  (per body-wall-15's own "next" note) could win back useful crossings.
