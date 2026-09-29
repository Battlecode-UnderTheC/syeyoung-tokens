# counter-swarm-41

Task g24-5. Operator: crossover. Base: counter-swarm-40 (namespace cs40 -> cs41, CS40_* -> CS41_*,
files cs41.cpp/cs41.hpp). Donor: gather-split-trade-115.

## Idea

Port gst115's opening nearest-pearl preference (the g22-1 change, isolated by diffing gst115
against gst101) onto the counter-swarm line: in the opening, a plain greedy "nearest pearl"
goal out-throughputs the careful crowd/share/contest-scored ChooseGoal.

## What changed from the parents, and why

- New params (cs41.hpp): `opening_end` 150, `trap_opening_cost` 0.5, `opening_pearl_radius` 40,
  `opening_pearl_factor` 1.0 (same values as gst115).
- `ChooseGoal`: the main per-tile scan also tracks the nearest reachable, unbanned pearl with
  positive value within `opening_pearl_radius` (score = TileValue(t, 0) / (d + 1), cs's
  equivalent of gst115's `base[t]/(d+1)`; cs only builds `base[]` when cluster_bonus > 0, so it
  calls TileValue directly, pearl tiles only). After the dead-end check of the top candidates
  and before the patrol fallback: while `round_ < opening_end`, if nothing scored or the best
  scored candidate is below that pearl's plain score * factor, the pearl becomes the goal.
  Unlike gst115, an occupied pearl (value 0) is never taken.
- Dead-end cost: `trap_opening_cost` instead of `trap_cost` in the opening (the cs trap_loss
  pricing still overrides it for length >= 4 / grow cases, as in gst115).
- Otherwise cs40 unchanged. `check`: all checks passed, 0 avoidable deaths; big_empty slowest
  turn 8.0 ms under check's audit (cs40: 6.3 ms), others <= 0.3 ms.

## Measurements (both seats; worldgen = 20 maps, seed 109000)

| opponent | official | worldgen |
|---|---|---|
| counter-swarm-40 (parent) | 13-0-9 | 27-0-13 |
| gather-split-trade-115 (parent) | 9-0-13 | 12-0-28 |
| gather-split-trade-96 (v7) | 12-0-10 | 20-0-20 |
| trading-swarm-8 | 16-0-6 | 29-0-11 |

Baseline for the gst115 row: cs40 vs gst115 is 10-0-12 official, 15-0-25 worldgen (same maps),
so the port doesn't close the lineage gap to gst115 (slightly worse, within noise at n=40).

- vs cs40 official: r50 10.1 dragons/L3.4/T24.6 vs 9.8/3.2/23.9; r100 19.2/3.8/49.0 vs
  18.3/3.7/46.3. No-action deaths 83.3 vs 110.0/match (all on own move); trapped-L2-3 fatal
  34.2 vs 57.8. The fewer "no goal: roomiest" turns (68 vs 178) show the pearl fallback filling
  goal-less turns.
- vs cs40 worldgen: end +8.0 longest, +33.4 total, but r50 31.8/4.1/80.4 vs 34.0/4.2/83.7 and
  r100 44.8/4.5/114.1 vs 48.0/4.8/123.0 (we're behind early and ahead at the end). No-action 559 vs
  489; trapped-L2-3 228 vs 220.
- vs gst115 worldgen: r50 29.4/4.0/73.8 vs 35.1/4.9/93.1; r100 44.5/4.5/112.1 vs 50.0/5.5/132.3.
  No-action 413 vs 658; trapped-L2-3 209 vs 277.
- vs gst96 worldgen: r50 T74.5 vs 86.2, r100 T109.9 vs 120.8; trapped-L2-3 191 vs 256.
- vs ts8 worldgen: r50 T84.5 vs 64.8; r100 T134.5 vs 83.1.

## What should improve

Fewer idle and roomiest-step turns in the opening and fewer trapped-L2-3 deaths on official maps.
It beats counter-swarm-40 clearly on both map sets.

## What didn't work / open questions

- It doesn't beat gst115 itself: the cs line still trails gst115 on worldgen r50/r100 totals.
  The port helped cs40 but didn't close that gap.
- On worldgen, the port lowered cs's r50/r100 totals slightly against cs40 even though it won
  more games, so the win likely comes from the endgame or safety rather than faster opening eating.
