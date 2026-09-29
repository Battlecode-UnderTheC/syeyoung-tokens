# counter-swarm-40

Parent: counter-swarm-38 (mutate). Task g21-6. Namespace renamed cs38 -> cs40 (files
cs40.cpp/cs40.hpp), CS38_* macros -> CS40_*.

## Idea

strategist-f-r3 (P2): our biggest loss driver in the live-server replays is not the enemy but
self-inflicted congestion -- `no_valid_action:resign` and `head_to_head:mover` deaths from a
packed team, at 4-19x the opponent's rate on several maps. gst96's fix (`fan_out`) only
discounts goal tiles near other allies' heads, only before round 80, and never touches the
actual step chosen. cs38 has no direct equivalent of `fan_out`/`fan_out_until`: its analogous
mechanism is `ChooseGoal`'s `share_factor` discount (score *= share_factor when another ally is
nearer to a candidate tile) -- but that one is *not* round-gated at all in cs38 (only gated by
leader/champion role), so there was no hard cutoff to extend there. The gap strategist-f-r3
actually found is real in cs38 too: `SafeStep`'s tile-choice tiebreak (`key()` at cs40.cpp
~L843) never looks at ally density, so among otherwise-equal candidate steps it can walk a
dragon deeper into an already-packed neighbourhood, causing exactly the collisions/resigns P2
targets.

## What changed from the parents, and why

- New `AllyDensity(tile, radius)`: counts our own occupied tiles (body via `Mine`, or
  `board_.at().occupant_team == spawn_.team`) within a diamond radius (same shape as the
  existing `cluster_bonus` scan, so no new board-scan pattern).
- New params: `fan_radius=2`, `fan_floor=0.3`, `fan_decay=300.0`.
- `SafeStep`'s `key()` tuple gets a new last element `-dens`, where `dens = AllyDensity(to,
  fan_radius) * fan_weight` and `fan_weight = max(fan_floor, 1 - round_/fan_decay)`. Since it's
  the last (weakest) tuple element it only breaks ties among candidates the existing metrics
  (doom/exposure/escape level/liberties/turn-out count/room) already rank equal -- it doesn't
  override any existing decision, just picks the less-crowded of otherwise-equivalent steps.
  The round-decaying weight narrows the tiebreak's resolution (via truncation) as the game goes
  on rather than shutting it off at a hard round like `fan_out_until=80`: it matters most while
  the opening pack is forming and keeps some effect through the mid/endgame instead of
  disappearing at r80, which is exactly what P2 asked for ("extend beyond the opening").

## Measurements

`build/tools/check counter-swarm-40 maps/official`: all checks passed, 0 avoidable deaths.

vs parent (counter-swarm-38), official maps, both seats, threads 2:
- **13-0-9** W-D-L over 22 matches. At end: cs40 leads by +2.9 longest, +4.2 total (mean).
- r50: cs40 9.3 dragons/3.2 longest/22.9 total vs cs38 10.2/3.3/25.3 (slightly fewer dragons,
  same longest -- consistent with less pile-up, not less growth).
- r100: cs40 17.6/3.9/46.2 vs cs38 17.8/3.6/46.3 (about even count, cs40 ahead on longest).
- deaths/match: cs40 no-action 84.0, head-to-head 47.9 vs cs38 no-action 86.5, head-to-head
  47.7 -- a small drop in resigns, roughly flat on mutual collisions; the effect is modest but
  in the direction P2 predicts.

vs parent, 20 fresh worldgen maps (seed 90000, both seats):
- **25-0-15** W-D-L over 40 matches. End: +6.3 longest, -8.0 total (mixed on total, ahead on
  longest, which is the primary win condition).
- r50: cs40 28.1/4.0/70.2 vs cs38 27.9/4.0/69.8 (even). r100: cs40 44.4/4.7/117.5 vs cs38
  42.9/4.7/113.9 (cs40 slightly ahead on both dragons and total).
- deaths/match: cs40 no-action 532.2, head-to-head 206.3 vs cs38 no-action 503.2, head-to-head
  205.9 -- no-action is actually *higher* here at n=40 map-pairs; this metric is noisy
  match-to-match (per-map variance dwarfs the ~30/match gap) and the W-D-L/longest/total swing
  is the more reliable signal at this budget. Flagging this for the next task to watch with
  more samples.

vs gather-split-trade-96 (live v7 parent lineage), official maps, both seats: **12-0-10**.
vs trading-swarm-8 (server-opponent clone), official maps, both seats: **17-0-5**.

Ran out of budget before a clean per-map breakdown by category (Trophy/Slithery-like/Devil/
Schooltime vs default worldgen) for the non-parent opponents; the aggregate W-D-L above is
solidly positive for both.

## What should improve

Fewer packed-step collisions/resigns in the crowded mid-late game (Trophy/Slithery/Devil-style
maps per strategist-f-r3), without giving up eating pace, since the tiebreak only fires among
already-equal candidates.

## What didn't work / wasn't tried

- Did not extend `ChooseGoal`'s `share_factor` past a round cutoff: cs38 never had one, so
  there was nothing to extend there; all the change is in `SafeStep`.
- Did not implement P1 (resign+mover counters printed by `sim`) or P3/P4 (trade-into-stub-ally,
  split-cluster-cap) -- out of scope for this task, but P3/P4 are natural next steps building on
  this same `AllyDensity` helper (e.g. skip a growth split when `AllyDensity(head_, 2)` is
  already high).
- Didn't sweep `fan_radius`/`fan_floor`/`fan_decay`; picked values by analogy with cluster_radius
  (2) and gst96's fan_out_until order of magnitude (300 as "well past 80").

## Next steps

- Add strategist-f-r3's P1 telemetry (resign/mover rate print in `sim`) so the next tuning pass
  can judge this change's effect on the self-crowding metric directly instead of only via W-D-L.
- Consider P4 (cap simultaneous splits per local cluster) using the same `AllyDensity` helper.
- Re-run the worldgen comparison at n=40+ maps or more seeds; the no-action death count was
  noisy enough at this sample size to not confirm the mechanism, even though W-D-L/longest
  improved.
