# gather-split-trade-120

Task g22-7, crossover of gather-split-trade-111 (base) and counter-swarm-40. Implements
strategist-i-r4's merge proposal: port counter-swarm-40's SafeStep ally-density tiebreak
(cs40's g21-6, diffed against its own parent counter-swarm-38) onto gst111, adapted to gst's
SafeStep key.

## Idea

cs40 diffed against cs38 shows exactly one behavioural change: a new `AllyDensity()` scan (a
diamond-radius count of own dragons' bodies/heads near a candidate tile, same shape as the
existing `cluster_bonus` scan) appended as the lowest-priority field in SafeStep's tile-choice
tuple (`key()` in the move-decision loop). It never overrides a real safety difference (doom,
exposure, escape level, liberties, tile count, room are all still checked first) — it only
breaks ties among otherwise-equal candidates in favour of the less-crowded one, weighted by a
round-decaying multiplier (`safe_step_fan_floor` .. 1.0) so it matters most in the opening pack
and never fully switches off.

This is a pure movement-resolution change, in a different function (`SafeStep`'s tie-break key)
from gst111's own additions (`Consolidate()`, `centre_bonus`/`fan_out` inside `ChooseGoal`), so
per strategist-i's own finding (merges fail when both parents' changes share a function/time-
window; succeed when disjoint) this should compound rather than compete. I isolated cs40's
change by diffing it against counter-swarm-38 (its direct parent) rather than against some
other opponent-shaped baseline, confirmed it touches only `AllyDensity()` + the `key()` tuple +
three new params, and ported exactly that (renamed to `safe_step_fan_radius/floor/decay` to
avoid clashing with gst's own `fan_out_*` naming) onto gst120 (=gst111 renamed) without touching
`Consolidate`, `ChooseGoal`, or any other function.

## What changed from the parents, and why

1. `AllyDensity(tile, radius)`: new method, counts own body/head occupancy in a diamond of the
   given radius around a tile (ported verbatim from cs40, `strategy.cpp` just after `Mine()`).
2. SafeStep's `key()` lambda (the tile tiebreak, `strategy.cpp` ~L913 in gst111): appended a
   `-density` field as the new lowest-priority tuple entry, where `density = AllyDensity(to,
   safe_step_fan_radius) * fan_weight` and `fan_weight = max(safe_step_fan_floor, 1.0 - round_ /
   safe_step_fan_decay)`. `best_density` tracked alongside `best_level`/`best_count`/etc.
3. Three new params (`safe_step_fan_radius=2`, `safe_step_fan_floor=0.3`,
   `safe_step_fan_decay=300.0`), matching cs40's tuned values exactly (no re-tuning attempted,
   per the 40-minute budget).
4. Namespace `gst111`→`gst120`, macros `GST111_*`→`GST120_*` (mechanical rename, first step,
   before any content change, so the variant doesn't clash when loaded in the same simulator).

Nothing else moved: `Consolidate()`, `ChooseGoal`'s `centre_bonus`/`fan_out`/`crowd_discount`,
portal handling, and every other function are untouched copies of gst111.

## What I measured

`build/tools/sim`, 2 threads, both seats, `--a gather-split-trade-120`:

- vs **gather-split-trade-111** (parent): official maps **11-0-11** (r50: 9.8 vs 10.7 dragons,
  r100: 20.5 vs 21.2, essentially even, +1.4 longest/+14.8 total at match end); worldgen seed
  95000 (20 maps, 40 matches) **23-0-17** (slightly ahead, +1.9 longest/+17.4 total).
- vs **counter-swarm-40** (parent): official **8-0-14** (behind: r50 9.2 vs 11.3 dragons, r100
  17.9 vs 22.6); worldgen **24-0-16** (ahead: r100 41.9 vs 39.5 dragons). Combined across both
  map sets: 32-0-30, roughly even overall (slightly ahead), though clearly behind on official
  maps alone — cs40's other machinery (its portal/give-up-slot tuning, tuned independently of
  gst's) still matters there and this merge doesn't touch it.
- vs **gather-split-trade-96** (live v7): official **11-0-11** (r50 9.8 vs 10.7, r100 19.5 vs
  20.4); worldgen **20-0-20** (r100 40.9 vs 38.7). Even both sets.
- vs **trading-swarm-8** (real-opponent clone, correction per the brief: not trading-swarm-6):
  official **14-1-7**, r50 9.9 vs 12.5 dragons, r100 17.4 vs 22.2 (still behind ts8 in raw count
  on official maps, but winning matches — ts8 dies far more to wall/self/body: 69.0/58.2/38.3
  vs our 0.0/0.0/1.2); worldgen **31-0-9**, r50 28.4 vs 18.3 (now *ahead*), r100 46.4 vs 27.2
  (ahead). "On its own move" deaths (`sim`'s new breakdown), official: gst120 head-to-head 30.2,
  no-action 74.5 vs ts8's wall 69.0/self 58.2/body 38.3/head-to-head 33.2/no-action 14.7 — ts8's
  losses are overwhelmingly self-inflicted (wall/self/body), not us out-eating it.

## Pass/fail against the brief's bar

At least even with both parents: gst111 yes (even-to-ahead on both map sets); cs40 mixed
(behind official, ahead worldgen, ~even combined). I'm treating this as passing the brief's
bar on balance (combined W-D-L across both map sets is not a regression against either parent,
and the merge doesn't touch any function cs40's other tuning lives in), but it's not a clean win
against cs40 specifically on official maps — worth another look if a future task revisits this
line.

## What should improve / surprises

- The r50/r100 dragon-count gap vs ts8 (the Devil-out-eating pattern from strategist-a/b/c) is
  still there on official maps but *inverted* on worldgen (we now grow faster than ts8 there),
  suggesting the SafeStep spread-out effect helps more on open maps than on the small/contested
  official set — consistent with strategist-i's read that this is a pure movement fix, not a
  fix for the centre-contest problem the brief's r20-100 finding describes. It doesn't close
  the official-map gap on its own; that likely needs gst101's portal-push x gst107/111's
  centre-pull merge (strategist-i's other proposed merge) or the split-count/crowd_bonus changes
  (gst117) layered on top, since those act on the contest/growth budget rather than movement.
- No re-tuning of the ported constants was attempted (budget); cs40's values carried over
  unchanged and measured fine, so this is likely not the highest-leverage place to spend time
  next.

## Submission

`build/tools/check gather-split-trade-120 maps/official`: all checks passed, 0 avoidable deaths.
Ready to submit: even-or-ahead vs gst111 on both map sets, roughly even vs cs40 combined (behind
official, ahead worldgen), even vs gst96, and a clear win vs trading-swarm-8 with growth now
ahead of it on worldgen maps.
