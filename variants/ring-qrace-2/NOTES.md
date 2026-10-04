# ring-qrace-2 (arena ring-sched-14), v2 of ring-qrace-1

## Idea
ring-qrace-1 (ring-sched-11) fed our queen whenever the enemy queen had been seen recently.
Its verdict was 54-47 vs ring-sched-6, with longer-queen games 24-18 and +1.3 queen length at
r490 when both queens were alive. It also spent workers when our queen was already ahead. v2
feeds only while our queen is behind theirs, or within 3 of it.

## Change vs ring-qrace-1
- **Relayed length:** the enemy queen's observed length at her latest sighting is relayed too:
  - it is a lower bound when she is only partly in view;
  - fresh sightings keep the max, because late queens grow;
  - it is sent halved in 5 bits, bit 9 plus bits 27-30;
  - the champion length field shrinks from 12 to 8 bits, capped at 255.
- **Feeding gate:** `QueenRace` steps aside when our queen > their queen + kRaceLead (3). Our
  length is taken as the max of the report and the sighting. The margin covers the halving and
  the lower-bound error.

## Escape-split guard: tried, dropped
- **What happened:** in the maze case, the queen shed 2 segments per turn from r487 (37 → 11).
  Her only free neighbour was a portal whose exit was not in current view, and the base
  deliberately splits instead of taking an unseen portal landing.
- **What I tried:** a guard that takes a late free step whose room covers the rounds left.
  It never fired, because that tile is not "current", so I removed it.
- **What a real fix needs:** a decision to risk an unseen portal late, which is a separate idea.

## Local
- **maps/server, seeds 1,2, vs ring-sched-m1:**
  - v2 went 34-34 (queen at the end 9.8 v 9.5; queen-decided 17-18);
  - v1 went 33-35 (8.6 v 9.1; 16-19).
- **How often it fires:** about 23 walk turns per match (v1: 33). Feed resigns are about
  0.1-0.3 per match.
- **mapgen seed 740219, 44 maps x 2 seeds:** every family 8-0-8, sparse 6-4-6. It is inert
  there because the queens are mostly dead by r430.

## Arena duels vs ring-sched-6 (eph-6bb82bca; the arena had errors, so some games are missing)
- **Server, random:2:** 26-0-23 at 49/68 played (17 errors).
  - Longer queen 10-6.
  - queen_length@end 7.0 v 6.6. v1 was 14-15 and 8.7 v 9.0.
- **Official, random:2:** 8-0-9 at 17/20 played.
  - Longer queen 3-2.
  - queen_length@end 14.1 v 10.6. v1 was 3-3 and 10.8 v 10.1.
- **CPU max:** 30.6M.

## Verdict (ring-sched-14; 117 games vs the parent at the time of writing, still finishing)
- **Vs the parent ring-sched-6:** 55-0-62 (47.0%, loses to the parent within noise).
  - Longer queen 25-27, elimination 22-28, longest dragon 8-7.
- **By suite:** server 38-38, custom 14-18, official 3-6.
- **Vs the other bots:** ring-sched-8 54-50, ring-rush-1 45-56, ring-plus-21 45-51.
- **Both queens alive at r490 (39 games):** 22.2 v 21.6, i.e. +0.6. v1 was +1.3.
- **Eval P(win) at r100/200/300:** 0.49/0.48/0.48. That is map-mix noise: the code is identical
  before r430.
- **CPU:** p99 21.3M, max 34.8M.
- **Conclusion:** v2 does NOT improve on v1 (longer-queen 25-27 vs 24-18; +0.6 vs +1.3 queen
  length). Stopped here, as instructed. The lead gate seems to cost more queen growth than it
  saves in workers.
