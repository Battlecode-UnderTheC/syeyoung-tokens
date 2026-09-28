# gather-split-trade-84

## Idea

Replay pattern #33: in the opening several of our dragons pile onto the same bare frontier
tile because each independently scores it as the best unexplored edge to walk to, and
`share_factor`'s discount for "an ally in view is nearer to this tile" doesn't reach pure
explore tiles (their score is a `max()` with the discounted base score, not a product, so the
undiscounted explore value always wins).

## What changed from the parents, and why

- Namespace/macros renamed `gst74` -> `gst84` (and `GST74_*` -> `GST84_*`) so this variant can
  be loaded alongside its parent without clashing.
- New params `fan_out_until` (80) and `fan_out_radius` (3).
- In `ChooseGoal`'s frontier scan (the loop over unexplored-edge tiles), before round
  `fan_out_until`, a candidate frontier tile within `fan_out_radius` of an ally seen this turn
  who is nearer to that tile than we are gets **no explore credit at all** (the tile is simply
  skipped for the explore-value override), instead of being discounted. This is deliberately
  different from gst82 (task/g16-25, not tagged): that attempt multiplied the shared tile's
  explore score by `share_factor` and lost 26-36, because the discounted dragon then had
  nothing else to fall back on and often ended up idling ("no goal: roomiest") instead of
  picking a different frontier tile. Excluding the tile from scoring (rather than discounting
  it) lets the scan's natural max-search land on the next-best (further) frontier tile, so the
  dragon still has a goal, just not the contested one.
- Gated to explore goals only (the branch this touches only ever assigns the "unexplored edge"
  score) and to the opening (`round_ < fan_out_until`), so midgame pearl/bed contention (already
  handled by `share_factor`/`contest_factor`) is untouched.
- Did not add the debug-count-and-compile-out instrumentation for "distinct goals in view" /
  "pearls eaten by r50" the brief suggested; `sim`'s per-match action-reason tallies (a large
  `[toward goal]`/`[no goal: roomiest]` split, and total pearls via total-length margin) were
  enough signal for this size of change given the 20-minute budget, and adding a second set of
  debug-only fields felt like scope creep for a one-line frontier-scan change.

## What was measured

- `check gather-split-trade-84 maps/official`: all checks passed (0 avoidable deaths on every
  official map; slowest-turn timings were noisy on this shared machine, from 0.1ms to ~40ms
  across repeated runs of the *same* build with identical audited-memory-step counts, so that
  number is machine load, not a regression).
- `sim --a gather-split-trade-84 --b gather-split-trade-74 --maps maps/official --threads 2`:
  **12-0-10** (22 matches, both seats), leads by +1.9 longest, -8.6 total length.
- Two independent batches of 20 worldgen maps (`worldgen.py --batch 20`), both seats, vs the
  same parent: **19-0-21** and **24-0-16** (40 matches each). Combined worldgen: 43-0-37.
- Combined official + worldgen vs gather-split-trade-74: 55-0-47 (roughly even, tilted
  slightly our way).
- `sim --a gather-split-trade-84 --b trading-swarm-5 --maps maps/official --threads 2`:
  **13-1-8** (22 matches) — a clear win against the opponent the replay pattern was measured
  against.

## What should improve

Fewer dragons stacking on one bare frontier tile in the opening, which the #33 replay flagged
as costing us pearls early (we ate 24 to trading-swarm-5's 173 on wg3, r0-49) — this should
translate into more distinct explore targets among allies in view and a better early pearl
count, though I did not add the instrumented counters to confirm that mechanism directly this
round; the win margin against trading-swarm-5 and rough parity/slight edge against the parent
are the evidence I have.

## What I'd try next

- Wire the debug counters (distinct goals among dragons in view; pearls eaten by r50, compiled
  out under `GST84_DEBUG`) to directly confirm the fan-out is firing and check it against the
  #33 replay map (wg3) specifically with `--trace`.
- Consider whether `fan_out_radius`/`fan_out_until` need tuning — radius 3 and round 80 were
  picked from the brief's own numbers, not swept.
- The exclusion is one-sided (only checks tiles nearer to an ally than to us); it doesn't
  currently distinguish "ally is heading there" from "ally merely happens to be near", so on a
  map where allies cross paths without both wanting the same tile this could occasionally give
  up a tile needlessly. Didn't see evidence of this in the sim numbers, but worth watching.
