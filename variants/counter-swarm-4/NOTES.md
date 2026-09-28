# counter-swarm-4

## Idea

counter-swarm-1 with leaders that keep out of enemy strike reach. A MOVE may take up to
length − 1 steps, so an enemy head of length L can kill ours from up to L − 1 free steps
away; counter-swarm-1 only avoided tiles *next to* enemy heads.

## What changed from the parents, and why

- Namespace `cs` → `cs4`.
- `MapThreats()`: each turn, for every enemy head seen this round that we won't trade with,
  BFS over unoccupied tiles to depth min(max(seen length, 4) − 1, `threat_cap` = 8) and mark
  those tiles threatened (seen length is a lower bound, hence the floor of 4).
- `SafeStep`: a threatened goal step is refused only if a *refuge* exists (a free,
  non-exposed, non-threatened step with enough room); otherwise the goal step is taken as
  before, so a long enemy's reach can't freeze us. The fallback key ranks
  exposed > threatened > libs > room. New reason: `dodge strike reach`.
- Leaders only (`MapThreats` returns early for workers).

## Measured (sim, both sides of each map)

| opponent | maps | counter-swarm-4 | counter-swarm-1 (parent) |
|---|---|---|---|
| counter-swarm-1 | official (22) | 11-0-11 | — |
| counter-swarm-1 | 20 worldgen (40) | 20-0-20 | — |
| swarm-hunt-1 | official (22) | 16-0-6 | 15-0-7 |
| swarm-hunt-1 | 20 worldgen (40) | 21-0-19 | 23-0-17 |

About even with the parent, within noise. Leaders dodge ~30 times per 22 official matches.

## What should improve

Fewer leaders lost to multi-step strikes/lunges late in the game. The effect is small:
strike/lunge kills are ~10–25 per match in total, and most of them hit workers.

## Tried and didn't work

- Applying the dodge to every dragon (workers too): 9-13 vs counter-swarm-1 (official),
  18-22 (worldgen), 12-28 vs swarm-hunt-1 on worldgen (parent 23-17). Workers giving up goal
  steps cost more gathering than the few strikes it saved; "toward goal" fatal turns barely
  moved (1.3 → 1.0 per match), and trapped deaths rose slightly on official maps.
