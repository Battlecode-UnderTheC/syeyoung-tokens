# body-wall-23

## Idea

Crossover of body-wall-21 (base: idle-stub give-up + body-wall-19's room gate, the league's
consolidation leader) and body-wall-17 (the early centre pull). body-wall-21's line wins by
consolidating into one long champion, but without a centre pull it cedes the central field on
small maps and lost Trophy to trading-swarm-5. body-wall-17's pull won in all five lines it has
entered (league/KNOWLEDGE.md), so port it as-is.

## What changed from the parents, and why

body-wall-21 (namespace renamed `bw21` -> `bw23`, macros `BW21_*` -> `BW23_*`), plus body-wall-17's
centre pull ported verbatim (diffed against body-wall-16 per the brief):
- Three new params in `Params`: `centre_max_tiles = 1024`, `centre_until = 50`,
  `centre_bonus = 0.3` (also wired into the `BW23_P` tuning-build override list).
- In `ChooseGoal`: while `round_ < centre_until` on maps with `width*height <= centre_max_tiles`,
  each candidate's score (both the pearl/bed score and the unexplored-edge score) is multiplied
  by `1 + centre_bonus * (1 - torus_dist(tile, centre) / (W/2 + H/2))`, where `centre` is the
  `(W/2, H/2)` tile. Nothing else changed: body-wall-21's idle-stub give-up and room gate are
  untouched, and the pull is inert on big maps (unchanged behaviour there).

## Measurements (sim, both seats, --threads 2)

| opponent | maps | body-wall-23 | note |
|---|---|---|---|
| body-wall-21 | official 11 | 11-0-11 | total margin +15.2 for bw23; mean 302 rounds |
| trading-swarm-5 | official 11 | 17-0-5 | |
| body-wall-21 | worldgen pool 1 (20 maps) | 23-0-17 | |
| body-wall-21 | worldgen pool 2 (20 maps) | 20-0-20 | |
| trading-swarm-5 | worldgen pool 1 (20 maps) | 28-0-12 | |
| trading-swarm-5 | worldgen pool 2 (20 maps) | 28-0-12 | |

`check body-wall-23 maps/official`: all checks passed, 0 avoidable deaths; slowest turn 0.1ms
except big_empty/stronghold/trauma/trophy, which match body-wall-21's existing numbers (the
pull is a no-op on those large maps).

At least even with the parent everywhere measured, and clearly ahead of trading-swarm-5 (a
material jump from body-wall-21's presumed weaker showing there, matching body-wall-17's own
report of gains specifically against the swarm on small official maps).

## What should improve

Small-map openings against swarms (Colosseum, default, default_small, arena), where
body-wall-17 previously reported the pull's gain concentrates. Untested here beyond the
aggregate W-D-L; a per-map breakdown against trading-swarm-5 would confirm it's not just
carried by a couple of maps.

## Tried and didn't work

Nothing else attempted; the brief scoped this task to a single verbatim port, budget ~20
minutes.
