# ring-qscout-1 (g42-10): re-scout a stale enemy queen before the r430 gate

Base: ring-qgate-1 (arena ring-sched-30). Arena draft: ring-sched-37.

## Idea
ring-qgate-1 blocks the queen's r430 ring exit when the enemy queen's last sighting is from
before r300. Its weak spot is a LIVE enemy queen parked far away and unseen since then: our queen
stays parked at 2-3 while hers grows late (duel-26c6e951 g56; arena-r16 cluster B). So send one
worker, not the queen, to her last-known tile to confirm or clear the sighting.

## Change (one mechanism, three tries)
- The ring word (0xBA) now also carries the tile of the newest sighting (bits 36-47, valid bit 49).
  Only the dragon that saw her knew where; workers relayed the round only.
- Nomination: from r350 (kScoutFrom) to r440, while the queen's own gate would block (sighting
  before r300, not seen dead) and she knows the tile, she names the longest worker in her view
  (bit 48, id & 63 in bits 50-55). Workers around the ring relay the name for 12 rounds; the named
  worker accepts only while it sees our queen (ids sharing the low 6 bits elsewhere ignore it).
- The scout walks (SafeStep) to the tile, with a BFS that steps across never-seen tiles as plain
  steps (a seen tile's unknown side is a portal with an unseen far end, so it is skipped). If no
  route is known, it heads for the reachable tile nearest hers. Seeing her sets enemy_queen_seen_;
  the existing champion-word and ring-word relays carry it to our queen, and the gate is rechecked
  every round, so she exits as soon as she hears it. The scout then walks back toward our queen.
  If her tile is in view and she is not there, the scout stops and the gate holds. The walk ends at r460.
- plugin.cpp: the late (r250+) lookahead step no longer replaces the scout's steps ("qscout" reason).
- Why these values: in sim the stale tiles lay 35-47 steps from the scout, and it gained about 0.5
  tiles a round, so it starts 80 rounds before the r430 decision. r460 is the last exit that still
  leaves about 40 rounds to grow (the base queen grew about 0.4 a round from r430; cs's late queen went
  3->19 from r450). There are no map keys and no board-position targets: the target is an observed
  sighting.

## Tries
| try | change | v ring-sched-30 server random:2 | v cs-v1-7 server seeds 11,12 (base 44-24) |
|---|---|---|---|
| 1 | lowest-id worker in the queen's view, start r350 | 35-33 (official 10-10) | 43-25 |
| 2 | + lookahead exemption, portal-edge fix | 33-35 (official 10-10) | 44-24 |
| 3 | + longest worker named | 33-35 | 45-23 |
Try 1 replays: every named scout zig-zagged. Cause: plugin.cpp's late lookahead step replaced its
one-step moves, so it walked in circles (autarky, local `unswbc run` with QSCOUT_DBG). Try 2: scouts
reach further but in arena v cs most die on the way, by resigning in a dead end (length 2-3, no
split out) or losing a head-to-head. Try 3 names a longer scout.

## Local
- Server maps, 2 seeds, v ring-qgate-1 (debug build): 35-33. Of 7 scouts, 2 found a live queen
  (autarky r410, islands r395) and the gate opened (r444, and by r430). The others could not reach
  her tile.
- Fresh mapgen (seed 91010, 44 maps, seeds 1,2) v ring-qgate-1: 88-88. By family: anti-centre 7-9,
  rich-near-home 9-7, all others 8-8 (crowded, maze, mixed, off-axis, open, ring, rooms,
  sealed-spawn, sparse).

## Arena verdict (ring-sched-37, 448 games, 250-0-198)
- vs parent ring-sched-30: 56-0-56 (even, noise). server 34-34, custom 17-17, official 5-5.
  Decided by: elimination 23-23, longer queen 22-21, longest 11-12. Eval P vs parent 0.50 at
  r100/r200/r300 (play is identical before r350).
- vs ring-sched-17 57-55, cs-queen-split-tail-risk-v1-7 68-44 (parent 66-46), ring-rush-1 69-43.
- Target, v cs on the same 112 seeds as the parent: 13 games had a nomination and 6 games
  differ from the parent, all of them nominated games. 2 flipped to wins and none to losses:
  166283 (tower_defense: the parent lost the queen race 3-7; ours exited late, about r460, and
  won 11-0) and 166349. Wrong blocks (enemy queen alive at r430 while ours stayed parked): 2 for
  the parent, 1 for ours, in those games.
- CPU: worst p99 27.3M, max 38.4M.

## Replays
- l66 (local autarky v cs, try 1 then try 2): in try 1 the scout looped around (13-17, 15-17)
  because the lookahead replaced its steps. In try 2 it walked straight across (dist 40 -> 14 by
  r405) and died head-to-head at r411, 10 tiles short.
- duel-4156ffc0 g62 / duel-a55d5d2a g44 (autarky v cs): the cs queen sat at (44-46,15-16) all game.
  The scout set off but stalled at the x-border kelp (the plan wraps through unseen edges) or
  resigned in a dead end on row 0. Our queen stayed parked: the wrong block remains.
- duel-4156ffc0 g35 (around_unsw v cs): the scout got from 46 to 32 steps away and was killed
  head-to-head at r398. The wrong block remains.

## Next
The scout rarely arrives, so the travel is the bottleneck, not the gate. A scout able to split
out of dead ends, and a route that avoids enemy heads, would be the next try. The optional
"feed from r400 when she is confirmed" was not tried.
