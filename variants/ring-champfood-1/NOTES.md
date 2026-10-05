# ring-champfood-1

## Idea
In late concentration (queen-dead games, from r430; ring-lateconc-1), grow the champion from
PEARLS, not resigns. A resign drops a pearl only on every second segment, so the parent's
feeders (resign within 3 of the champion's head from r430) threw half their mass away. Observed
cause: server-r15 C2, our longest is 17-24 against 26-76 in longest-tiebreak losses, while our
total was larger in 8 of 14.

## What changed from the parents, and why
Base: ring-sched-17 (code = variants/ring-sched-m3). One change, three parts, all gated on
`late_conc_` (the queen-alive case is untouched):
- **Champion harvest** (`champfood.cpp`): the late champion takes a free multi-step move
  (at most ceil(L/4) steps, capped at 6, only over tiles in view), chosen by a 6-wide beam. The
  score is pearls eaten, plus the food reachable from the end of the move (pearls, and beds by
  their exact countdowns via TileValue/BedReady) summed with weight 0.8^arrival, so a rich field
  beats a lone pearl. Enemy reach (QueenRisk), no escape and no liberties are penalised as in the
  late queen's harvest. The work cap is 50k. With nothing in reach it falls back to the usual
  goal walk. The reason string is "champion: harvest", so the plugin's lookahead and judge leave
  it alone, as they do for the queen. A partly known body is allowed: the sprint steps only on
  tiles in view, and own segments in view that memory lacks are refused.
- **Workers leave the champion its food** (ChooseGoal): a worker zeroes every food tile the
  champion reaches no later than the worker does, if it reaches it before r499. The champion's
  speed is min(ceil(L/4), 3) tiles a turn, because the sprint covers only the 7x7 view.
- **Resign only as a last resort** (Decide): no feeding (no walk-in, no resign) before
  kLastFeedFrom = 480, less the feeder's torus distance to the champion, so a far feeder arrives
  by r480. After that the parent's late feeding applies. Why r480: a feeder of length L leaves
  ceil(L/2) pearls along L tiles, and the champion sweeps at most 3 tiles a turn, so it needs about
  L/3 turns per feeder.

Iterations:
1. r488, harvest only with a fully known body. Duels: server 33-35, official 10-10.
2. Partial body allowed. Server 32-36 (longest-decided 4-11), official 9-11. In queen-dead games
   the champion gained +7 from r430 to r500, against +10.6 for the parent.
3. r480 with the early walk-in (submitted). Results below.

## Local numbers (iteration 3)
Fresh mapgen maps (seed 42317, 44 maps, all families), sim seeds 1-2, vs ring-sched-m3:
102-0-74.
- Decided by longest: 46-22. By elimination: 45-41. By queen: 9-9.
- Games our queen lost, played to r499: longest 40.0 v 32.6, total 376 v 307.
- Per family (W-L): anti-centre 8-8, crowded 8-8, maze 10-6, mixed 11-5, off-axis 7-9, open 10-6,
  rich-near-home 12-4, ring 11-5, rooms 9-7, sealed-spawn 8-8, sparse 8-8.

## Arena duels vs ring-sched-17 (iteration 3, eph-976c5c0a)
- Server suite, random:2: 36-0-32.
  - Decided by longest 6-7, by queen 19-14, by elimination 11-11.
  - Starters: 3+ went 20-20, 2 or fewer went 16-12.
- Official suite: 9-0-11.
- Queen-dead games to r500, ours v the parent's:
  - Server (14 each): longest at r480 20.2 v 20.0, at r500 22.0 v 21.3. Total at r500 135.7 v 110.6.
  - Official (4 each): longest at r500 47.0 v 44.0. Total 717 v 630.
- Eval at r100-r300: 0.5. The change acts only from r430.
- Duel CPU max: 39.7M.

## Arena verdict
ring-sched-26. Against the parent ring-sched-17: 56-0-56, even.
- By suite: server 34-34, custom 17-17, official 5-5.
- Decided by: elimination 25-24, queen 21-19, longest 10-12.
- Against the other bots: ring-rush-1 72-40, cs-queen-split-tail-risk-v1-7 69-43, ring-plus-21 66-46.
- Queen-dead games to r500 against the parent (28 v 27 games):
  - longest at r480: 20.7 v 22.4;
  - longest at r500: 23.4 v 23.2;
  - total: 215 v 165.
- CPU: p99 27.3M, max 42.7M.

**Conclusion:** the target metric did not move. Total length rose a lot, but the longest did not.
The server-r16 analysis agrees: bed pearls are already equal (4.8 v 4.5). The gap is where our dead
fall: the opponent's champion eats 13.8 pearls from its own team's dead in r400-499, ours 5.8.
Pearl routing can't close that gap.

## What should improve
The longest dragon at r480/r499 in queen-dead games, without total length dropping (the total
rises). On the server, the champion gain is small: our groups pick separate champions (sonar
fragmentation), and the food near the champion is limited.
