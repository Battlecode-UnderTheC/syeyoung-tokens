# counter-swarm-2

## Idea

counter-swarm-1 (strikes, pearl sharing, early feeding) with gather-split-trade-1's look-ahead
safety layer: a step or a split is judged by whether the dragon can still get back out.

## What changed from the parents, and why

- Namespace `cs2`. Everything of counter-swarm-1 kept (strikes, kamikaze, sharing, feeding).
- `SafeStep`: each candidate step now also gets `EscapeLevel` (timed flood from the new head in
  which our own segments free up as the tail moves; other dragons' bodies, allies' included,
  stand still) and, when the flood says doomed, `TurnOut` (exact search over our own moves,
  3000 nodes per step, recursion depth capped at 200). The goal step is taken only if it can
  get out (or it is a dead end worth entering and we are long enough, >= 4, to split out).
  Detour ranking: not exposed > escape level > liberties > tiles reached > room.
- A step through a portal onto a tile out of view counts as crowded (liberties capped at 1).
- Growth splits only when both halves can get out (`SplitSafe`).
- Kept counter-swarm-1's half-liberty `Liberties` with `min_libs = 2`.

## Measured (native sim, deterministic)

| opponent | maps | W-D-L |
|---|---|---|
| counter-swarm-1 | official (22) | 12-0-10 |
| counter-swarm-1 | 20 worldgen (40) | 26-0-14 |
| gather-split-trade-1 | official (22) | 13-0-9 |
| gather-split-trade-1 | 20 worldgen (40) | 26-0-14 |

Trapped deaths ("trapped: no move") vs counter-swarm-1: 43 vs 49 per match (official), 235 vs
248 (worldgen). Small drop, as the knowledge file predicted: most are small workers boxed in by
allies' later moves, which a look-ahead with other dragons standing still can't see.
`check`: slowest turn 0.1-0.4 ms (one 1.1 ms spike under load on `default`, 0.1 ms on rerun).

## Tried, didn't work

- `contest_first = 1`: the flood's first ring skips tiles next to another dragon's head (the
  ally may step there first). 8-14 official, 20-20 worldgen vs counter-swarm-1. Off.

## What should improve / next

Fewer deaths in corridors and splits into pockets, mostly on worldgen mazes. Trapped deaths
remain the top cause; next ideas: coordinate who enters a 1-wide corridor (sonar), or let
length-2/3 workers feed/resign earlier instead of wandering into crowds.
