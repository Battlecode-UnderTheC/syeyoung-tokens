# qg9-4

qg9-1 (queen-guard-3) with a guard that vetoes gst's queen move for safety only. If gst's own step
is reachable by no enemy head (risk 0), has room, and its rollouts kill the queen at most once more
(out of 16) than the best candidate's, the guard keeps it. Before, the guard picked by rollout
value: 8 rounds of a random policy, so its material term is myopic and it walked the queen off
gst's foraging route.

## Diagnosis (sim, stripes + weakhold, seeds 11-40, 120 games vs gst-search-8)
- The guard alone causes most of the small-map gap: dragons at r100 6.9 v 13.9 with it, 9.7 v 11.4
  without; queen mode off mirrors gst-search-8 exactly.
- Pasture off, allies not yielding, and the queen removed from allies' sharing/fan-out/crowd lists
  each recover little.
- Per-dragon logs: at r30 the guarded queen plus the rest of the team are 9.6 long v 11.7
  unguarded; the queen's splits fall from 212 to 123 (r<40, 120 games). The queen eats less, so
  it supplies fewer children.
- Guard overrides before r150 (10 seeds): 1110 by rollout value, 363 by risk, 94 by crowd,
  10 by room. Only 10 of the overrides moved the queen off a pearl tile.

## Results
| | small maps (120) | queen died (small) | server maps seeds 1-16 (320) | queen died (server) | queen-decided (server) |
|---|---|---|---|---|---|
| qg9-1 | 41-79, r100 dragons 6.9 v 13.9 | 74% | 166-2-152 | 81.5% | 38-0 |
| qg9-4 | 47-73, r100 dragons 8.3 v 13.1 | 81% | 169-0-151 | 82% | 36-1 |
| slack 0 | 35-85, r100 dragons 6.8 v 13.8 | 76% | - | - | - |

