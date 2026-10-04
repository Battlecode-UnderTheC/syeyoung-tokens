# ring-sched-m2 (g39 merge 2): STOPPED, do not submit or promote

Stack: ring-sched-m1 (arena ring-sched-6) + ring-rush-1's middle race.

## Why stopped
The user ruled that ring-rush-1 rushes the board's GEOMETRIC centre. That bakes in an
assumption about the maps we will see, and no number of games on known maps can validate it.
An observation-driven version will be built separately and merged later. This folder is kept
for its conflict resolution.

## How the merge was done
- Applied `diff -uN variants/ring-sched-1 variants/ring-rush-1` per file: rush.cpp was copied
  in, and strategy.hpp applied cleanly.
- strategy.cpp: 6 of 7 hunks applied. The failed hunk was the growth-split condition and goal
  choice in Decide (step 4/5). ring-sched-m1 had added the cap-aware growth (g38-4) and
  parallel-growth (g39-5) clauses there. Resolution, keeping both sides:
  - `!EarlyHold() && rush_state_ != 1 &&` followed by m1's two extra clauses unchanged;
  - `const Goal target = ...` replaced by rush-1's if/else chain: ally head, then RushGoal
    while rushing, then ChooseGoal.
- Rush-1's removal of `#include <cstdio>` (debug block) came along. It still builds.

## Numbers (before the stop)
- Check: maps/official, all checks passed.
- Local sim, maps/server seed 1, vs ring-sched-m1: 22-0-12.
  - Dragons at r100: 25.1 v 21.3.
  - "rush: toward middle" fired 16 times.
- Arena duels, server suite random:2, ephemeral eph-c4531ac0:
  - vs ring-sched-6: 38-0-30 (within noise).
    - 3+ starters 20-20, 2 or fewer 18-10.
    - Dragons at r100: 24.2 v 20.9.
    - Board control at r200: .53 v .43.
  - vs ring-rush-1: 46-0-22.
    - 3+ starters 29-11, 2 or fewer 17-11.
    - Dragons at r100: 21.6 v 21.9.
    - Queen alive at the end: .69 v .53.
  - CPU max 35.2M.
- Rotating suite: not run (stopped first).
