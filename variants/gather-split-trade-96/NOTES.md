# gather-split-trade-96

Mutation of gather-split-trade-90 (task g18-3): make dead-end (pocket / corridor) harvesting pay.

## Idea

A dead-end target was charged a flat `trap_cost` 3.0, so a pocket holding 1-2 pearls was never
worth it. Charge the expected loss instead: nothing if we can follow our tail back out, else
the stub a split leaves behind. Short dragons that can't split yet may also go in when the
pocket's pearls will make them long enough to split.

## What changed from the parents, and why

- Namespace `gst90` -> `gst96`, macros `GST90_*` -> `GST96_*`.
- `ChooseGoal` dead-end block (`trap_loss = 1`): it counts the pocket's pearls (tile value >= 0.5).
  `grown = length + pearls`. The cost is:
  - 0 if the pocket has at least `grown + 1` tiles (room to U-turn by following the tail;
    SafeStep's EscapeLevel still checks each actual step);
  - `trap_stub` = 2.0 if length >= 4 (split out; the 2-long stub is left behind);
  - 2.0, with `Goal::grow` set, if length is 2-3, `grown >= 4` and a unit slot is free;
  - otherwise the old `trap_cost` 3.0.
- `SafeStep(preferred, trap_ok, trap_grow)`: the `trap_small` refusal is lifted when `trap_grow`
  is set, so a short dragon may take a no-way-back step into a pocket that will grow it to 4.
- The `no_trap` gates (cap_safety, frozen champion, >= 10 long from r400) are unchanged.

## What was measured

The machine was at load 200-250 on 10 cores (other workers were running). Worldgen took about
5 min per map, so I measured only part of the plan.

| opponent | maps | W-D-L | end margin (longest / total) |
|---|---|---|---|
| gather-split-trade-90 | official (11), both seats | 10-0-12 | +2.5 / +2.0 |
| gather-split-trade-90 | labyrinth worldgen seeds 18300-18308 (9), both seats | 10-0-8 | +0.8 / +2.8 |

Trap counters from the sim's action tallies, ours vs the parent's:

| counter | official | labyrinth |
|---|---|---|
| "into a dead end worth it" steps | 212 vs 124 | 607 vs 234 |
| "trapped: split" (split-outs, all lengths) | 24 vs 21 | 149 vs 104 |
| "trapped: no move L2-3" (short dragon boxed in; always fatal) | 62 vs 57 | 268 vs 216 |

Nearly every official map flipped result by seat, so the official result is noise-level.

Not measured: caverns, default worldgen, and trading-swarm-5.

## What should improve

Pearls in small pockets and narrow corridors on corridor-heavy maps (labyrinth, caverns, devil,
queen_of_spades, stronghold).

## Tried / didn't work, and the risk

No sweeps. Short dragons now die boxed in more often: labyrinth "trapped: no move L2-3" went
from 216 to 268. They go in expecting to grow or to U-turn, then get stuck, probably because the
pocket's pearls were already eaten (they are remembered, not seen) or because another dragon
closed the mouth. So far that has not cost results, because a dead L2-3 stub frees a unit slot.
Next steps: require the pocket's pearls to be seen this round for the `grow` case, and have the
U-turn test use EscapeLevel at the target rather than the tile count.
