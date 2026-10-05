# ring-lungeguard-1 (g43-7)

Base: ring-contest-m1 (arena ring-contest-1). One change: long workers stay out of enemy stubs' lunge reach.

## Idea (strategist B, server-r17)
In ring-contest-1's server losses our 8+ dragons die to enemy heads 3.97 times per loss, 3.72 of
them to stubs of length 5 or less that were in view (90 of 108). Exposed() only flags the tiles
next to an enemy head; a stub reaches further with paid steps.

## Change
- `BuildLungeField()` (strategy.cpp): for a non-queen worker of length 6+, mark every free tile in
  this turn's view that an enemy head seen this turn reaches within ceil(Le/4)+Le-2 steps (BFS over
  free tiles). Enemies we'd trade with (WorthTrading) are skipped. If the enemy's body touches the
  view edge, its seen length is only a lower bound, so we add one segment.
- `SafeStep`: when at least one step allowed today is out of every lunge reach, steps in reach
  count as exposed. When no such step exists, today's rule decides.
- plugin.cpp: from r250 the lookahead replaces gst's single step, and the rollout judge can override
  it. Neither knows the lunge field. When their step ends in avoidable lunge reach and gst's own step
  is clear, gst's step is kept. Multi-step lunges of ours are left alone.
- Debug only: LUNGE_DBG counters, and LUNGE_OFF (the base's behaviour with the same counters).

## Why the plugin part was needed (v1 -> v2)
With only the SafeStep change (v1), 6+-long head deaths were unchanged (5.68 v 5.68 per game,
server seeds 1-2). Per 68 games, 235 final steps of 6+ workers went into avoidable reach. About 180
of them came from the lookahead (r250+) and from rollout overrides, which replaced SafeStep's step.
v2 adds the plugin keep and brings that to about 10.
v3 (this one) also counts the hidden segment of edge-touching enemies: 89% of sightings touch the edge.
I also tried "edge-touching = reach 6" (gst-search-5's rule). It gave fewer deaths (6.09 -> 4.34) but
lost a little board control (Voronoi 0.497 v 0.503) and went 31-37, so I dropped it.

## Local numbers (sim, A = this, B = ring-contest-m1; per side per match)
| set | W-L | 6+ head deaths (len) A v B | eats per 6+ turn A v B | Voronoi r100-300 A v B | heads near enemy heads A v B | trades started A v B |
|---|---|---|---|---|---|---|
| server s1-2 | 39-29 | 4.91 (46) v 5.69 (56) | .1150 v .1156 | .501 v .499 | .113 v .113 | 30.8 v 29.2 |
| server 2-starter s3-4 | 15-13 | 0.93 v 0.93 | .114 v .120 | .493 v .507 | .069 v .067 | 10.0 v 10.3 |
| server 3+-starter s3-4 | 20-20 | 6.78 (67) v 8.10 (77) | .111 v .111 | .496 v .504 | .109 v .108 | 35.6 v 35.2 |
| fresh mapgen (44 maps, seed 43007) | 94-82 | lower in 8 of 9 active families | within 5% except off-axis (+24%) and ring (-8%) | .498 avg; off-axis .481 v .519, maze .494 | equal | slightly more |

Fresh by family, W-L: anti-centre 10-6, crowded 8-8, maze 7-9, mixed 8-8, off-axis 9-7, open 11-5,
rich-near-home 8-8, ring 9-7, rooms 8-8, sealed-spawn 8-8, sparse 8-8. No family is a clear loss.

Board control: no sign that we cede the frontier on the server s1-2 or most fresh families. There is
a small Voronoi deficit on server s3-4 (-0.01) and on off-axis (-0.04, though 9-7 with more eats).
Heads near enemy heads and trades started are unchanged.

`check ring-lungeguard-1 maps/official`: passed. Local slowest turns are the same as the base (noise).

## Arena
Not run: the permission system blocked `tokenz ephemeral` and submit, so the orchestrator submits it.
