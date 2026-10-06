# ring-sprint-conveyor (g44-7; line ring-sprint, parent ring-sprint-2 = ring-sprint-founder)

## Idea
Strategist g44 #3, the feeding conveyor. arena-r18 T2: we lose late queen races on growth (their
queen +21-24 from r400, ours +6-8) while holding more material. Convert it: while our queen grows in
a queen race, send feeders one at a time and lay each in a lane beside her so she sprints through
the corpse line.

## What the base does (read in code, reproduced in sim: unsw, seed 14130595456413075645)
- QueenRace() sends a worker only if it saw the enemy queen itself within 30 rounds. A parked
  enemy queen is rarely seen, so nobody walks. In the reproduced game our queen went 3 -> 12 with
  34 dragons (210 length) alive at r450.
- Its walk step is replaced anyway: plugin.cpp's late lookahead keeps gst's step only for
  "feed"/"champion" reasons or WalkingToAlly(), and "qrace: walk to queen" is neither.
- From r450 feed14 resigns every worker with an even segment within 3 steps of her head, all at
  once. Workers further out never come.
- Not changed, but seen: in 4 of 6 large-margin B losses of ring-sprint-2 (189194, 189175,
  189143, 189121) our queen stays parked at length 2 all game. EnemyQueenRace() is false when the
  enemy queen was last HEARD of before r300 (QGATE: seen -1000, heard < 300). The gate is a
  separate idea (g44 #1 / g44-5's area).

## Change (conveyor.cpp; CONVEYOR_MODE 1 replaces QueenRace and the Ring4Supply/feed14 wave)
Active from kRaceFrom (430) while EnemyQueenRace() holds and she isn't parked in her ring (the
gate her own ring exit uses). For a worker of length >= 2:
- **Find her:** in view, or a report up to 10 rounds old. A report whose tile is in view but
  empty is stale, so the worker forages.
- **Lay:** it sees her, its head is within kLineReach (3) free steps of hers, and her line is not
  busy (fewer pearls within 3 steps of her head than ceil(L/4), one turn of her free sprint). It
  then resigns, leaving a line of pearls along its trailing body. Dragons act in turn, so the
  next feeder already counts this corpse.
- **Walk:** the worker is first in line (no ally head in view is closer to her) and within
  (500-r)/2 + 2. Its step is marked WalkingToAlly so the plugin keeps it.
- **Wait:** not first, or the line is busy. Within kQueueDist (5) of her it steps away; further
  out it forages as usual (the reserved ground keeps it off her pearls).

Iteration 1 laid only into an empty zone and only at <= 2 steps. That fed her less than the base's
wave: in the duel's 25 both-alive queen games, growth from r400 was 18.5 v 27.4. Iteration 2 (this
one) uses the sprint-sized buffer and lays at <= 3.

## Local
- Reproduction game (unsw, seed above), conveyor team A, base B:
  - base: our queen 3 -> 12;
  - iteration 1: 3 -> 36 by r480, then killed head-on;
  - iteration 2: 3 -> 52, with 13 lines laid.
- Server maps, seed 11 (iteration 1): 20-0-14; queen at end 9.8 v 6.9; queen-decided 9-4.
- Fresh mapgen (all families):
  - iteration 1: 66 maps, 64-2-66;
  - iteration 2: 22 maps x 2 seeds, 22-0-22.
- check: all passed.

## Light duels vs ring-sprint-2
- Iteration 1 (eph-4d480802):
  - server 33-35 (queen-decided 11-18, queen at end 7.2 v 8.9);
  - official 9-11 (queen 2-4).
- Iteration 2 (eph-2ce8b90a):
  - server 36-32; queen-decided 18-14; queen at end 9.4 v 7.6; ≤2 starters 16-12, 3+ 20-20;
    CPU max 32.1M.

## Arena
(filled in after the verdict)
