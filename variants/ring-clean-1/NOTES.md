# ring-clean-1

## Idea

ring-sched-m1 (arena ring-sched-6) with its map-shape assumptions removed and its most
impactful tuned gate rewritten on its observed cause. The full gate audit is in AUDIT.md.

## What changed from the parents, and why

- **Replaced the centre pull** (strategy.cpp, from body-wall-17). Before this change, on boards
  of at most 1024 tiles, goal and explore scores were boosted near `(W/2, H/2)` before r50.
  Removing it outright lost devil 1-19 locally. There, the pull steered dragons past the first
  kelp-walled bed strip. The replacement uses only what the dragon has seen: before r50, an
  unexplored-edge tile's explore score gains up to x2 by the share of beds among the known
  tiles within radius 3. Devil 4-16 locally (still a loss). Fresh generated maps 69-1-62 vs the
  parent (plain removal: 64-1-67).
- **Removed search7's centre bias** before r80. It had no effect on play: the search agent only
  steers from r250.
- **Forage gate on its cause.** Before: the queen foraged when `units_<=2 && length_<=3`.
  Now it forages when it cannot split (length < 4, from the rules), no ally in view at r0 can
  split, and the team has fewer than 6 dragons (the existing settle threshold). Same on every
  1- and 2-starter map we know. It also fires on portals and islands, where no starter can split.
  On dilemma it fires only when the length-11 ally is out of view.
- **Give-up slot**: dropped `units_ > 32`. "At the cap" is `FreeSlots() <= 1`, from UNIT_LIMIT.
- Deleted the stale `.orig` files.

The queen's safety rules are unchanged (the forage corridor/room penalties, threat reach,
clearance, the ring).

## What should improve

Nothing on known maps by design, except possibly portals and islands (the forage change).
The point is that no behaviour depends on the board's size or its geometric centre.

## Measured (arena duels vs ring-sched-6, official judge, 2 random seeds, both seats)

| build | server suite (17 maps) | official-only (5 maps) |
|---|---|---|
| v1: centre pull removed outright, plus forage and give-up (eph-8c643d40) | 28-0-40 (41.2%); 3+ starters 16-24, 2 or fewer 12-16; dilemma 0-4, devil/trophy/weakhold/islands 1-3 | 9-0-11; default_small 0-4 |
| v2 = this commit: bed-field pull in place of the centre pull, plus forage and give-up (eph-2f9f1c3a) | 28-0-40 (41.2%); 3+ starters 17-23, 2 or fewer 11-17; slithery 0-4, dilemma/qos/stripes/tower_defense 1-3 | 10-0-10 |
| control: forage and give-up only, centre pull kept (eph-aed795e2; source at /tmp/g39-audit-v3) | **37-0-31 (54.4%)**; 3+ starters 23-17, 2 or fewer 14-14; portals/islands/dilemma 3-1 | (not run) |

Local sim vs ring-sched-m1 on 22 freshly generated maps (mapgen seed 90517, 3 seeds): v1
64-1-67, v2 69-1-62.

Verdict: the forage rewrite is a gain (+4 points, on exactly the maps where it now fires). The
centre-pull removal is a clear loss on the known maps (about -13 points against the control, both
seat-classes). The pull to (W/2, H/2) really helps on the known small maps, devil most of all:
locally 1-19 without it. The bed-field replacement does not recover that on the arena. So
ring-clean-1 is **not submitted**: one class is a clear loss.
