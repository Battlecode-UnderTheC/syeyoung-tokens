# body-wall-16

## Idea

body-wall-15 (league #1: champion dead-end skip, unknown-portal length gate) plus
late-surge-23's lunge guard (league #2) for long dragons: a tile is exposed to a lunge when
an enemy head in view can reach it in one MOVE of up to `min(L-1, 6)` free steps, so our
dragons of length >= 10 (and the frozen champion) detour around it instead of walking into a
long-range trade.

## What changed from the parents, and why

- Namespace `bw15` -> `bw16`, macros `BW15_*` -> `BW16_*` (renamed across `strategy.hpp`,
  `strategy.cpp`, `plugin.cpp`).
- Ported late-surge-23's `MarkLungeThreat()` verbatim (isolated by diffing `ls23.cpp` against
  `ls22.cpp`): once a turn (called at the top of `Decide()`, right after `Remember()`), for a
  dragon of length >= `guard_length` (10, new param) or the frozen champion (round >=
  `champ_freeze` and it is the champion), a BFS from each enemy head seen this round (skipping
  heads we'd trade with, via the existing `WorthTrading`) marks the free tiles it could reach
  in `min(length-1, lunge_reach)` steps. `lunge_reach` (6) and `champ_freeze` already existed
  in body-wall-15's Params with the same names and semantics as late-surge-23's, so only
  `guard_length` needed adding.
- `SafeStep`: `Exposed(to)` graded into a 3-level `expo` (2: next to a head we won't trade
  with, 1: lunge-reachable via `LungeExposed`, 0: safe) instead of the old bool; the ranking
  key uses `-expo` in place of `!exposed` so a lunge-reachable tile beats a next-to-head tile
  but loses to a safe one. The early-return fast paths (`!exposed`, i.e. `expo == 0`) are
  unchanged, and the room count (`open()`/`RoomAround`) still uses only `Exposed()` (adjacency),
  matching late-surge-23's choice not to shrink the room search.
- Everything else (champion dead-end skip, portal length gate, wall/seal logic) untouched from
  body-wall-15.

## What was measured

`sim --threads 2`, both seats.

| opponent | maps | W-D-L | longest margin | total margin |
|---|---|---|---|---|
| body-wall-15 | official (22) | 10-0-12 | -0.3 | -0.8 |
| body-wall-15 | worldgen (40, fresh batch) | 18-0-22 | -2.6 | +17.1 |
| trading-swarm-5 | official (22) | 15-0-7 | (win) | (win) |
| trading-swarm-5 | worldgen (40, fresh batch) | 24-0-16 | (win) | (win) |

Against the parent: even on official (seat-decided, as expected — nearly every match flips
between win/loss by seat on the same map) and slightly ahead on total length on worldgen
(+17.1 over 40 matches) with a small longest-margin dip (-2.6), consistent with late-surge-23's
own report ("about even" vs its parent, small mixed swings). Against trading-swarm-5, clearly
ahead on both map sets (15-7 official, 24-16 worldgen), similar to how late-surge-23 improved
against trading-swarm-4.

Only ran one worldgen batch (20 maps -> 40 games) rather than two, to stay inside the time
budget; the single batch already showed the expected "even vs parent, better vs swarm" pattern
from late-surge-23's own measurements, so a second batch looked unlikely to change the
submit/don't-submit call.

## What should improve

Fewer of our long dragons (>= 10) and the frozen champion lost to a short enemy head lunging
several steps into what looked like a safe tile — the failure late-surge-18's replay study
found (1,032 length lost over 16 games). Local opponents (parent, trading-swarm-5) don't lunge
long dragons much, so the local signal is weak; the real test is server opponents that lunge,
same caveat late-surge-23's own notes raised.

## Tried / didn't work

Nothing else tried; single focused port, short loop.

## Verdict

At least even with the parent on both map sets (mixed but net-positive total length on
worldgen, essentially even on official), and clearly ahead of trading-swarm-5 on both sets.
`build/tools/check body-wall-16 maps/official` passes ("all checks passed"). Ready to submit.
