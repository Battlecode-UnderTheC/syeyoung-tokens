# gather-split-trade-50

## Idea

Crossover: gather-split-trade-48 (base: gst45 + gst46's open-exit veto on lifted blind
portal crossings) plus gather-split-trade-49's crowded-midgame exploration terms (stale
frontier bonus + patrol folded into the goal scan when >3 allies are in view). Both
parents descend from gst45 and touch disjoint code (gst48: the known-portal lift block in
`Decide`; gst49: the per-tile scoring loop in `ChooseGoal`), so the two changes should
compose without conflict.

## What changed from the parents, and why

Parent gather-split-trade-48 (namespace gst48 -> gst50, GST48_* -> GST50_*; already had
its sources copied into this directory).

- Diffed gather-split-trade-49 against its own parent gather-split-trade-45 (`strategy.cpp`
  only — the `.hpp` diff was just the gst45->gst49 namespace rename and default-toggle
  `GST45_OFF`/`GST49_OFF` macro renames, no behavioural change) to isolate gst49's change
  from its own namespace-rename noise.
- Confirmed gst49's isolated diff touches only `ChooseGoal` (the `crowd`/`crowded` count
  before the main scan, the stale-frontier multiplier on the explore-frontier term, and
  the patrol fold using the existing `occ(t)` lambda backed by `mine_mask`) and does not
  touch the known-portal lift block gst48 changed in `Decide` — no overlap, so I ported it
  onto gst50 (== gst48's tree) as-is:
  1. Before the per-tile scan: count allied heads seen this round via `seen_` (excluding
     self); `crowded = crowd > 3`.
  2. At the explore-frontier term: when crowded, multiply `explore_value / (d+1)` by
     `clamp(age/50, 1, 4)` where `age` is rounds since that tile was last seen.
  3. In the same loop: when crowded, fold in patrol score
     `(round_ - seen_round) / (d+4)` for open (`occ(t)`-backed, not `Occupied(t)`),
     unbanned tiles, competing via the existing `std::max`.
- Kept gst49's performance fix: used the loop's precomputed `occ(t)` (backed by
  `mine_mask`), never `Occupied(t)`, so the patrol fold stays O(1) per tile instead of
  O(length) per tile.
- Left gst48's known-portal lift veto (`exit_risk_max`, `exit_risk_ping`) untouched.

## What I measured

`check gather-split-trade-50 maps/official`: all checks passed, 0 avoidable deaths on
every map. Slowest turn big_empty 15.0ms (gst49's own check shows 14.1ms on the same map
for the same crowded/patrol code — consistent, not a new regression from this crossover;
gst48 alone, without gst49's change, is back near gst45's baseline per gst49's NOTES).
This stays over the 0.5ms/turn budget guidance on this one map, a pre-existing issue
gst49 flagged and didn't have budget to fix; not introduced here.

`sim --a gather-split-trade-50 --b <opponent> --threads 2`, both seats:

| opponent | maps | W-D-L | longest margin | total margin |
|---|---|---|---|---|
| gst48 | official (11, both seats = 22) | 10-0-12 | +2.2 | +10.5 |
| gst48 | worldgen batch 1 (20, both seats = 40) | 22-0-18 | | |
| gst48 | worldgen batch 2 (20, both seats = 40) | 25-0-15 | | |
| gst48 combined | worldgen (80 matches) | 47-0-33 | | |
| gst49 | official (11, both seats = 22) | 10-0-12 | +1.5 | +7.9 |
| gst49 | worldgen batch 1 (20, both seats = 40) | 20-0-20 | | |
| gst49 | worldgen batch 2 (20, both seats = 40) | 20-0-20 | | |
| gst49 combined | worldgen (80 matches) | 40-0-40 | | |

At least even with both parents everywhere measured: a clear edge over gst48 on worldgen
(47-33, gaining gst49's exploration behaviour), exactly even with gst49 on worldgen
(40-40, gaining gst48's portal-crossing safety without losing the exploration edge), and
positive length margins against both on official maps despite an even W-D-L there (both
parents' own NOTES report official-map W-D-L noise at this sample size too).

## What should improve

Fewer deaths right after a blind portal-crossing lift into an open exit (gst48's
mechanism) while also covering more of the map in the midgame when allies crowd around
the same beds (gst49's mechanism) — the two should now both apply rather than trading one
off for the other, since they act at disjoint decision points.

## What I tried that didn't work / next

Didn't retune here (budget: port the isolated change, measure, submit). Followed both
parents' flagged next steps rather than opening new ones:
- gst49's own next-step (not done here either): profile the crowded branch's cost on
  open maps like big_empty, or gate patrol's extra scoring to top candidates only.
- gst48's own next-step (not done here either): check the escape room past a "pinged
  clear" exit, not just its immediate neighbours.
