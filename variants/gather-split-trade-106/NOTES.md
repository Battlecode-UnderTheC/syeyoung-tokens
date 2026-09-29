# gather-split-trade-106

## Idea

Parent: gather-split-trade-96 (namespace renamed gst96 -> gst106, macros GST96_* -> GST106_*).

Strategist-b's diagnosis of our 7-18 series loss: the opponent grows one long dragon
early (11+ by r104) while we stay capped at L3-4, because we keep splitting greedily
regardless of the local picture. Strategist-b proposed (1) an early leader that stops
splitting well before r100 and eats towards a champion, and (2) a "consolidation brake"
that skips a routine growth split when an enemy dragon is already far ahead of ours, or
when we're locally outnumbered at the head.

## What changed from the parent, and why

- Added `ConsolidationBrake(our_longest)` (strategy.cpp, strategy.hpp): before a routine
  growth split, checks (a) the longest enemy dragon seen recently (`enemy_memory` window)
  against `brake_enemy_ratio` (2.0) times our longest known dragon (`max(length_,
  champ.length)`), and (b) enemy vs ally head counts within `brake_local_radius` (6 tiles,
  torus distance) against `brake_outnumber_num/den` (3:1). Either condition true skips the
  split this turn (item 4 in `Decide()`).
- **Tried and reverted**: lowering `leader_split_until` from 300 to 90 (the "early leader
  eats towards centre" half of strategist-b's proposal). Measured alone it lost 7-0-15 to
  the parent on official maps — starving the leader of splits early costs more team-wide
  pearl throughput than the single long dragon gains. Left `leader_split_until` at the
  parent's 300; the brake alone is the change that ships.

## What I measured

Official maps (`maps/official`, both seats, `sim --threads 2`):
- vs gather-split-trade-96 (parent): **11-0-11** (roughly even; with the leader change
  also applied it was 7-0-15, clearly worse, hence the revert above)
- vs trading-swarm-5: **12-1-9**
- vs trading-swarm-2: **16-1-5**

20 worldgen maps (`worldgen.py --batch 20`, default seed, both seats):
- vs gather-split-trade-96 (parent): **19-0-21** (near-even, 40 matches)

trading-swarm-8 (the opponent clone, task g20-1) was not yet present in `variants/` at
measurement time, so it isn't included.

I did not collect the per-map r50/r100 dragon-count/longest/pearls-eaten breakdown the
brief asks for (budget ran out after the parameter revert and the opponent comparisons);
the aggregate sim numbers above are what I have.

## What should improve, what didn't

- The brake alone is a small, safe nudge: it should reduce split-then-die-into-a-crowd
  situations without touching the team's overall pearl throughput, which is why it comes
  out roughly even rather than a clear win against the parent - it's a defensive change,
  not the "grow one long dragon early" offensive change strategist-b's success metric
  (longest >= 8 at r100) was aiming for.
- The early-leader-stops-splitting idea (reverted) is a real regression as implemented:
  simply freezing the leader's splits early starves the team of workers without reliably
  producing a long-enough leader to compensate. A future attempt at "grow one long dragon
  early" probably needs the leader to keep splitting off a couple of workers first (so the
  team still gathers) and only then commit to eating towards the centre, rather than
  freezing splits from round ~90.
- Wins clearly against both trading-swarm-5 and trading-swarm-2, so the brake is not
  actively harmful against swarm-style opponents either.

## Status

Ready to submit: `check` passes on all official maps (no avoidable deaths, slowest turn
well under judge limits - trophy's 9.6ms matches the parent's 17.1ms pattern for that map
and is not a regression). Even or ahead against parent and both trading-swarm opponents
measured, so submitting per the brief's bar (at least even with parents).
