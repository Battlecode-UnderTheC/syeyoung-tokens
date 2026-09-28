# body-wall-17

## Idea

On small maps (<= 1,024 tiles) the swarm reaches the central field first and out-eats us
in the opening. Before round 50, pull goal choice (pearls, beds and the unexplored edge)
towards the map centre, so our dragons contest the middle, not only their own side.

## What changed from the parents, and why

body-wall-16 plus one thing (namespace renamed to `bw17`, macros `BW17_*`):
- `ChooseGoal`: while `round < centre_until` (50) on maps with `W*H <= centre_max_tiles`
  (1,024), each candidate's score, and the explore-edge score, is multiplied by
  `1 + centre_bonus * (1 - torus_dist(tile, centre) / (W/2 + H/2))`, with centre_bonus 0.3.
- Big maps (> 1,024 tiles) are unchanged. The sim results on big_empty, schooltime, stronghold
  and trauma match the parent's exactly.

A split at length 3 is illegal (SPLIT n needs 2 <= n <= length-2, so length >= 4), and
split_min_length is already 4. The "split earlier" half of the focus can't be done.

## Measurements (sim, both seats)

| opponent | maps | body-wall-17 | body-wall-16 |
|---|---|---|---|
| body-wall-16 | official 11 | 11-0-11 | |
| trading-swarm-5 | official 11 | 19-0-3 | 15-0-7 |
| body-wall-16 | worldgen 91401-420 | 19-0-21 | |
| body-wall-16 | worldgen 92500-519 | 24-0-16 | |
| trading-swarm-5 | worldgen 91401-420 | 32-0-8 | 27-0-13 |
| trading-swarm-5 | worldgen 92500-519 | 32-0-8 | 31-0-9 |

Against trading-swarm-5 on the official maps, the gain is on small maps: Colosseum 2-0
(parent 0-2), default 2-0 (1-1), default_small 2-0 (1-1).
Devil is still lost in both seats. Team at r50: seat A 10 dragons, length 25, vs the swarm's 17 and 39
(parent: 8 and 20 vs 21 and 49); seat B 10 and 21 vs 19 and 51 (parent: 11 and 25 vs 19 and 48).
Devil's gap-1 beds sit in the centre and on the wrap rows (y=0/15), and the swarm still
eats them faster from r13 on.

## Tried and didn't work

- centre_bonus 1.0: 8-0-14 against body-wall-16 on the official maps. "Trapped: no move" deaths
  doubled (52 vs 28 fatal turns per match), and it lost both seats on Colosseum,
  default_small, default and queen_of_spades. A strong pull piles the team into the same
  crowded middle.

## What should improve / next

Small-map openings against swarms. Next: work out why the swarm out-eats us on Devil from
r13 (it sits on the gap-1 beds, while ours count a bed that is due as 1.0/(ready+1) like any other).
Try valuing a bed by its respawn rate (the gap range, if the observation exposes it), and
try a centre defined by the spawn mirror rather than the (W/2, H/2) tile.
