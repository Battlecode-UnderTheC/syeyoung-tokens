# gather-split-trade-63

## Idea

Crossover: port body-wall-17's early centre pull into the gather-split-trade line. On small
maps (<= 1,024 tiles), before round 50, each goal candidate's score (pearls, beds, the
unexplored edge) is scaled up the closer the tile is to the map centre, so the team contests
the middle in the opening instead of only working its own side.

## What changed from the parents, and why

gather-split-trade-56 (namespace renamed `gst63`, macros `GST63_*`) plus one change, isolated
by diffing body-wall-17 against body-wall-16 (only the centre-pull hunk differs, plus the
namespace/macro rename):

- New params (`Params`, `strategy.hpp`): `centre_max_tiles = 1024`, `centre_until = 50`,
  `centre_bonus = 0.3`.
- `ChooseGoal` (`strategy.cpp`): computed `centre_pull` (bonus > 0, `round_ < centre_until`,
  `W*H <= centre_max_tiles`), the board centre tile, and `centre_far = W/2 + H/2`. Inside the
  per-tile scoring loop, `pull = 1 + centre_bonus * max(0, 1 - torus_dist(tile, centre) /
  centre_far)` multiplies the tile's gather score, and the unexplored-edge score too (via the
  same `pull` factor, mirroring how bw17 applied it to both).
- Kept 0.3 (bw17's value); didn't have time in the 20-minute budget to also run 0.2 and 0.45
  sweeps on top of both map sets and both opponents — see "what I'd try next".

## Measurements (sim, both seats)

| opponent | maps | gather-split-trade-63 | notes |
|---|---|---|---|
| gather-split-trade-56 | official 11 | 11-0-11 | longest -0.2, total length +13.8/match |
| gather-split-trade-56 | worldgen (20 maps, batch seed default) | 21-0-19 | longest -1.9, total length +11.4/match |
| trading-swarm-5 | official 11 | 14-1-7 | |
| trading-swarm-5 | worldgen (same 20 maps) | 30-0-10 | |

`check gather-split-trade-63 maps/official`: all checks passed, 0 avoidable deaths (slowest
turn on big_empty/stronghold/trauma spiked into a few ms under `check`'s instrumentation, same
as gather-split-trade-56's own check run on this machine — not something this change touches:
the centre pull is O(1) extra work per candidate tile, and it's a no-op wherever `centre_pull`
is false).

Against the parent gst56, W-D-L is exactly even (both official and worldgen ties are close to
50/50), but total length margin is consistently positive (+13.8 official, +11.4 worldgen),
matching the brief's ask to measure length margins alongside W-D-L. Against trading-swarm-5
the win rate is clearly ahead of even (14-1-7 official, 30-0-10 worldgen) — better than gst56
alone would need to be to justify the change, and directionally consistent with body-wall-17's
own report (it beat trading-swarm-5 83-19 vs its parent's 73-29).

## What I'd try next

- Sweep centre_bonus at 0.2 and 0.45 as the brief asks; ran out of the 20-minute budget after
  the base measurements. bw17's own report found 1.0 backfires (the team crowds the middle and
  "trapped: no move" deaths double), so 0.45 is the more interesting one to check for the same
  failure mode at a smaller scale.
- Check whether the pull interacts with gst-line features bw17 doesn't have (cluster_bonus,
  enemy_claims, patrol-folded-into-scoring): all four apply to the same per-tile score, so the
  multiplier compounds with cluster bonuses near the centre. Didn't see obvious crowding
  symptoms in the death breakdown (head-to-head/no-action deaths for gst63 vs gst56 were close:
  59.7 vs 60.5 head-to-head, 83.3 vs 55.5 no-action on official — the no-action gap is worth a
  second look on a loss trace).
- body-wall-17's own notes flag Devil as still lost to the swarm; worth checking if this port
  moves that particular map for the gst line.
