# ring-bedlearn-1 (g40-6): exact bed timing from observed countdowns

Base: ring-sched-6 (code identical to variants/ring-sched-m1). Queen unchanged.
Scope was narrowed by the user mid-task: no per-bed min/max/mean gap estimator,
no symmetry mirroring, no sonar sharing. Only use the countdowns we actually saw, exactly.

## Audit: where the parent blurs countdown information
Engine facts (shared/engine/dragons/game.cpp `BeginRound`, docs/game.md): a bed read with
countdown c in round s fires at the tick of round s + c. At every tick it redraws its gap,
whether the pearl spawned or was blocked by a dragon on the bed. A head's k-th step happens in
round now + k - 1, so `dist` steps get the new pearl only when dist >= due + 1 (due = s + c - now).

1. `TileValue`, `ChooseGoal` (both scoring passes), the cluster bonus and the scheduler's
   `ready_of` all used `max(dist, due)` (or `dist + wait*(due - dist)`). That is one round early.
   A dragon that arrives one round early is on the bed at the tick: the pearl is blocked, the
   timer redraws, and the whole wait is lost.
2. The movement has no idea of timing: a worker whose goal is an unripe bed walks straight onto it
   and stands there. If due <= its length, its body blocks the tick.
3. A remembered bed whose countdown ran out out of view got a flat `ripe_bed_value = 0.6`. That
   ignores how long ago it fired, which is known exactly.
4. The enemy-contest check used the same early arrival (`max(torus, due)`, tie at `ready == due`).
5. `hot_gap` (UpdateHot) is a fixed threshold on the reset countdown. Left alone, because the user
   asked for exact use of observed countdowns, not gap estimation.
6. `search7.cpp` and the rollout judges model beds separately. Left alone (late game and queen).

## Change (BEDTIME_MODE=1; 0 restores the parent exactly)
- `BedReady(dist, due, wait)`: arrival is counted against the exact tick (due + 1 steps). Used by
  TileValue, both ChooseGoal passes, the cluster bonus, the scheduler (wait 0) and the enemy
  contest (`theirs`, and a tie at `ready == due + 1`).
- An expired bed seen out of view is worth `pearl_decay^(rounds since it fired)`, the same
  per-round "still there" chance a remembered pearl gets, instead of a flat 0.6.
- Camp, don't block: next to a goal bed that is still to fire, the worker takes the free step
  that stays nearest to it (SafeStep may not step onto the bed: `avoid_tile_`). If no other move
  exists it steps on as before. Reason "bed: wait for its tick". The loop breaker does not ban such
  a goal, and the plugin's late lookahead step does not override it. Each turn ChooseGoal still
  decides whether waiting beats everything else (unchanged wait_factor cost).
- No new tuned numbers.

## Local
- Debug counters (GM2_DEBUG, maps/server seed 1, 34 games vs ring-sched-m1): steps onto a bed whose
  tick our own body would then block went from 4604 (mode 0) to 2777. The rest come from paths that
  only cross beds and from judge overrides. 5322 bed-wait steps.
- maps/server seed 1 vs ring-sched-m1: 17-0-17 (mirror is 17-17). At r100 the total length was
  60.9 vs 58.1 and the dragon count 24.4 vs 23.2.
