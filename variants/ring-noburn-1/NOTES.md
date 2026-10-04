# ring-noburn-1

Arena id: ring-sched-5 (parent ring-sched-2, code parent ring-sched-port). Task g39-5.

## Idea

server-r13 #3: at the cap we cycle material through split/resign stubs, and at r360 (kPantryFrom)
the feed phase resigns most of the team beside the champion, halving its mass. Replace it with
gradual consolidation and parallel growth.

## What changed from the parents, and why

All in strategy.cpp (constants kFeedMaxLen 8, kNoBurnFrom 250, kParallelGrowLen 16, kCapRelease 12):
- Feed: a feeder resigns only when the champion's head is one step from its head (was: within 2 of
  any champion segment, or within 3 steps of its head). Only dragons shorter than 8 feed, so long
  dragons never merge.
- Give-up slot: a 2-3 stub resigns only with a long ally's head (6+) directly next to its head; the
  crowd trigger is gone.
- Parallel growth: from r250, once units reach UNIT_LIMIT-4 (latched; released below UNIT_LIMIT-12),
  non-leader workers make no growth split until length 16. Capgrow kept.
- Queen logic unchanged.

## Results

- Local check: maps/official passes. Smoke (seed 7): slithery W (total 293 v 70), default W, schooltime L (longest 15 v 62).
- Light duels vs ring-sched-2: server 33-0-34, official 9-0-11; vs cs-queen-split-tail-risk-v1-7 server 44-0-24.
- Arena verdict: vs ring-sched-2 42-0-36 (53.8%, within noise; server 36-32, official 6-4);
  decided by queen 23-14, elimination 14-12, longest 5-10. vs ring-plus-21 44-34, ring-sched-1 45-33,
  cs-queen 49-29. Record 180-132. Worst map schooltime 4-12.
- Metrics vs parent (78 games): total length r400 125.8 v 99.5 (r300 108 v 111); longest r350 9.7 v 10.0,
  r400 13.2 v 14.9. Eval P(win) r100/200/300 0.50/0.50/0.49.
- CPU: worst p99 20.7M, max 37.3M.

Conclusion: the mass burn is gone (total at r400 +26%), but longest does not rise: the parallel growth
latch does not produce 12+ dragons by r350, and the longest tiebreak got worse (5-10).
