# ring-sched-port

ring-sched-1 plus the two ring-plus changes made after ring-sched branched off at ring-plus-16. Both were applied as their exact diffs with `patch`:

- **Cap-aware growth** (ring-plus-19, from `diff ring-bud-cpu ring-capgrow-1`): within 4 of UNIT_LIMIT, workers of length 4+ make no growth split. Locally it went 140-130 (server) and 125-113 (official).
- **Guarded queen lunge** (ring-plus-21, from `diff ring-capgrow-1 ring-qlunge-2`): RING12_ATTACK_MODE 1, with paid steps only when we have 4+ non-queen dragons. Neutral overall (P(win) 0.50), but on 3+-starter server maps queen-decided games went 31-20. Top teams kill more enemy queens (evals-r13).

The arena parent is ring-sched-1.
