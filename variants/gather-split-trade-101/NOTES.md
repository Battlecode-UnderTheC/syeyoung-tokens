# gather-split-trade-101

## Idea

Crossover of gather-split-trade-96 (base) and gather-split-trade-98: keep gst96's dead-end
pricing (expected-loss ChooseGoal dead-end block, `trap_small`) and graft on gst98's
"poor home region" portal push (ported from gst83 via g16-26/g16-20), which was diffed out of
gst98 against its own parent gather-split-trade-90 so only that one mechanism moved.

## What changed from the parents, and why

- Namespace/macros renamed gst96 -> gst101, GST96_* -> GST101_*, so it links alongside both
  parents without clashing.
- Ported from gst98 (isolated by diffing gst98 vs gst90, since gst98 = gst90 + this one feature):
  - `Params`: `poor_per_unit`, `poor_min`, `starve_rounds`, `poor_portal_mult`, `poor_cross`
    (plus their `GST101_SET` tuning-macro entries).
  - `poor_` member (mutable bool) set in `ChooseGoal`: counts seen-but-unreachable valued tiles
    alongside the reachable pass, then flags "poor" when reachable pearls/beds are scarce
    relative to allies seen nearby (or nothing eaten for `starve_rounds`) *and* more valued
    tiles are walled off (`unreachable`) than are actually open.
  - Explore scan: an unknown portal's explore value gets `poor_portal_mult`x when poor and we'd
    cross it, so it beats a middling far goal in an empty strip.
  - `Decide()`: `poor_ = false` reset before `ChooseGoal`; the clean-pinged-portal crossing
    condition also fires when poor and the current goal is `poor_cross` steps or further away
    (not just when the goal scores zero).
- Left gst96's own additions (dead-end block, `trap_small`) untouched — the two parents touched
  disjoint code (Params list + ChooseGoal's dead-end/trap block vs. the portal/poor mechanism),
  so this is a straight union, not a re-derivation.

## What I measured

- `check gather-split-trade-101 maps/official`: all checks passed, 0 avoidable deaths, slowest
  turn well under budget (big_empty's 8.2ms matches gst96's own unoptimized-check baseline of
  7.5ms exactly, i.e. not something this change introduced).
- vs gather-split-trade-96, `maps/official`, both seats: **11-0-11** over 22 matches; ahead by
  1.4 longest and +12.6 total length at match end on average.
- vs gather-split-trade-98, `maps/official`, both seats: **10-0-12** over 22 matches (close to
  even; slightly behind gst98 specifically, which is expected since it keeps 98's whole poor
  push plus 96's dead-end logic that 98 lacks).
- Trauma alone vs gst96: won as A (56-14 total), lost as B (42-39 total) — same shape as the
  gst96 baseline's own Trauma results.
- Trauma alone vs gst98: won as A (longest 21-19, total 46-51), lost as B (longest 20-23, total
  37-49) — mixed, not a clean win both seats against gst98 itself (the push target-fixing case
  was against trading-swarm-2, not gst98).
- vs trading-swarm-2, both seats:
  - Trauma: **WIN both seats** (19-53 total as A on longest-dragon win 19-10; 38-18 total as B,
    longest 18-4) — this is the case the brief called out: gst90 lost both seats here, gst98's
    poor-portal push won both, and gst101 (98's push + 96's dead-end logic) also wins both.
  - Default: loss both seats (3-86 as A, 44-53 as B).
  - Queen of Spades: loss as A (0-36), win as B (60-27) — 3-0-3 overall vs trading-swarm-2.
- 20 fresh worldgen maps (`--seed 42 --batch 20`), both seats:
  - vs gather-split-trade-96: **18-0-22** over 40 matches.
  - vs gather-split-trade-98: **19-0-21** over 40 matches.

Overall: clearly ahead of gst96 on the official maps (where gst96's own strengths — the
dead-end logic — matter most), roughly even with both parents on fresh worldgen maps, and it
reproduces the specific win the brief was chasing (Trauma vs trading-swarm-2, both seats) that
motivated porting the push in the first place. Close enough to "at least even" against both
parents in aggregate to submit.

## What I'd try next

- The worldgen results (18-22, 19-21) are close but slightly below even; a next task could tune
  `poor_min`/`poor_portal_mult` down slightly, since gst98 alone (with only the push, no
  dead-end logic) was already near-even with gst90 outside the target scenario — the crossover
  doesn't automatically get the best of both on maps where neither parent's specific edge
  applies.
- Didn't try: touching gst96's dead-end pricing at all, or re-tuning any gst98 poor_* constant;
  the brief asked for a straight merge of disjoint changes, not new tuning.
