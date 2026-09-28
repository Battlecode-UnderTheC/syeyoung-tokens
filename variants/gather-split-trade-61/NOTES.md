# gather-split-trade-61

## Idea

gather-split-trade-56 plus late-surge-23's lunge guard. A MOVE reaches length-1 free
steps, but Exposed() only guards the tiles next to enemy heads, and replays (league-3) show
our 10-18-long feeders and the champion lunged by 3-4-long heads that were in view the turn
before.

## What changed from the parents, and why

- Namespace `gst56` -> `gst61`, macros `GST56_*` -> `GST61_*`.
- `MarkLungeThreat()` (once a turn, right after Remember), ported from ls23: for a dragon of
  length >= `guard_length` (10) or the frozen champion (round >= champ_freeze and it is the
  champion), BFS over free tiles from every enemy head seen this round that we would not
  trade with, up to min(its length - 1, lunge_reach) steps; those tiles are lunge-threatened.
  Heads of length <= 2 are skipped (adjacency, already in Exposed()).
- SafeStep: a step onto a lunge-threatened tile counts as exposed (the goal step is refused,
  a detour is taken). The fallback ranking grades exposure 0/1/2 (safe / lunge-reachable /
  next to a head). The room count still uses adjacency-only exposure.
- For guarded dragons (length >= 10 or a non-empty threat set) the ranking key puts
  not-exposed ahead of the doom_first term. Note: gst56 has `doom_first = 0`, so exposure
  already came first in its key; this only matters if doom_first is turned on later.
- Short dragons unchanged.

## What was measured

Both seats, `sim --threads 2`. Official = maps/official (22 matches); A = 20 worldgen maps
(default seed, pg_455123..), B = 20 worldgen maps (--seed 7777). Each 40 matches.
"lunged 10+" = our dragons of length >= 10 killed head-to-head by an enemy multi-step move,
per match.

| matchup | maps | W-D-L | longest / total margin | our lunged 10+ (length) |
|---|---|---|---|---|
| gst61 vs gst56 | official | 10-0-12 | -0.3 / +2.0 | 1.5 (22.5) vs gst56's 1.9 (24.2) |
| gst61 vs gst56 | A | 23-0-17 | +2.1 / -10.4 | 1.2 (16.9) vs 1.4 (20.5) |
| gst61 vs gst56 | B | 19-0-21 | -1.0 / +3.8 | 3.0 (44.1) vs 3.3 (48.7) |
| gst61 vs trading-swarm-5 | official | 17-0-5 | +8.1 / +13.3 | 2.6 (38.6) |
| gst56 vs trading-swarm-5 | official | 15-0-7 | +6.4 / +14.3 | 2.8 (43.0) |
| gst61 vs trading-swarm-5 | A | 26-0-14 | +0.2 / +33.1 | 1.6 (23.4) |
| gst56 vs trading-swarm-5 | A | 26-0-14 | +2.9 / +44.8 | 2.2 (34.3) |
| gst61 vs trading-swarm-5 | B | 31-0-9 | +8.7 / +97.8 | 3.2 (45.7) |
| gst56 vs trading-swarm-5 | B | 32-0-8 | +8.2 / +106.0 | 3.7 (54.5) |

Totals: vs gst56 52-0-50; vs trading-swarm-5 74-0-28 against gst56's 73-0-29. Long-dragon
lunge deaths drop on every set (by ~10-25%), short-dragon lunges unchanged. `check`: all
passed, slowest turn 0.1 ms.

## What should improve

Fewer long feeders / champions lost to shadowing short heads against lunging swarms
(trading-swarm-5 and the server's split-style opponents). The local effect on W-D-L is
small: about even with the parent.

## Tried / didn't work

Nothing else tried (short loop). Not addressed: pattern #26 (escorted champion killed by a
one-step head-on). The guard only adds lunge tiles; a one-step head-on means the champion
stepped next to the escort anyway (probably with every alternative exposed, or on a move
that does not go through SafeStep's ranking, e.g. feeding/champion moves). Worth a trace.
