# ring-bedlearn-2 (g40-6 v2): step around beds we would block

Parent: ring-bedlearn-1 (arena ring-sched-12). One change on top of it. Queen unchanged.

## Idea
After v1, most of the remaining "self-blocking" bed steps (the head enters a bed with
1 <= due <= length, so our body still covers it at the tick: the pearl is blocked and the gap
redrawn) happen on the way to OTHER goals ("toward goal" 1050, "no goal: roomiest" 809,
"detour" 206 of ~2500 on maps/server seed 1).

## Change
- Decide: when the first goal step lands on such a bed (and the bed is not the goal), a
  breadth-first pass from the goal (only on those turns) finds the shortest other first step,
  allowing at most +2 (one sidestep out and back). That costs fewer rounds than the pearl it
  saves, and only lasts until the bed's tick (due <= length rounds). If that step is not a plain
  safe goal step, the original step is kept. Reason "bed: step around its tick" (the plugin's
  late lookahead leaves "bed: " steps alone).
- SafeStep fallback: among steps that are equally safe (level, liberties) and both roomy enough,
  a step that doesn't block a bed is preferred.

## Local
- Self-blocking bed steps on maps/server seed 1 vs ring-sched-m1 (GM2_DEBUG): base 4604, v1 2777,
  v2 2157 (786 step-arounds). With equal-length alternatives only, it was 2758, so the +2 sidestep
  is what matters.
- Fresh mapgen (seed 4066, seeds 1-2) vs ring-bedlearn-1: 80-12-84 in total. open 7-9,
  rich-near-home 9-7, maze 2-12-2, rooms 5-11, sealed-spawn 8-8, ring 9-7, off-axis 6-10,
  sparse 7-9, crowded 11-5, anti-centre 7-9 (r100 total 32.9 vs 47.1), mixed 9-7.

## Arena
- Duels vs ring-sched-12 (eph-a81d95d1; many games lost to "sandbox failed to start"): server
  9-12 + 23-19 = 32-31, official 3-5. Pearl rate at r100 1.47 vs 1.49, at r200 2.19 vs 2.09.
- Draft ring-sched-16 vs parent ring-sched-12: 61-0-51, which the arena calls "BEATS parent", but
  within noise. By suite: server 32-36, custom 23-11, official 6-4. Games were decided by
  elimination 27-20, longer queen 21-19, longest dragon 13-11. vs ring-sched-8 62-50, ring-rush-1
  75-37, cs-queen-split-tail-risk-v1-7 64-48. Eval P(win) vs parent: 0.53 at r100, 0.51 at r200,
  0.52 at r300. Pearls eaten vs parent: 62.2 vs 60.3 by r60, 138.2 vs 131.7 by r100, 376.0 vs 364.5
  by r200. CPU p99 23.9M, max 37.8M.
