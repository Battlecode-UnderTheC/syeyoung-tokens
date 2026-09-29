# gather-split-trade-126

Operator: mutate. Parent: gather-split-trade-121 (namespace renamed gst121 -> gst126, incl.
GST121_* -> GST126_* macros, to avoid clashing when linked alongside other variants).

## Idea

strategist-k-r5's P3: when we lunge, prefer the opponent's longest reachable dragon over
whatever target of opportunity the BFS meets first. Diagnosis was that the opponent lunges
our longest dragon selectively while we lunge back at anything reachable, so even when we do
land lunges we aren't removing their biggest threat.

## What changed from the parent, and why

`StrategyAgent::Lunge()` (strategy.cpp, ~L1009) did a BFS out to `length_-1` steps and returned
as soon as it met the first `WorthTrading` enemy head. I changed it to keep exploring (still
bounded by `reach` and the existing 256-node cap) and collect every reachable, worth-trading
enemy head as a candidate, along with the enemy's last-seen length (via `seen_`, the same map
`WorthTrading`/`Exposed` already use). After the BFS, pick the candidate with the greatest
enemy length, tie-broken by shortest distance (closest of equally-long targets), then
reconstruct the move steps for that candidate exactly as before. No other lunge logic changed:
`WorthTrading`'s trade-margin/outnumbered gating still decides *whether* to lunge at all; this
only changes *which* reachable head we pick once we've decided to.

## What I measured

Built `gather-split-trade-126`, `gather-split-trade-121`, `gather-split-trade-96`,
`trading-swarm-8`; `check gather-split-trade-126 maps/official` -> all checks passed (slowest
turn 0.6ms on big_empty, same ballpark as the parent architecture; well under budget on the
maps that matter for combat: devil 0.2ms, schooltime 0.3ms, trophy 0.1ms).

`sim --threads 2`, both seats, mean of A/B:

- vs gather-split-trade-121 (parent), maps/official: **11-0-11**, dead level (0.0 longest, 0.0
  total margin at the end). Deaths-by-cause identical to the decimal between the two sides
  (11.2 lunged-short, 0.6 lunged-of-10+ each) — on these 11 official maps the change essentially
  never had more than one reachable candidate at once, so it's a no-op there.
- vs gather-split-trade-96 (live v7), maps/official: **13-0-9**, gst126 leads by +2.3 longest /
  +11.5 total at match end; r50/r100 dragon-count and total-length both favour gst126 (11.3 vs
  9.3 dragons, 28.5 vs 23.0 total at r50; 22.5 vs 17.8 dragons, 57.2 vs 43.2 total at r100).
  Lunged-of-10+ deaths: 1.0 (us) vs 0.7 (them) per match — no regression there.
- vs trading-swarm-8, maps/official: **17-1-4**, +4.3 longest margin.
- vs gather-split-trade-121, 20 fresh worldgen maps (seed 103000), both seats (40 matches):
  **20-0-20**, gst126 leads by +1.4 longest / +4.4 total at match end. Lunged-of-10+ per match:
  1.7 (gst126) vs 2.0 (gst121) — a small improvement in line with strategist-k's expectation
  (fewer of our long dragons eaten because we're now killing their long dragons instead of
  letting them keep growing). r50/r100 dragon counts and totals are identical between the two
  (as expected: this change only fires inside `Lunge()`, after everything else is unchanged).

Per strategist-k's per-length-band ask: `sim`'s existing breakdown only splits "short" vs "10+"
lunge-victim length, not the full 6-12 mid-band; within that limit, 10+ deaths (the closest
proxy) did not regress on any matchup and improved slightly on worldgen and vs gst96.

## What should improve

More lunges landing on the opponent's longest dragon rather than a short worker, which should
reduce how long their champion survives to run the r200-350 corpse-feast cycle strategist-k
documented (VISUAL #31/#32). This is a small, narrow change (P3 only, not P1's SafeStep
re-ranking or P2's consolidation) — it doesn't touch exposure avoidance or formation play, so
it shouldn't regress anything else, and the measurements bear that out (no regression anywhere,
modest gains vs gst96/worldgen/trading-swarm-8, exact parity vs the parent on official maps
where lunges are rare enough that ties with a single candidate dominate).

## What I tried that didn't work / didn't get to

Didn't attempt P1 (SafeStep exposure re-ranking) or P2 (targeted consolidation) — out of scope
for this task and P3 alone budgeted at ~15 min by strategist-k. Didn't add a length-6 minimum on
which candidates count (P3's own text mentions "length >= 6" as the enemy's own-threat bar) since
`WorthTrading` already filters out lunges not worth taking; adding a further floor risked
skipping a worthwhile lunge on a small map with only short dragons in reach. If a future task
wants sharper targeting, worth trying: weighting by (enemy length - distance) rather than a pure
length-then-distance tie-break, since a slightly shorter but much closer long dragon may be a
better catch within the same turn.

## Status

`build/tools/check gather-split-trade-126 maps/official` says "all checks passed". At least
even with both parents (dead level vs gather-split-trade-121, ahead vs gather-split-trade-96
and trading-swarm-8) on official and worldgen maps. Ready to submit.
