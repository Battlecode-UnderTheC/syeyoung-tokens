# ring-lateconc-1

Task g40-3. Base ring-sched-6 (code parent variants/ring-sched-m1).

## Idea
arena-r14 #3: in longest-dragon losses our longest/total is 0.14 v 0.27. No-burn (g39-5)
keeps the material spread over many mid-length workers to the end. Late concentration: from
r430, when our queen is not known alive (only then can the longest-dragon tiebreak decide),
the team agrees on one non-queen champion and its dragons feed it.

## What changed from the parents, and why (final = iteration 2)
All in strategy.cpp (`UpdateLateChampion`, `kLateConcFrom = 430`, `kQueenAliveFresh = 80`,
`kLateFeedDistance = 3`):
- **Queen gate.** From r430, a non-queen dragon checks whether our queen is known alive: a
  report of her (length > 0) or a sighting at most 80 rounds old, unless a later report says
  she died. This is the window in which Champion() already keeps her as the team's champion.
  While she counts as alive, nothing changes, so the queen is never starved.
- **Champion.** Otherwise the dragon picks a late champion: the longest non-queen ally by the
  faded report score, ties to the lower id. It stays champion while its report is at most
  kReserveFresh (20) rounds old. Champion() returns it, so pings, reserved ground and feeding
  all follow it.
- **Feeding (ported from gst-giant-2's `Decide` step 1, `feed_distance = 3`).** Feeders of any
  length resign within 3 path steps of the champion's head while it is in view. They are still
  held while 80% or more of the champion's length. Before r430, and while the queen counts as
  alive, the g39-5 rule stands: feeders shorter than 8, directly beside the head.
- **Walking.** If the champion's reported tile has no path in memory, the feeder walks to the
  nearest reachable tile within 2 of it. Diagnosed: 2 of 3 late feeders had no path and went
  foraging instead.
- **Splits.** No growth splits during late concentration. This is a no-op in practice: growth
  splits already end by r400.

## What gst-giant-2 does (git show variant/gst-giant-2:variants/gst-giant-2/strategy.cpp)
- The champion is the longest ally by faded sonar reports. It is not a queen.
- From r380, every other dragon not longer than the champion walks to its head or its
  last-reported tile.
- A walker dies within 3 path steps of the champion's head, when the head is in view.
- A walker holds while it is 80% or more of the champion's length, unless it is 20+ long
  from r440.
- Leaders stop splitting at r300.

We ported the delivery (head distance 3, any length) and kept our own start (r430) and queen
gate.

## Iterations (local: instrumented copy v the same code with concentration off; 44 fresh mapgen maps, `--family all --seed 2775412178`, seeds 1,2, 176 games)
Metrics are for games where our queen was dead at r430: longest / total / longest-over-total.

| iteration | r450 | r480 | r499 | W-D-L | longest-decided |
|---|---|---|---|---|---|
| off | 20.8/211/.266 | 25.7/253/.263 | 28.1/278/.277 | | |
| 1: <8 feeders, any segment within 2 | 21.8/197/.261 | 26.7/219/.270 | 28.3/225/.277 | 86-8-82 | 21-17 |
| off | 20.6/205/.276 | 23.9/234/.278 | 26.5/252/.293 | | |
| **2: any length, within 3 of head (giant-2)** | 23.0/199/.270 | 28.8/229/.282 | 32.6/247/.298 | 92-8-76 | 31-13 |
| off | 22.5/294/.225 | 25.9/334/.254 | 28.4/374/.264 | | |
| 3: iteration 2 from r380 | 24.8/149/.327 | 29.5/147/.389 | 31.1/140/.423 | 86-8-82 | 24-17 |

- Iteration 1: total fell 19% for +0.2 longest. Feeders died along the body, and the champion
  rarely came back for the pearls.
- Iteration 2: longest +5 at r480 and +6 at r499, total -2%, queen length unchanged.
- Iteration 3 (rejected): total fell 62% for +2.7 longest.

Iteration 2 v ring-sched-m1 (plain sim, same maps, seeds 1,2): 92-8-76.
- Per family: anti-centre 8-8, crowded 8-8, maze 5-6-5, mixed 12-4, off-axis 9-7, open 9-7,
  rich-near-home 9-7, ring 8-8, rooms 8-8, sealed-spawn 9-7, sparse 7-2-7.
- Seat-pair flips: 10 won on longest, 1 lost on longest, 1 lost on queen.

## Arena duels v ring-sched-6 (light: random:2 seeds)
- iteration 1 with a 30-round queen window (not kept): server 30-38. Queen alive at the end
  54% v 65%. The early window stopped relaying and reserving for a live queen.
- iteration 1 (kept gate): server 33-35 (longest-decided 8-5), official 7-13.
- iteration 2: see the final report.
