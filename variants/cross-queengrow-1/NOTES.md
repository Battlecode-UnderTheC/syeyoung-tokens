# cross-queengrow-1 (task g35-6, line crossover, parent crossover-1 = local gst-cross-1)

## Idea
ring17's edge is late queen growth. Its queen stays short until about r450 and then grows to 10-24.
That wins the queen-decided games when both queens survive (REPORT.md section 5: 54-19, against
qg7's 19-34). crossover-1 feeds a non-queen "heir" champion instead, so its queen ends 3-4 long.

## What changed from the parent, and why (one change: grow the queen instead of an heir)
- strategy.cpp/hpp: `OurQueenId()` and `QueenAlive()` (a report of our queen at most kQueenFresh = 40
  rounds old, with length > 0). While she is reported alive, nobody walks to feed a non-queen
  champion. Feeding the queen herself, when she is the champion, is still allowed.
- From r250, allies relay the queen's report on even rounds, so the team keeps hearing that she
  lives. The champion and obituaries keep the odd rounds.
- From r430 the reserved ground (kReserve = 4, Manhattan) is around the queen's head instead of the
  champion's: the others leave those pearls to her (ring17's Ring5Reserved idea).
- From r450, `QueenSupply()` ports ring17's Ring4Supply (SUPPLY14_MODE 1). A non-queen dragon resigns
  where it is when it sees our queen's head this turn and has a pearl segment (every second one,
  head first) within 3 free steps of it.
- queen_guard.hpp: from r430 the queen makes no economy splits; trapped splits are still allowed.
  Otherwise she keeps gst's eating under the guard's review (risk first, then room), so she only
  eats where the guard finds the step safe and roomy.
- When the queen dies and her report goes stale, the base's heir feeding comes back.

## Local (sim, seeds 1-4, both seats, vs gst-cross-1)
| maps | W-D-L | decided by (W-L) | queen at end (us v them) | queen died |
|---|---|---|---|---|
| server | 42-0-38 | elim 19-18, queen 12-5, longest 11-15 | 2.9 v 0.7 | 78% v 81% |
| official | 72-0-48 | elim 27-21, queen 31-14, longest 13-12, total 1-1 | 5.8 v 1.3 | 58% v 68% |
| tournament-sprint | 31-0-25 | elim 11-5, queen 17-16, longest 3-4 | 6.4 v 1.4 | 55% v 61% |

- vs queen-ring17 (server, seeds 1-2): 18-0-22, queen-decided 2-8. The parent went 19-0-21 and 4-9.
- Our queen still dies far more often than ring17's (80% v 58%). ring17 wins the queen-decided
  games because its queen survives, not because she is longer.
- No extra late eliminations; longest-decided games barely move.

## Arena
