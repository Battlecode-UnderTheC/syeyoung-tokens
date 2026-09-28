# gather-split-trade-3

## Idea

gather-split-trade-1, plus: keep out of enemy *lunge* reach. A MOVE may take up to
length-1 steps, so an enemy head of length L reaches any tile up to L-1 free steps away in
one turn; the parent only kept off tiles next to enemy heads.

## What changed from the parents, and why

- Namespace `gst1` -> `gst3`; the getenv tuning block removed (no runtime knobs).
- `MapLunges()` (once per SafeStep): for every enemy head seen this round, a bounded BFS
  over free tiles up to `min(length-1, lunge_cap=8)` steps (their seen length, a lower
  bound). Skipped when reach < 2 (adjacent heads are `Exposed()`'s job) or when the trade is
  clearly ours (`WorthTrading` and they are strictly longer). Even-length enemies are
  avoided too: we see only part of their body, and the first version (avoid only enemies we
  won't trade with) barely ever fired.
- SafeStep: the goal step is taken outright only if it is not in lunge reach. Candidate key
  is now (not adjacent-exposed, not doomed, not in lunge reach, level, liberties, count,
  room). If every live step is in lunge reach, take the goal step anyway ("toward goal (in
  lunge reach)") - prefer to avoid, don't freeze. Deviations are tallied as "dodge lunge".
- Slowest turn in `check`: 0.2 ms (parent ~0.1-0.2).

## What I measured (sim, both sides of each map)

| opponent | maps | gst3 (final) | gst3 (first try: skip all tradeable) | parent gst1 |
|---|---|---|---|---|
| swarm-hunt-1 | official (22) | 16-0-6 | 16-0-6 | 14-0-8 |
| swarm-hunt-1 | worldgen 20 (40) | 22-0-18 | 15-0-25 | 18-0-22 |
| gather-split-trade-1 | official | 10-0-12 | 10-0-12 | - |
| gather-split-trade-1 | worldgen 20 | 20-0-20 | 19-0-21 | - |

swarm-hunt-1's lunge deaths (= trades with us) per match: official 12.7 (vs gst1) -> 10.9;
worldgen 26.5 -> 26.2. So the avoidance fires rarely ("dodge lunge" ~2-3 per match) and
changes little; most head-to-head losses are adjacent "strike"s and our own kamikazes,
all one-for-one trades among length 2-4 dragons.

## What should improve

Slightly fewer even trades forced on us by lunging opponents. Expect roughly parent-level
results; the gain is small and within noise.

## Tried, didn't work

- Avoiding only enemies we won't trade with (parent's Exposed rule extended): fired ~1
  time per match, worldgen vs swarm-hunt-1 15-25.
