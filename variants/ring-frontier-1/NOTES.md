# ring-frontier-1 (g40-5)

## Idea
The user's idea: "rush toward unseen; the map is a torus anyway". ring-sched-8's centre rush aimed
at the board's geometric centre, which won on known maps but lost 64-96 on fresh anti-centre maps
(rushers walked 20-30 rounds without eating or splitting, into centre traps, stalled on walls).
Replace the target with the frontier, which is observed, not assumed.

## What changed from ring-sched-m2 (arena ring-sched-8), and why
All in rush.cpp, plus the split gate in strategy.cpp. Movement (RushSprint) is unchanged.
- Explorers: the two lowest-id non-queen starters, as before. The 5..26-from-centre gate is gone.
- Target, each turn: food within 2 steps first, so the opening isn't sacrificed. Otherwise the reachable
  frontier tile (known free, beside an edge never seen) with the most never-seen tiles in the
  7x7 view it would give, divided by (path distance + 1), i.e. new tiles seen per round walked.
  The current target gets x1.25 so near-ties don't make the explorer dither.
- Spread: a frontier tile within 6 of an ally head that is nearer to it is left to that ally.
- One growth split is allowed while exploring, at length 4+ (GrowthSplit checks the child's room).
  The base banned splits while rushing.
- Stop exploring when:
  - a bed is seen within 4 of an enemy head (contested): the explorer holds it to r60, the base's hold rule;
  - a cluster (2+ live or soon-due beds within 2) is seen that wasn't in the r0 view and has no ally head within 4: the scheduler farms it;
  - 6 rounds pass with no new tile seen;
  - or at r30.
  - Why the r0-view exemption: in the first build, 50 of 59 explorers stopped at about r8 on clusters at their own spawn, which the queen and the other starters already farm.
- Parity/contest zone: the base used "head within 7 of the board centre". It is now "an explorer
  holding a contested find, its head within 4 of it" (4 = HeadsNear's radius).
  - Ablation: applying the parity rule to every worker within 4 of any dense cluster went 30-50 vs m1 on anti-centre maps.
  - The narrow zone went 37-43, and no contest rule at all went 40-40.
- No board-size gates, no fixed points, no map keys. The 7x7 view is the game's window.

## Local numbers (sim, fresh mapgen maps, seeds 1-2, both seats)
- 44 maps, `--family all --seed 40517`:
  - vs ring-sched-m1 (rush off): 91-3-82;
  - vs ring-sched-m2 (centre rush): 102-0-74.

  Per family, vs m1 / vs m2:

  | family | vs m1 | vs m2 |
  |---|---|---|
  | anti-centre | 8-8 | 9-7 |
  | crowded | 9-7 | 9-7 |
  | maze | 9-7 | 5-11 |
  | mixed | 8-8 | 10-6 |
  | off-axis | 8-8 | 10-6 |
  | open | 10-6 | 12-4 |
  | rich-near-home | 9-7 | 11-5 |
  | ring | 9-7 | 11-5 |
  | rooms | 7-9 | 10-6 |
  | sealed-spawn | 9-2-5 | 10-6 |
  | sparse | 5-1-10 | 5-11 |

- 20 maps, `--family anti-centre --seed 40523`:
  - vs m1: 37-43 (centre rush m2 was 64-96 vs m1 on g39's set);
  - vs m2: 45-35.
- maps/server seed 1 vs m1: 17-17 (the centre rush was 22-12 there).
- Board seen by explorer ids at r30 / r60 (each dragon's own memory):

  | maps | ring-frontier-1 | ring-sched-m1 | ring-sched-m2 |
  |---|---|---|---|
  | all-family | .28 / .39 | .27 / .38 | .24 / .32 |
  | anti-centre | .19 / .26 | .19 / .29 | n/a |
  | server | .25 / .33 | .21 / .29 | n/a |

  - In the all-family runs, the m2 column comes from the run against m2; frontier's own figures in that run were .27 / .36.
- Weak spot: explorers die more often on anti-centre maps. Explorer ids alive at r30: 101 v 116 for m1. Total length at r40: 31 v 34.
- Check: `build/tools/check ring-frontier-1 maps/official` passes.

## Arena
(see below)
