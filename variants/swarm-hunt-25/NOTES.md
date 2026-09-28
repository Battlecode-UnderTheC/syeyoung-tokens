# swarm-hunt-25

## Idea

Crossover: port body-wall-17's early centre pull into swarm-hunt-24. On small maps
(<= 1,024 tiles), before round 50, pull `ChooseGoal`'s candidate scores (pearls, beds, and
the unexplored-edge score) towards the map centre, so the swarm contests the middle instead
of only farming its own side.

## What changed from the parents, and why

swarm-hunt-24 plus the one change (namespace renamed `swh24` -> `swh25`, macros `SWH24_*` ->
`SWH25_*`), in `hunt.cpp`'s `HuntAgent::ChooseGoal`:
- Added `centre_bonus` (0.3), `centre_until` (50), `centre_max_tiles` (1024) to `Params`
  (`hunt.hpp`), same defaults as body-wall-17.
- While `round_ < centre_until` on maps with `width*height <= centre_max_tiles`, every
  candidate's score and the explore-edge score are multiplied by
  `1 + centre_bonus * max(0, 1 - torus_dist(tile, centre) / (W/2 + H/2))`, `centre` being
  `board_.Tile(W/2, H/2)`.
- Checked the mirrored frame the brief flagged: team B plays through `MirrorAgent`, which
  turns side B's whole `Observation` (tiles, edges, segments) into side A's frame *before*
  `HuntAgent::Turn` ever sees it — `board_` is built entirely from that shared frame. The
  180-degree rotation used is centred on the map's own centre, so `(W/2, H/2)` is (up to a
  one-tile rounding artifact on even dimensions) a fixed point of the rotation: pulling
  towards `board_.Tile(W/2, H/2)` in the internal frame already points at the true map
  centre for team B too. No extra correction was needed; HuntAgent's board coordinates are
  identical for both seats.

## Measurements (sim, both seats)

| opponent | maps | swarm-hunt-25 | notes |
|---|---|---|---|
| swarm-hunt-24 | official 11 | 13-0-9 | leads by 2.1 longest, 13.2 total at match end |
| swarm-hunt-24 | worldgen (batch of 20, fresh seed) | 23-0-17 | leads by 3.4 longest, 21.6 total |
| trading-swarm-5 | official 11 | 16-0-6 | |
| trading-swarm-5 | worldgen (same batch) | 30-0-10 | |

Given the time budget only one worldgen batch of 20 was run (not the two the brief asks
for); numbers are consistent with the brief's cited body-wall-17/gather-split-trade-63/
late-surge-27 results (small positive gain vs parent, bigger gain vs trading-swarm-5), so
the direction looks right even with less sampling than ideal.

`check swarm-hunt-25 maps/official` passes with 0 avoidable deaths on all official maps.
Per-turn wall-clock in `check` was noisy (0.1-45ms across two runs on this shared machine)
but that tracks machine load, not CPU points: the added work per turn is O(board size),
same order as the rest of `ChooseGoal`, and both runs still said "all checks passed".

## What should improve / next

Small-map openings, same as body-wall-17: more contested middle ground and pearls in the
first ~50 rounds. Kept `centre_bonus` at 0.3 (not swept 0.2/0.45 given the time budget;
body-wall-17's own sweep found 1.0 backfires by crowding the team into the middle, so 0.3
is likely near the safe end already). Next: try the strength sweep (0.2, 0.45) properly,
and body-wall-17's own follow-up idea of valuing beds by their respawn rate rather than
`1/(ready+1)` for all beds alike, to close the remaining Devil-map gap.
