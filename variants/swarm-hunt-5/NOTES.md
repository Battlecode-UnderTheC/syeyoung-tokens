# swarm-hunt-5

## Idea

swarm-hunt-4 with side B playing in side A's frame. Every map is 180-degree symmetric, so
a team-B dragon's observation is rotated 180 degrees (tile coordinates, window order, edges,
facings), handed to the unchanged swarm-hunt-4 agent, and its moves and sonar directions
are rotated back (`MirrorAgent` in `hunt.cpp`). Both sides now make the same choices, so
absolute direction and tile-scan tie-breaks can't favour one side.

## What changed from the parents, and why

- The task was swarm-hunt-4's side-B weakness: in the league it went 22-9 as A and 11-20 as
  B against counter-swarm-5, and similarly against counter-swarm-1 and counter-swarm-3.
- I read the code for side-dependent logic and found none. Keeper detection (round 0, no
  ECHOES) and the sonar words (own magic 0xA7 and a team bit, while counter-swarm uses 0x5C)
  work the same for both teams. The champion tie-break by id is within one team, so it
  isn't side-dependent either.
- The only side dependence left was absolute direction tie-breaks, so B now plays in A's
  frame. With this change, self-play on stronghold and trauma is an exact draw: the mirror
  is correct.

## What I measured (official = 11 maps x 2 sides; worldgen = 20 fresh maps x 2 sides)

W-L-D, as A | as B. The "as A" games are identical to the parent's, since side A is unchanged.

| matchup | official | worldgen |
|---|---|---|
| swarm-hunt-4 vs counter-swarm-5 | 9-2 \| 2-9 | 9-11 \| 9-11 |
| swarm-hunt-5 vs counter-swarm-5 | 9-2 \| 3-8 | 9-11 \| 12-8 |
| swarm-hunt-4 vs counter-swarm-1 | 9-2 \| 2-8-1 | 11-9 \| 10-10 |
| swarm-hunt-5 vs counter-swarm-1 | 9-2 \| 4-7 | 11-9 \| 9-11 |
| swarm-hunt-4 vs counter-swarm-3 | 9-2 \| 2-8-1 | 13-7 \| 10-10 |
| swarm-hunt-5 vs counter-swarm-3 | 9-2 \| 4-7 | 13-7 \| 10-10 |
| swarm-hunt-5 vs swarm-hunt-4 | 8-3 \| 1-8-2 | 11-9 \| 8-12 |

The side gap appears on the **official maps only**. On worldgen maps the parent is already
balanced by side.

## What I learned

- **On the official maps the side gap is mostly real move order, not a bug.** With exact
  mirroring, swarm-hunt-5 self-play on the official maps goes 9-1-2 for side A. The mirror
  is exact (stronghold and trauma draw), so the only asymmetry left is that team A's
  dragons act first each round. This challenges KNOWLEDGE's "no first-mover edge": for this
  strategy on these maps, side A wins most mirrored games. On worldgen, self-play is 12-8,
  about even. As B against the parent (both now in the A frame), we lose the way side B
  loses in self-play.
- The mirror flips some deterministic outcomes (arena as B against counter-swarm-5 is now
  a win), but it can't remove a move-order edge.

## What should improve

A few more side-B wins against the counter-swarms on the official maps. Elsewhere,
expect about the same as the parent (the differences are within noise).

## What I'd try next

Find where side A's first move wins on the official maps. In default_small self-play the
mirror first breaks at round 31 (B eats a pearl first, but A later wins the trades).
Candidates: pearl races at equal distance, and lunge timing (a B dragon sees A heads after
they have moved, while A sees B heads one move stale).
