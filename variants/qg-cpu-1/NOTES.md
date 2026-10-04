# qg-cpu-1

## Idea
CPU fix only: qg-narrow-1 (arena queen-guard-15) went to 99.8M of the judge's 100M points a turn
(7 overruns in 7 arena games; an overrun kills the wasm instance and the dragon restarts with
no memory). Bring the worst turn under 80M with the same play.

## Where the points went
Instrumented build (the judge's virtual clock counts CPU points, so `clock_gettime` deltas inside
`--sandbox` are exact points), arena game 79300 (big_empty, seed 1592614637, vs ring-plus-7)
and the heavy autarky/maze games: every turn over 30M was **the queen** (`id 0/1`, L8-15, r330+),
and the queen guard (`queen_guard.hpp` `Guard`) took 75-90% of it:
gst 4M, search7 6M, I/O ~3M, guard up to 85M.

- The guard scores every candidate with 16 rollouts x 8 rounds; a queen of length 9+ has sprint
  candidates (all simple paths of 2-3 free steps), so up to ~50 candidates = ~800 rollouts.
- Inside a rollout: the policy's nearest-pearl scan over every pearl in the model (div/mod per
  pearl), and `LungePath`'s visited test, a linear search of its queue (O(n^2)).
- Outside the rollouts: `HeadRisk`'s BFS also had a linear-search seen list (per candidate per enemy).

## What changed (parent qg-narrow-1)
Exact (identical decisions):
1. `Sim::Dist` reads precomputed x/y tables instead of `%`/`/` (both Sims).
2. `Sim::LungePath` and `QueenGuard::HeadRisk` mark visited tiles with an epoch stamp array instead
   of searching the queue/seen list (same set, same order, same caps).
CPU safety net (changes play only on budget turns):
3. A work meter in the rollout policy (16 + L/4 per policy call, + pearls scanned, + lunge nodes;
   about 120 points per unit). In `QueenGuard::Guard`, once the work so far projects the remaining
   candidates past `kWorkBudget` = 250k units (~30M points), the rest get fewer rollouts (min 4)
   and every candidate is scored on the same first R rollouts (common random numbers, so it is a
   fair comparison); past `kWorkHard` = 350k units the remaining candidates are dropped (gst's own
   is still scored). Under budget the loop is the old one, bit for bit.
4. The same budget (without the drop) in `gstg1r::Judge::Review` (non-queen rollout reviews), which
   never came close in the measured games.
No clock reads, no env vars: the meter is a deterministic counter, so the native sim and the judge
agree.

## Measured
Sandbox (`unswbc run --sandbox`, judge metering), our team's points per turn:

| game | base (queen-guard-15) max | qg-cpu-1 max |
|---|---|---|
| arena 80400 wg39019 vs crossover-1 | 99.8M | 35.6M |
| arena 79300 big_empty vs ring-plus-7 | 99.4M | 49.9M |
| arena 78410 server/autarky vs codex-queen-value-1 | 99.3M (arena) | 44.0M |
| arena 76830 big_empty vs queen-guard-7 | 97.6M (arena) | 46.8M |
| arena 80259 wg39019 vs gst-search-12 | 96.2M | 33.4M |
| arena 78742 wg39007 vs gst-search-12 | 96.0M | 39.9M |
| arena 79373 wg39010 vs gst-giant-2 | 92.6M | 45.4M |
| arena 80368 server/maze vs gst-giant-2 | 84.0M (arena) | 32.0M |

Base numbers without "(arena)" were reproduced locally and match the arena's exactly; the qg-cpu-1
numbers for 79300/78410/76830/80368 come from the profiling build (its stderr prints add a little). Sweep vs ring-plus-7, seed 7, all 17
server + 15 official maps: worst turn 94.0M -> 41.5M (big_empty), every other map within 0-2M or
lower; p99 per map unchanged (worst 21.0M both; per-map +-0.6M); 31/32 identical results (the one
change is big_empty, where the base overran).

Exactness: with the budget disabled, the sandbox game 79300 is identical to the base turn for turn
until r386, where the base had overrun its CPU (its instance restarted with empty memory: reported
memory dropped). Native sim vs qg-narrow-1, maps/server, seeds 21-24, both seats: 68-0-68, every
statistic identical on both sides (mirror games).
