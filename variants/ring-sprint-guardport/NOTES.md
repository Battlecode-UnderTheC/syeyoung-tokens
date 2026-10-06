# ring-sprint-guardport (g45-1) = arena ring-sprint-10

- **Line:** ring-sprint.
- **Parent:** ring-sprint-6 (variants/ring-sprint-conveyor: ring-sprint-1 + founder + feeding conveyor).
- **The one change:** a port of the lunge guard + escape run as built in ring-sprint-3 (g44-3,
  variants/ring-sprint-guard).

## Idea
Long non-queen workers die to enemy heads lunging from several tiles away. ring-sprint-3 cut those
deaths 21% on server maps (5.37 v 6.84 a game) by keeping 6+ workers out of an untradeable enemy's
lunge reach, and running clear with free steps when no single step is clear. The head
(ring-sprint-6) has neither.

## Change
- The exact diff from variants/ring-sprint-1 to variants/ring-sprint-guard, applied by a 3-way merge
  (`git merge-file`) onto ring-sprint-6's sources. It merged with no conflicts.
- What it brings:
  - `BuildLungeField`, the SafeStep guard and `LungeEscapeRun` (strategy.cpp/.hpp);
  - FreeSprint's extra steps refuse lunge tiles (rush.cpp);
  - the plugin keeps gst's step or run against the lookahead and the rollout judge (plugin.cpp).
- founder.cpp and conveyor.cpp are untouched:
  - The founder only picks goals.
  - The conveyor's walk and wait steps call SafeStep. As before, they fall back to foraging when
    SafeStep does not give "toward goal". A guard veto or an escape run therefore means "forage
    this turn", the conveyor's own rule for an unsafe step.
  - Laying a feed line is a resign, which the guard does not touch.

## Local
- `check ring-sprint-guardport maps/official`: passed.
- Server maps, 1 seed, smoke run: 15-0-19. Escape runs: about 5 a match.
- Fresh mapgen (all families), 22 maps (seed 4501), 1 seed:
  - 18-2-24;
  - 10+ dragons lunged by the enemy: 15.2 v 20.5 a match.
- Light duels vs ring-sprint-6 (eph-88a615d5):
  - server: 33-35 (queen-decided 12-13; ≤2 starters 15-13, 3+ 18-22); queen at end 9.0 v 7.9;
    CPU max 30.7M;
  - official: 9-11.

## Arena: ring-sprint-10
- **Verdict v ring-sprint-6:** 54-0-58 (48.2%, CI 39.2-57.4), "loses to parent", within noise.
  - By suite: server 34-34, official 5-5, custom (rotating) 15-19.
  - Decided by: elimination 28-29, longer queen 19-16, longest dragon 7-13.
  - By starters: ≤2 18-18, 3+ 36-40 (server 3+ 20-20; custom is all 3+, 15-19).
  - By map: no class lost clearly. Every map is within 1-3 / 3-1, mostly seat-split 2-2 or 1-1:
    - sealed queen (fixed-pen) 1-1;
    - corridors (maze 2-2, stripes 2-2, fixed-maze-wide 1-1);
    - cramped (fixed-tiny 1-1).
- **Other opponents** (ring-sprint-6's own record against them in brackets):
  - ring-contest-4 62-50 (99-1-72);
  - ring-sched-17 71-41 (109-63);
  - ring-rush-1 78-34 (109-63).
- **CPU:** p99 25.8M, max 51.8M.
- **Eval P(win) v parent:** r100 .494, r200 .496, r300 .499, r400 .508.

## The targeted weak spot, in its own 112 games v the parent
Tools: g44-3's h6.py, threat.py and agg.py (arena-r18 decode.py), with ME = ring-sprint-10.

Deaths are non-queen 6+ deaths to enemy heads, per game, new v parent. "Stubs" are killers
shorter than 6.

| suite | deaths | by stubs | after our multi-step move | eats per 6+ turn | Voronoi r100-300 |
|---|---|---|---|---|---|
| server (68) | 5.62 v 6.60 (-15%) | 4.18 v 5.46 | 0.75 v 0.37 | .136 v .139 | .476 |
| official (10) | 15.4 v 16.4 | 8.9 v 9.1 | 3.3 v 1.5 | .141 v .149 | .495 |
| custom (34) | 17.3 v 19.7 | 10.1 v 14.1 | 2.53 v 0.91 | .132 v .147 | .469 |
| all (112) | 10.04 v 11.44 (-12%) | 6.40 v 8.40 | 1.52 v 0.63 | .135 v .143 | .476 |

- **Threatened states:** a 6+ worker with a shorter enemy head in view, and every free single step
  in its lunge reach or next to it.
  - All suites: it ran clear in 544 of 1905 (the parent: 15 of 2311).
  - Server only: 133 of 567 (the parent: 5 of 657).
  - Death to an enemy head within 2 rounds: 16.0% after a clear run, 27.8% without one (the
    parent: 25.2%).
- These are the numbers ring-sprint-3 had (527/1881; -21% on server, -14% overall). The port works
  as it did on ring-sprint-1.
- **The conveyor is intact.** Games with both queens alive at r400: 47.
  - Our queen grows +15.4 from r400, the parent's +12.1.
  - Record 23-24. Queen deaths after r400: 13 v 17.
  - L5+ workers resigning within 4 tiles of their queen from r430: 180 v 161.
  - So feeders did not run away.

## Replays
- 198292 (autarky) r411: a 19-long worker with a 7-long enemy 2-3 tiles away runs WNWN out of
  reach and lives to the end. At r416 a 14-long worker runs ENN from the same enemy.
- 198305 (devil) r407: a 16-long worker between 4-, 6- and 4-long stubs runs SSWW and lives 47 more
  rounds.
- 198498 (australia):
  - r404: a 14-long worker among four stubs runs WNWN and lives 64 rounds.
  - r450: a 21-long worker runs EEE away from a 10-long enemy and is lunged in the same round. This
    is the side effect seen in ring-sprint-3: deaths right after our multi-step moves.

## Side effects (not fixed here; the same as ring-sprint-3's)
- Deaths right after our own multi-step move: 1.52 v 0.63 a game (server 0.75 v 0.37).
- Eats per 6+ turn: -6% overall, -2% on server.
- Custom suite: 15-19, eats .132 v .147.
