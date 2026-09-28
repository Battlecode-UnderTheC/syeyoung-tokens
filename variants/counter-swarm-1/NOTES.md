# counter-swarm-1

## Idea

Beat strategy-v1 at its own game by fixing its leaks and using a move it never makes. The
base is strategy-v1's code (explore, split at 4, trade only for an enemy at least as long,
feed leaders late). On top of it:
- **Multi-step strikes**: a MOVE of up to `min(length-1, 5)` steps along free tiles in view,
  ending on an enemy head worth trading with. strategy-v1 only looks one tile ahead, so it
  never strikes and never stays out of reach. Its leaders keep off tiles *next to* our heads,
  but we can still hit them from 2 to 5 tiles away.
- **A bug fix**: strategy-v1's `SafeStep` loops over `d = (preferred + k) % 4`. With no goal
  (`preferred = -1`) that gives -1, 0, 1, 2, so it never tries West, and a dragon whose only
  free step is West resigns "trapped: no move". This bug causes a large share of its trapped
  deaths.
- **Fewer crowds**: a pearl that another of our heads in view is closer to (torus distance) is
  worth 90% less. A step toward the goal must also leave at least one free neighbour that no
  other head is next to.
- Feeding starts at round 340 (was 400), and ally sightings are remembered for 200 rounds
  (was 80).

The code uses its own namespace `cs` (cs.cpp / cs.hpp). Knobs that tested neutral or worse
stay in Params, switched off: snipe (x2/x3), patrol, sonar gossip, `leader_split_until`.
Putting `#define CS_TUNE` at the top of cs.cpp lets environment variables override Params
for sweeps.

## What changed from the parents, and why

This is a fresh variant, but it started from a copy of strategy-v1, because exploiting it
needs its economy. Each change above was kept only if it helped over about 200 matches.

## Measured

Native sim vs strategy-v1. Map sets: official = 11 maps × 2 sides; practice = 30 worldgen
maps × 2; wg2 = 60 worldgen maps × 2; wg3 = 60 more worldgen maps × 2, kept aside for
validation.

| version | official | practice | wg2 |
|---|---|---|---|
| copy of strategy-v1 | 11-11 | 30-30 | - |
| + West fix, liberties | 8-14 | 38-21 | 74-46 |
| + strike reach 3 | 15-7 | 43-17 | 78-42 |
| + yield pearls to closer allies | 16-6 | 46-14 | 88-32 |
| + feed 340, reach 5, ally memory 200 (submitted) | 18-4 | 50-10 | 103-17 |

Validation set wg3: 96-24 vs strategy-v1, 120-0 vs anchor-greedy, 120-0 vs anchor-random.
Official maps: 21-1 vs anchor-greedy, 22-0 vs anchor-random. `check`: slowest turn 0.1 ms.
Single matches are chaotic: one knob flips individual games both ways. Only trust
aggregates of 100+ matches.

## What should improve

Win rate vs strategy-v1 and its descendants (about 80%), and fewer "trapped: no move" deaths
on worldgen maps.

## Tried, didn't work

- Splitting at 6 or 8 (instead of 4): much worse against strategy-v1. Numbers still win.
- Sonar gossip of the biggest ally (x, y, length, round packed in a 32-bit word and relayed
  by every dragon), with late workers walking towards it: 137-65, against 165-37 without.
  Walking to stale rumors wastes the late game.
- Snipe mode (workers chase big enemy heads over a long radius, in any round): it almost
  never triggers, because the enemy's big dragons are rarely in view.
- Patrolling unseen tiles late to find the enemy's last dragons: worse (154 wins vs 171).
- Leaders that stop splitting earlier (at round 150 or 230): neutral to worse.
- Trade margin ±1, hunt radius 3 or 10, room margin 1 or 6, trap cost 1.5 or 6, explore value
  0.12 or 0.5: all within noise, or worse.

## Remaining weaknesses and what to try next

- Most losses are longest-dragon losses at round 499 in which we have *more* total length:
  our mass is spread over about 15 dragons, while theirs is fed into one. Better
  consolidation is the lever.
- In some matches the enemy is down to 2 dragons for 150 rounds and we never find them.
- No defence against multi-step strikers. strategy-v1 doesn't strike, but a variant that does
  could hit our leaders from range.
