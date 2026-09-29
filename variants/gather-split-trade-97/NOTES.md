# gather-split-trade-97

## Idea

Fix a false dead-end call on through-corridors. `SafeStep`'s room check used
`open(t) = !Occupied(t) && !Exposed(t)` as the `enter` predicate for
`RoomAround`. `Exposed(t)` is true whenever *any* enemy head sits adjacent to
`t` and isn't worth trading with. Inside a BFS flood fill this doesn't just
gate the entry tile (which is already checked separately via the `exposed`
local at the actual step) — it also excises every interior tile within 1 of
any enemy head anywhere in the reachable region. A single enemy head
anywhere near a 1-wide corridor's interior (or near open space just past it)
can sever the flood fill there, so the room count for stepping into the
corridor comes back below `RoomNeeded()` even though the corridor plainly
leads to open space. `SafeStep` then treats the goal step as an unsafe
"detour", and the dragon skips pearls that require crossing that corridor.

`ChooseGoal`'s own dead-end check (line ~603) already uses
`open(t) = !Occupied(t)` with no `Exposed` filter — so this was a real
mismatch between the two dead-end judgments in the same file, not an
intentional double safety margin.

## What changed from the parents, and why

- Namespace/macros renamed `gst90`→`gst97`, `GST90_*`→`GST97_*` (mechanical,
  required so this variant doesn't clash with its parent when both are
  loaded).
- `strategy.cpp`, `SafeStep()`: room-check `open` predicate for `RoomAround`
  dropped the `!Exposed(t)` term, matching `ChooseGoal`'s predicate. The
  immediate step's own exposure is still gated separately (the `exposed`
  local, checked at the point of the actual move), so a dragon still refuses
  to step onto a tile an enemy head can take next turn — this change only
  stops an enemy head *deep inside or past* a corridor from making the
  corridor itself look like a sealed dead end.

## What I measured

- `check gather-split-trade-97 maps/official`: all checks passed, 0 avoidable
  deaths on every official map.
- `sim --a gather-split-trade-97 --b gather-split-trade-90 --maps maps/official --threads 2`,
  both seats, all 11 official maps (22 matches): **12-0-10** W-D-L in gst97's
  favor; gst97 leads by +2.8 longest, -1.6 total length on average at match
  end. Deaths/actions profile is close to identical to the parent's
  (head-to-head and no-action deaths within noise), consistent with a
  narrowly-targeted fix rather than a behavior change that ripples widely.
- `sim --a gather-split-trade-97 --b gather-split-trade-90` on 13 fresh
  `--style labyrinth` worldgen maps (seed 20+, both seats, 26 matches):
  **16-0-10** W-D-L in gst97's favor, +4.3 longest / +17.6 total length at
  match end on average. Death and action-reason profiles track the parent's
  closely (no new death category, no spike in "trapped" outcomes), which is
  what a narrow fix to one refusal path should look like.
- Time ran out before a caverns and default-style worldgen comparison (the
  worldgen batch generator stalled partway through caverns); the official
  and labyrinth results above are what this submission rests on.

## What I'd try next

- Instrument a histogram (as the brief suggested) of *why* the preferred
  direction gets refused in `SafeStep`, split by corridor-like step tiles
  (<=2 free neighbours), to confirm `Exposed`-inside-`RoomAround` really is
  the dominant cause here and not, say, the BFS in `ChooseGoal`'s path search
  itself avoiding corridors before a goal is even picked.
- Consider whether `Exposed` should also be softened for the `no_trap`
  dead-end gate in `ChooseGoal` (it already excludes `Exposed` from `open`
  there, so that path shouldn't have this bug — but worth re-checking with
  `--trace` on a real skipped-pearl example).
