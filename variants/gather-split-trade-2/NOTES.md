# gather-split-trade-2

## Idea

strategy-v1 loses ~47 dragons a match to "trapped: no move" (no safe step available),
worse on worldgen mazes and 1-wide warrens. `Params::room_margin` and `room_min`
(strategy.hpp) set how much reachable room (via flood fill) a step must leave: at least
`length_ + room_margin`, never less than `room_min`. The claim to test: stricter room
checks (bigger margin/min) reduce trapped deaths without costing games. This is a pure
parameter tweak — no other logic touched.

## What changed from the parents, and why

- Namespace `strat` -> `gst2` in strategy.hpp/.cpp/plugin.cpp (required: the simulator
  loads strategy-v1 and this variant into one process; identical symbol names clash).
  Shared-lib types from `shared/lib/{board_memory,pathing}.hpp` stayed in `namespace
  strat` (that's the shared library's own namespace, not the copied variant code), so
  references to `BoardMemory`, `TileMemory`, `Paths`, `Search`, `kBlocked`, `kUnknown`
  were qualified with `strat::`.
- `room_margin`: 3 -> 6, `room_min`: 8 -> 14 (final value; see settings tried below).
  Nothing else in `Params` or the logic changed.

## Settings measured (vs strategy-v1, `build/tools/sim`, 2 threads)

All numbers are **per-match averages** (deaths-per-match / fatal-turns tables that
`sim` prints), on the 11 official maps (22 matches, both sides as A and B) and on
fresh `worldgen.py --batch 20` maps (40 matches per batch, freshly generated for each
run, so the two worldgen rows for margin 6/14 are two independent batches).

| room_margin | room_min | maps | W-D-L | trapped/match (gst2 vs v1) |
|---|---|---|---|---|
| 6 | 14 | official | 12-0-10 | 3.86 vs 3.53 (+9%) |
| 6 | 14 | worldgen batch1 | 24-0-16 | 9.73 vs 10.55 (-8%) |
| 6 | 14 | worldgen batch2 | 18-0-22 | 10.02 vs 11.01 (-9%) |
| 5 | 12 | official | 12-0-10 | 3.75 vs 3.23 (+16%) |
| 5 | 12 | worldgen | 19-0-21 | 9.69 vs 10.60 (-9%) |
| 4 | 10 | official | 12-0-10 | 3.99 vs 3.32 (+20%) |
| 4 | 10 | worldgen | 19-0-21 | 9.95 vs 10.61 (-6%) |

Combined across both worldgen batches for the chosen setting (margin 6 / min 14):
**42-0-38** over 80 matches. Combined with official: **54-0-48** over 102 matches.

## What should improve

Trapped-("no move") deaths per match dropped consistently on worldgen maps (~8-9%
fewer than strategy-v1, both batches, all three margin/min settings tried) — the
claim holds there: worldgen's mazes and 1-wide warrens are exactly where a stricter
room check helps, since a shorter lookahead there more often walks a dragon into a
pocket it can't later escape.

On official maps the effect reversed: trapped deaths went *up* (9-20% more than
strategy-v1, worse the stricter the check). Official maps are more open; here the
stricter check makes the dragon reject moves it would have survived (turning down
tighter-but-safe rooms), so it ends up taking worse detours or getting cornered
elsewhere later, net more trapped deaths. Margin 6 / min 14 was the least-bad of the
three settings tried on official maps and the best on worldgen, so it's the value kept.

Overall the tweak is not a clean win: it trades official-map trapped deaths for
worldgen-map trapped deaths, and the head-to-head win record is close to a coin flip
in both pools (12-0-10 official, 42-0-38 worldgen across two batches) rather than a
clear improvement. The claim "stricter room checks fix trapped deaths" is **true on
worldgen mazes, false on official maps** — it isn't a universal fix, just a
map-dependent trade a wider margin makes. Kept margin=6/min=14 since it's at least
even overall and best on the maps the original complaint was about (worldgen mazes /
1-wide warrens).
