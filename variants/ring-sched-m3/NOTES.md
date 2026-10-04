# ring-sched-m3 (g40 merge step)

Base: ring-sched-m1 (arena ring-sched-6, rush-free). Five g40 changes stacked on it, each as a
3-way merge of the variant against ring-sched-m1 (`git merge-file m3 m1 <variant>`, per file):

1. `ring-bedlearn-2`: countdown timing v2. m1 -> bedlearn-2 directly, so this includes v1
   (BEDTIME_MODE early-step/wait) and the v2 step-around. Labels: "bed: wait for its tick",
   "bed: step around its tick".
2. `ring-sched-fg`: forage/give-up gate rewrite. The forage gate checks for a length-4+ ally
   producer and `kForageSettleUnits`; the give-up drops the `units_ > kGiveUpMinUnits` term.
3. `ring-qrace-1`: queen race v1. Labels: "qrace: walk to queen", "qrace: feed queen".
4. `ring-frontier-1`: frontier exploration. Its rush.cpp is new against m1, so it comes in in its
   FRONTIER form with its hooks. No geometric-centre target is left. Labels: "rush: frontier",
   "rush: sprint".
5. `ring-lateconc-1`: late concentration (r430+, queen not known alive). It uses `late_conc_`.

The restart rebuilt this from m1. The stopped worker's partial stack was discarded: it held
countdown v1, not v2, and was missing the forage/give-up rewrite.

## Conflicts (2, both in strategy.cpp, resolved by hand)
- **Growth-split condition:** frontier made it a block (`{ if (rush_state_ == 1) rush_split_ = true; ... }`),
  and lateconc added `!late_conc_ &&`. Both are kept: `... && !late_conc_ && GrowthSplit() >= 0) { rush_split_ ...; return split; }`.
- **After the goal step:** the bed labels (bedlearn) and the frontier sprint/label block both
  went here. Both are kept, bed labels first. The sprint is now skipped while `bed_wait` or
  `bed_skip` is active, because a second free step would walk onto the bed whose tick the
  first step held back for. This is the only new logic.

## Verification
- Build OK. `check ring-sched-m3 maps/official`: all checks passed, slowest turn 1.1 ms,
  0 avoidable deaths.
- **Every change fires.** Smoke run, maps/server seed 1 vs ring-sched-m1, 34 matches, using a
  throwaway instrumented copy that was not committed. Turn counts:
  - bed wait 5697, bed step-around 843;
  - rush: frontier 722;
  - qrace walk 666, qrace feed 8;
  - late_conc_ active 10278;
  - queen_forage_ active 492 (the fg gate).

  That run went 21-0-13.
- **Fresh mapgen**, seed 518337, 44 maps, seeds 1,2, vs ring-sched-m1: 106-0-70. Queen died in
  53% of games vs 70% for m1. At r100, total length 97 vs 78.
- **Per family** (8 maps each: seed 518337 plus an earlier fresh seed 40553117, seeds 1,2):
  - anti-centre 24-8, rooms 23-9, off-axis 20-12;
  - rich-near-home 24-8, ring 23-9, maze 20-12, sealed-spawn 20-12;
  - crowded 19-13, open 19-13, mixed 16-16, sparse 16-16.

  In total 224-0-128, and no family loses.
- **Arena duels** (eph-142e8fea, server suite, random:2):
  - vs ring-sched-8: 37-31.
  - vs ring-sched-6: 31-37 (within noise; queen-length losses 13-20).
  - Games with <=2 starting dragons went 10-18 and 11-17 in the two duels, the weak spot to watch.
  - The earlier partial stack (bed v1, no fg, no lateconc) went 38-30 vs rs6 and 36-32 vs rs8.
- **CPU:** max 31.5M in the duels (under 80M).

## Arena: draft ring-sched-17 (not promoted)
- vs parent ring-sched-8: 67-0-45, 59.8% [95% CI 50.6-68.4]. The arena calls this "BEATS parent".
  - On the server's maps only: 40-28 (noise).
  - Decided by: elimination 31-20, longer queen 24-18, longest dragon 12-6.
- vs ring-rush-1 68-44, cs-queen-split-tail-risk-v1-7 66-46, ring-plus-21 62-50.
