# ring-sprint-guard2 (g45-8)

- **Line:** ring-sprint.
- **Parent:** ring-sprint-16 (variants/ring-sprint-qroom: ring-sprint-1 + founder + feeding conveyor
  + the queen keeps room).
- **The one change:** a port of the finished lunge guard + escape run of ring-sprint-17 (g45-1
  iteration 2, variants/ring-sprint-guardport on task/g45-1, commit 11d1f02).

## Idea
Long non-queen workers die to enemy heads lunging from several tiles away. ring-sprint-17 (on
ring-sprint-6) cut non-queen 6+ deaths to enemy heads 24% (5.25 v 6.87 a game on server maps; all
suites 9.51 v 12.38) without raising deaths right after our own multi-step moves (0.52 v 0.71), and
left the conveyor's late queen growth unchanged. The line head ring-sprint-16 has neither.

## Change
- The exact diff from variants/ring-sprint-conveyor (main) to variants/ring-sprint-guardport
  (11d1f02), applied by a 3-way merge (`git merge-file`) onto ring-sprint-qroom's sources. It merged
  with no conflicts; the result is the same +162/+22/+58/+5 lines in strategy.cpp/strategy.hpp/
  plugin.cpp/rush.cpp.
- No semantic overlap with the queen-room change: the guard returns early for the queen
  (`IsQueen()` in BuildLungeField/LungeEscapeRun/LungeAvoid), and the room change lives only in
  QueenMove/QueenSpace.
- What it brings (unchanged from ring-sprint-17):
  - `BuildLungeField` (seen reach ceil(Le/4)+Le-2 for the single-step guard; unbounded end reach for an
    enemy whose body leaves the view), the SafeStep guard and `LungeEscapeRun` with the bounded search;
  - FreeSprint's extra steps refuse lunge-end tiles (rush.cpp);
  - the plugin keeps gst's step or run against the lookahead and the rollout judge (plugin.cpp).

## Local
- `check ring-sprint-guard2 maps/official`: passed.
- Server maps, 1 seed (7), smoke v ring-sprint-qroom: 17-0-17 (elimination 5-5, queen 8-7,
  longest 4-5); queen at end 10.6 v 8.7; queen deaths 47% each.
