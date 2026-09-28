# body-wall-6

## Idea

body-wall-5 with `hunt_radius` set to 0: workers no longer chase enemy heads
to force a trade; they still take strikes/trades that come to them (all other
trade/combat logic unchanged). Same single-parameter change that beat
gather-split-trade-31 54-48 and late-surge-6 60-42 in g10-2 (KNOWLEDGE.md:
"hunting hurts" — the busy mid game punishes going out of your way to hunt).

## What changed from the parents, and why

- Namespace renamed `bw5` -> `bw6` (macros too) so both can load in one sim.
- `strategy.hpp`: `hunt_radius = 6` -> `hunt_radius = 0`. Nothing else touched.

## Measured

vs body-wall-5 (parent), `--threads 2`:
- Official maps (22 matches, both sides): **11-0-11**, body-wall-6 leads by
  +1.2 longest / +4.8 total length at game end on average.
  - Per-map (A/B swap pairs): Colosseum 1-1, arena 1-1, big_empty 0-2 (loses
    both sides, contested map for this archetype already), default 2-0,
    default_small 1-1, devil 2-0, queen_of_spades 1-1, schooltime 1-1,
    stronghold 1-1, trauma 1-1, trophy 0-2.
- Worldgen set 1 (20 maps, 40 matches): 20-0-20, longest -1.1, total +21.7.
- Worldgen set 2 (20 maps, 40 matches): 24-0-16, longest +4.5, total +47.6.
- Combined worldgen: **44-0-36** in body-wall-6's favor, positive length
  margin in both sets. Death breakdown near-identical to the parent (slightly
  fewer head-to-head deaths, since we no longer walk into optional trades).

vs gather-split-trade-32 (gst31 + hunt_radius 0), official maps (22 matches):
- **9-0-13**, body-wall-6 trails by -1.1 longest / -15.7 total. Expected:
  gst-32 is a different archetype (gather-split-trade lineage) that is
  already ahead of body-wall on several of these maps regardless of hunting;
  this isn't a same-parent comparison, it's cross-lineage.

`check body-wall-6 maps/official`: all checks passed, slowest turn 19.8 ms
(well under the 0.5 ms/9M-point guidance mentioned in the brief, but also
well under any hard wasm turn budget concerns — this matches body-wall-5's
existing cost, unaffected by this change).

## What should improve

Fewer costly head-to-head/no-action deaths from workers wandering off to
chase heads during the crowded mid game, at no measurable cost on the
official maps (even record, slightly ahead on length) and a clear net
positive across 40 fresh worldgen maps (44-0-36, positive length margin in
both independent batches of 20).

## What I tried that didn't work

Nothing else attempted — brief scope is this one parameter change only.
