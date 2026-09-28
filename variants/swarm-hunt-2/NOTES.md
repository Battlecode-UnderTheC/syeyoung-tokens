# swarm-hunt-2

## Idea

gather-split-trade-1 (body-simulated escape look-ahead, target sharing, split children as
hunting workers) plus swarm-hunt-1's **lunge**: a MOVE of up to length-1 steps (capped at 6)
along free, currently-visible tiles onto an enemy head worth trading for (enemy at least as
long, the base's WorthTrading rule, never with our last dragon). Nothing else moves during
our turn, so a lunge seen is a lunge landed.

## What changed from the parents, and why

- Base: gather-split-trade-1, namespace renamed `swh2`; the GST1_TUNE getenv block removed.
- New `Lunge()` (bounded BFS: reach min(len-1, 6), at most 256 pops), tried right after the
  one-step kamikaze. Both leaders and workers lunge (leaders only at longer enemies, as before).
- `MarkThreats()`/`keeper_guard`: leaders treat tiles an untradeable enemy head could reach
  in `keeper_guard` steps as exposed (bounded BFS per head). Shipped **off** (`keeper_guard = 1`):
  with 2 it lost on worldgen (see below).

## Measured (native sim, 2026-09-28, load ~65)

keeper_guard = 2: vs gather-split-trade-1 official 14-0-8, worldgen set A (20 maps) 16-0-24;
vs swarm-hunt-1 official 15-0-7, worldgen A 18-0-22.

keeper_guard = 1 (shipped): vs gather-split-trade-1 official 14-0-8, worldgen A 19-0-21,
worldgen B (20 fresh maps) 22-0-18; vs swarm-hunt-1 official 15-0-7, worldgen A 21-0-19.
check: all checks passed (slowest turns 3-17 ms under load ~65; the parent showed similar).

## What should improve

Official maps (small/crowded: lunge trades land where the parent only trades adjacently).
Worldgen about even: lunges are rare there (~1 per match); "trapped: no move" (~300/match)
still dominates deaths for both sides.

## Tried, didn't work

Leaders keeping 2 steps from enemy heads (swarm-hunt-1's keeper guard): fewer paths for
leaders in mazes; 16-24 vs 19-21 on worldgen A.
