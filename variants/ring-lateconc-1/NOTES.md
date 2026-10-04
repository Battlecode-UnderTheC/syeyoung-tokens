# ring-lateconc-1

Task g40-3. Base ring-sched-6 (code parent variants/ring-sched-m1).

## Idea
arena-r14 #3: in longest-dragon losses our longest/total is 0.14 v 0.27. No-burn (g39-5)
keeps the material spread over many mid-length workers to the end. Late concentration: from
r430, when our queen is not known alive, the team agrees on one non-queen champion and short
feeders resign beside any part of it.

## What changed from the parents, and why
All in strategy.cpp (`UpdateLateChampion`, `kLateConcFrom = 430`, `kQueenAliveFresh = 80`):
- From r430, a non-queen dragon checks whether our queen is known alive: a report of her
  (length > 0) or a sighting at most 80 rounds old, unless a later report says she died.
  This is the same window in which Champion() already keeps her as the team's champion.
  While she counts as alive, nothing changes. A 30-round window lost queen games in a duel
  (queen alive at the end 54% v 65%).
- Otherwise it picks a late champion: the longest non-queen ally by the faded report score,
  ties to the lower id. It keeps that champion while its report is at most kReserveFresh
  (20) rounds old. Champion() returns it, so pings, reserved ground and feeding all follow it.
- Feeders (2..7 long) resign within kBodyReach (2) of ANY visible segment of it, also when
  its head is out of view. The g39-5 head-only gate still applies before r430.
- If the champion's reported tile has no path in memory, the feeder walks to the nearest
  reachable tile within 2 of it. Diagnosed: 2 of 3 late feeders had no path and went
  foraging instead.
- No growth splits during late concentration. This is a no-op in practice: growth splits
  already end by r400.

## Local (sim, vs ring-sched-m1)
- Fresh mapgen maps (`--family all --seed 2775412178 --count 44`, seeds 1,2): 86-8-82.
  - Per family: anti-centre 8-8, crowded 8-8, maze 4-6-6, mixed 8-8, off-axis 9-7, open 10-6,
    rich-near-home 8-8, ring 8-8, rooms 9-7, sealed-spawn 7-9, sparse 7-2-7.
  - 12 seat-pair flips: 7 to us, 5 against. 9 of the 12 were decided by longest/total with
    both queens dead.
- The change rarely fires: about 1 extra "feed champion" a game. Most late feeders never
  reach the champion before r500.

## Arena
(see below)
