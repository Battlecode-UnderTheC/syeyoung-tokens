# gather-split-trade-5

Crossover (task g3-4): gather-split-trade-4 (base) + swarm-hunt-2's lunge.

## Idea

gather-split-trade-4's sonar-agreed champion and late-game feeding, plus the lunge: when no
enemy head is adjacent, look for one worth trading within a multi-step MOVE and land on it
(nothing else moves during our turn, so a lunge seen is a lunge landed).

## What changed from the parents, and why

- Namespace `gst4` -> `gst5` (and the debug/tune macros).
- `Lunge()` copied from swarm-hunt-2 unchanged: BFS over free, currently visible tiles, reach
  min(length-1, `lunge_reach` = 6), at most 256 pops; target = enemy head passing the base's
  `WorthTrading` (leaders only at longer enemies, never with our last dragon).
- Tried right after the one-step kamikaze, only when `units_ > 1` and the dragon is not the
  champion (round >= feed_from_round and Champion() is us): the champion only grows.
- Feeding is untouched (it runs before the kamikaze, as in the parent). swarm-hunt-2's
  keeper guard was shipped off there, so it is not ported.

## Measured (native sim vs gather-split-trade-4, both sides of every map, load ~20-30)

| maps | W-D-L | longest / total margin |
|---|---|---|
| official (11) | 12-0-10 | +6.6 / +27.6 |
| worldgen, 20 fresh maps | 22-0-18 | +4.3 / +3.3 |

About 1.3 lunges per official match, 2.6 per worldgen match; kamikazes rose slightly too.
`check maps/official`: all checks passed (slowest turn 28 ms on schooltime under load; the
lunge search is capped at 256 pops, so this is machine noise, as with the parents).

## What should improve

A few more favourable trades per match without touching gathering or feeding; small edge,
within the noise (±8 in 80 per KNOWLEDGE).
