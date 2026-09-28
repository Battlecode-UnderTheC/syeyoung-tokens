# gather-split-trade-12

Mutation (task g4-3) of gather-split-trade-5. Focus: side asymmetry (turn order).

## Mechanism found

gst5 vs itself (deterministic) splits every official map 1-1, but not by side: A won 4 maps
(arena, default_small, trauma, trophy), B won 7. So there is no general first-mover edge.
The arena trace: trades are all 1-for-1 head-to-heads (whoever strikes first, both die), and
the gap opens in growth (rounds ~10-23, A goes 5 -> 10 dragons while B stays at 5), i.e.
the pearl race. Reasoning from the rules: at the moment a dragon decides, it always moves
before any enemy moves again (a lower-id enemy already moved this round; a higher-id one
moves after it), so for a pearl already on the board it wins every distance tie whatever the
ids. Turn order only decides a bed's tick: when both heads wait next to a bed as it grows,
the lower id eats. The rest of the "side" effect is deterministic divergence (absolute
direction tie-breaks on 180-degree-symmetric maps), not structure.

## What changed

- Namespace `gst5` -> `gst12`.
- `ChooseGoal`: a target an enemy head in view (this round) reaches first is worth
  `contest_factor` = 0.3 of its value. Enemy arrival = max(torus distance, bed due); ours =
  the existing `ready`. Lost when theirs < ours, or when equal at a bed's tick and the enemy
  id is lower than ours. (Torus distance ignores walls, so it errs toward conceding.)
  Workers stop wasting early turns on pearls an enemy will take first.

## Measured (native sim vs gather-split-trade-5)

| maps | as A | as B | total |
|---|---|---|---|
| official (11) | 6-0-5 | 7-0-4 | 13-0-9 |
| worldgen, 20 fresh | 11-0-9 | 11-0-9 | 22-0-18 |

`check maps/official`: all checks passed. No-action (trapped) deaths per worldgen match
323 vs parent 356.

## Expect / didn't try

Small edge within noise. Not tried: frame-relative direction tie-breaks; sitting on a bed
to block an earlier-acting enemy's tick; tuning contest_factor.
