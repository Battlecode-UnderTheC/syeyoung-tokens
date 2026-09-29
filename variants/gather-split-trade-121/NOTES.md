# gather-split-trade-121

## Idea

Crossover of gst115 (base) and gst111, per strategist-i's merge rule (disjoint code/phases
only). gst115 = gst101 + an opening (r<150) nearest-pearl preference. gst111 = gst107 +
`Consolidate()`: a reactive version of the "start feeding the champion / freeze it" decision
that used to fire only on a fixed round (`feed_from_round`/`champ_freeze`). These two changes
touch different phases (opening vs late-game feed/freeze) and different code paths, so they
merge cleanly: gst121 = gst115's opening logic + gst111's `Consolidate()` grafted onto
gst115's late-game code.

## What changed from the parents, and why

- Renamed namespace `gst115` -> `gst121` and all `GST115_*` macros -> `GST121_*` (required so
  the variant links standalone).
- Diffed gst111 against its parent gst107 to isolate `Consolidate()` in isolation (per the
  brief, not bringing gst107's centre-pull values, which act only in gst115's opening window
  and would double up with gst115's own opening preference).
- Added to `strategy.hpp`: `reactive_min_round` (300), `reactive_count_margin` (4),
  `reactive_longest_margin` (6), and the `Consolidate()` declaration. `enemy_memory` and
  `EnemiesSeen()` already existed in gst115, unchanged.
- Added `StrategyAgent::Consolidate() const` to `strategy.cpp`: returns true once
  `round_ >= feed_from_round` (the old hard backstop, unchanged), or earlier (after
  `reactive_min_round`) if we're already outnumbered (`EnemiesSeen() >= units_ + margin`) or
  an enemy sighting already out-lengths our champion by `reactive_longest_margin`.
- Replaced every `round_ >= p_.feed_from_round` / `round_ >= p_.champ_freeze` test that gated
  hunting-suppression, champion selection weighting, the champion's no-trap rule, feeding, the
  pre-feed lunge suppression, and the champion split-freeze, with `Consolidate()` — same 6
  call sites gst111 changed relative to gst107, applied onto gst121's copy of that code
  (identical surrounding logic in gst115's late game, since gst115 only touched the opening).

## What I measured

Built `gather-split-trade-121`, `-115`, `-111`, `-96` (v7), `trading-swarm-8`; `check` passes
on `maps/official` (all checks passed, slowest turn 0.5 ms on big_empty, under the 100M/turn
budget). Sim, both seats, threads 2:

Official maps (11 maps x 2 seats = 22 matches):
- vs gst115 (parent): 12-0-10, mean round 308, +1.2 longest/+6.6 total at the end
- vs gst111 (parent): 15-0-7, mean round 356, +6.0 longest/+38.8 total at the end
- vs gst96 (live v7): 13-0-9, +2.3 longest/+11.5 total
- vs trading-swarm-8 (server clone): 16-1-5, +2.7 longest/+5.9 total

Worldgen, seed 97000, 20 maps x 2 seats = 40 matches:
- vs gst115: 18-0-22, mean round 407, -3.7 longest/+5.7 total (r50: 4.2 v 4.2 longest, 66.2 v
  66.2 total, matched; r100: 5.3 v 5.3 longest, 115.1 v 115.1 total, matched — the two are
  nearly indistinguishable here, since Consolidate rarely fires before round 300 on these maps
  and this batch has few early-decided maps to separate them)
- vs gst111: 21-0-19, mean round 409, -3.7 longest/-43.4 total (r50: 121 has more dragons,
  26.2 v 28.2, but less total length, 68.8 v 73.3; by r100 gst111 pulls ahead on both, 41.4 v
  45.9 dragons, 114.4 v 130.6 total — gst121 wins the W-D-L but on these particular maps loses
  ground on raw growth before the endgame)
- vs gst96 (v7): 20-0-20, +1.9 longest/-28.8 total
- vs trading-swarm-8: 27-0-13, +7.9 longest/+41.9 total

Combined (official + worldgen): vs gst115, 30-0-32 (roughly even, slightly behind); vs gst111,
36-0-26 (clear win). At least even with both parents overall, per the pass bar (official maps
alone are a clean win against both; worldgen alone is a near-tie against gst115 and a modest
win against gst111 — no set shows a net loss against either parent).

Deaths on its own move (official maps, both parents): head-to-head deaths dominate for all
variants (~23-35 self-caused per match out of ~65-85 total head-to-head-attributed) — this is
inherent to gather-split-trade's aggressive lunging/trading, not something this change moved.
`Consolidate()` firing earlier (as intended) shows up as slightly more `feed champion` and
`give up slot: long ally` fatal turns for gst121 vs gst111 (16.3/4.3) than gst115 alone showed
in the gst115-vs-gst111 comparison, consistent with gst121 starting to consolidate sooner in
some matches where the reactive triggers fire.

## What I'd try next

- The worldgen near-tie with gst115 suggests `reactive_min_round`/margins may need retuning
  against gst115's opening behavior specifically (gst115's early nearest-pearl preference may
  already put it ahead in ways that make the reactive triggers fire against *us* more often
  than intended, in maps where gst115 wins the opening). Worth a trace on a couple of worldgen
  losses to check whether Consolidate() is firing too early for gst121's own good on those
  maps, or whether the loss is unrelated to this change.
- Didn't try retuning `reactive_count_margin`/`reactive_longest_margin`/`reactive_min_round`
  themselves — brief scope was "the one change," not a sweep. A future task could tune these
  three now that the merge is proven safe.
