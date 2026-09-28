# counter-swarm-30

crossover: counter-swarm-29 (base, namespace renamed cs29 -> cs30) x late-surge-23 (lunge guard).

## Idea

counter-swarm-29's SafeStep only avoids tiles adjacent to an enemy head it won't trade with
(`Exposed`). late-surge-23 added a softer guard for long dragons: a tile is also worse than safe
(but still better than a sure trade) if an enemy head in view could reach it in one multi-step
MOVE of up to `min(enemy_length - 1, lunge_reach)` free steps. A replay study on late-surge-18
found 1,032 length of dragons >= 10 long lost to enemy lunges over 16 games, so this guard is
ported in for our dragons of length >= 10 and the frozen champion.

## What changed from the parents, and why

- `cs29` -> `cs30` namespace and `CS29_*` -> `CS30_*` macros throughout (renamed, no logic
  change there).
- Added `Params::guard_length = 10` (reused the existing `lunge_reach = 6` param, already
  present in cs29 for the attack-side lunge search).
- Added `lunge_threat_` (an `unordered_set<int32_t>`) and `MarkLungeThreat()`, ported near-verbatim
  from `ls23.cpp`: once a turn, BFS over free tiles from each enemy head seen this round (skipping
  heads we'd trade with via `WorthTrading` and enemies of length <= 2), out to
  `min(enemy_length - 1, lunge_reach)` steps. Only runs when our length >= `guard_length` or we
  are the frozen champion (`round_ >= champ_freeze && champ.id == spawn_.id`); otherwise clears
  and returns early (no BFS cost for short/normal dragons).
- `SafeStep`'s ranking key changed from a boolean `exposed` to a 3-level `expo` (0 safe, 1
  lunge-reachable, 2 next-to-head), same as `ls23`. The fast-path early returns (`toward goal`,
  `into a dead end worth it`) still gate only on the strict `Exposed()` bool, unchanged from cs29
  — only the ranking among candidate steps (used when no fast path fires, e.g. `detour`/`no goal:
  roomiest`) uses the new 3-level key. cs29's existing hard `guard_champion`/`threat_` veto
  (champion-only, from round 340, full board reach) is untouched; the two guards can overlap for
  the champion, which is fine (the hard veto simply removes more options before the soft ranking
  sees them).

## What I measured

`build/tools/check counter-swarm-30 maps/official`: all checks passed, slowest turn scale
unchanged from cs29 (both dominated by the simulator's own per-turn audit cost on big maps, not
per-agent CPU — well within the 0.5 ms per-agent budget in both).

vs counter-swarm-29 (parent), official maps, both seats, threads 2:
`10-0-12` over 22 matches. Mean margin: longest -1.2, total -0.7 (noise-level on 11 maps x 2
seats). Lunged-by-enemy of length 10+: cs30 1.8 vs cs29 2.2 (per match).

vs counter-swarm-29, 20 fresh worldgen maps, both seats:
`18-0-22` over 40 matches. Mean margin: longest +0.2, total -8.2. Lunged-by-enemy of length 10+:
cs30 1.9 vs cs29 2.4 (per match) — about a 20% cut in the targeted stat, matching the intended
effect, while overall W-D-L stayed essentially even with the parent (as expected: the loss study
was specifically about long-dragon lunge deaths, a small slice of total deaths, so it's not
expected to swing the macro win rate much on its own).

vs trading-swarm-5, official maps, both seats:
`15-0-7` over 22 matches — a clear win, same ballpark as counter-swarm-29 normally gets against it.

## What should improve

Fewer of our length-10+ dragons and the frozen champion should die to enemy lunges specifically
(confirmed: ~20% fewer per match on worldgen maps). Overall W-D-L against the parent is even
(not a regression), so this is a safe, targeted port rather than a net strength change by itself.

## What I tried that didn't work

Nothing else attempted — single focused change per the brief's short budget; did not sweep
`guard_length` or `lunge_reach`, and did not touch the existing hard champion `threat_` veto.
