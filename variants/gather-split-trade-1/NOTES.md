# gather-split-trade-1

Mutation of strategy-v1 (task g1-1). Focus: its "trapped: no move" deaths.

## Idea

strategy-v1 plus two changes. It looks ahead before it steps: a timed flood fill, then an
exact search, asks whether the dragon can still get back out. And workers give way on
targets that an ally in view is nearer to, so the swarm spreads out instead of piling up on
the same pearls and boxing itself in.

## What changed from the parents, and why

- **Escape check (`EscapeLevel`)**: a timed BFS from the tile after the step. Segment i of
  our own body counts as free from move `L - i + 1` on, because the tail moves up. A step
  scores 2 if the flood reaches a freed segment of ours (we can follow our tail back out)
  or the unexplored edge, 1 if it reaches `max(2L+12, 24)` tiles, and 0 otherwise.
  RoomAround only counted tiles, so a 1-wide dead end longer than `L+3` passed it. A goal
  step now needs level ≥ 1.
- **Exact turn-out search (`TurnOut`)**: a level-0 step gets a DFS over our own moves with
  the body simulated, budget 3000 nodes. It succeeds when the head gets back onto a tile
  the body left. This rescues pockets that a short dragon can circle in and leave, such as
  2×2 shrine rooms, which the BFS can't see because it never waits.
- **Liberties**: a step is also ranked by whether the tile after it still has a free
  neighbour that no other head (ally or enemy) can take first. A step through a portal to
  a tile out of view counts as crowded (`avoid_blind`), since someone may be lying there.
- **Split safety (`SplitSafe`)**: a growth split happens only if both halves pass the
  escape check, each treating the other half as a wall.
- **Short dragons don't enter "dead ends worth it"** (`trap_small`): below length 4 they
  can't split out.
- **Target sharing (`share_factor = 0.1`)**: a pearl or bed that an ally in view is nearer
  to (torus Manhattan distance against our BFS distance) is worth a tenth. This was the
  biggest single gain in W-D-L.
- **Patrol**: with no target, walk towards the tile seen longest ago (about neutral).
- Tried and left off (the parameters are still there): pre-emptive "doomed: split" when
  every step is level 0 (`doomed_split_room`; it split long dragons into fragments late in
  the game); putting doom avoidance ahead of enemy-head avoidance (`doom_first`, worse on
  the official maps).

Cost: the worst turn seen (with GST1_DEBUG work counters) was about 7000 turn-out nodes plus a few hundred flood pops, a few million wasm points at most. `check` wall times were noisy (load average 50+), up to ~1 ms on worldgen maps, and strategy-v1 showed similar spikes.

`GST1_TUNE` / `GST1_DEBUG` blocks are for local builds only (parameter overrides from an
environment variable, and state dumps). The judge's build never defines them.

## Measured (sim, vs strategy-v1, both sides of every map)

| maps | W-D-L | longest / total margin |
|---|---|---|
| official (11) | 16-0-6 | +3.4 / +94 |
| worldgen seed 101 (40) | 72-0-8 | +11.0 / +192 |
| worldgen seed 9001 (40) | 62-0-18 | +7.4 / +150 |
| worldgen seed 55555 (40) | 64-0-16 | +10.8 / +178 |
| worldgen seed 777000 (40) | 54-0-26 | +5.8 / +154 |
| worldgen seed 31337 (40) | 58-0-22 | +6.3 / +175 |

Against anchor-greedy and anchor-random: 80-0-0 on worldgen and 21-0-1 on official. The
loss is trauma as A, which strategy-v1 also loses. `league quick` against strategy-v1 and
anchor-greedy gave 28-0-14 and 41-0-1 (measured on an earlier build).

**Deaths:** the raw count of trapped deaths did **not** go down in these matches. We make
about 35% more dragons (more pearls, so more splits), and most of the trapped deaths come
from a crowded swarm near the unit limit (64). Almost all are dragons of length 2–3 whose
last free tile an ally's body took. Per split, trapped deaths fell from 0.55 to 0.46 on the
official maps and from about 0.71 to 0.68 on worldgen. Early on, before target sharing,
the look-ahead alone left W-D-L about even with strategy-v1 (13-9 official, 48-30 and
52-28 on worldgen) but improved the total length margin.

The numbers are noisy: one-parameter changes swing results by ±8 matches in 80. Judge by
the sum over the sets.

## What should improve

More pearls collected (the swarm spreads out), fewer dragons walking into corridors and
1-tile notches they can't leave, and fewer blind portal deaths. On worldgen maps it wins
about 3:1 against strategy-v1.

## What didn't work / open problems

- Trapped deaths are mostly caused by the crowd, not the map: a short dragon whose last free
  tile an ally's body took. Neither the look-ahead nor the liberties fix that.
- The official losses (default, stronghold, trauma) are long games with more total length
  but a shorter longest dragon. Late consolidation (feeding) is weak. Tuning
  `feed_from_round`, `ally_memory` and `leader_split_until` did nothing measurable.
- Split length 5 or 6 is much worse (confirms split at 4). Keeping headroom below the unit
  limit, lower share factors (0.03) and exempting leaders from sharing were all neutral or
  worse.
