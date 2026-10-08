# ring-pen-1 (g52-4): the queen's pen cycle (planner-design 4.4, stage S4)

- **Line:** ring-pen. **Parent:** ring-sprint-21 (variants/ring-sprint-guard2).
- **Status:** PAUSED by the user ("queens can be fixed later; board control first"). Not submitted.
  Arena duels only (ephemeral builds). Not ready: the late queen race is not better than the parent.
- **Switch:** `PEN_MODE` (strategy.hpp). 0 builds the parent: in a local self-symmetric sim
  (pen-off v ring-sprint-guard2, server maps seed 1) every per-side statistic and action count was
  identical, 17-0-17.
- **Debug:** `-DPEN_DBG` prints `PEN`/`PENX`/`PENV`/`PENF`/`PENG`/`PENE`/`PCV`/`PCVLAY` lines;
  `-DPEN_DBG_FROM=r -DPEN_DBG_TO=r` adds an ASCII map of her pen each turn in that window.

## Evidence it answers
REPORT section 4: our queen is threat-free 90-100% of r150-450 and enclosed 52-69%, yet stays 2-3 long
until r450; when both queens survive we lose the length race 28-56.

## What was built (pen.cpp, plus hooks in QueenMove, ring.cpp, conveyor.cpp, Turn's sonar)
- **Start/drop (PenQueen, PenThreat):** from r250 (kPenFrom), after 20 rounds (kPenQuiet) with no
  hunter, i.e. no enemy head with a free path to her within its reach + 2 (heads seen up to 3 rounds
  ago count, their reach grown by their free speed per round), and room >= L + 10. A hunter drops the
  cycle at once and the existing queen code (QueenMove's beam, RingQueen) takes over.
- **Cycle (PenRebuild):** a stretch of her body from the head back to segment m, closed by the
  shortest free visible path from her head to a tile beside segment m (the longest such cycle; the
  whole body when her tail is in reach).
- **Growth (PenGrow, PenEar):** square insertion (a->b becomes a->c->d->b) until C >= L + 2F + 3
  (F = ceil(L/4)), then only towards pearls in view or beds, up to max(need + 16, 3L + 12) and 120.
  Ears (a -> free path -> pearl -> free path -> b) for pearls no square reaches, nearest to her head
  first. Insertions only lengthen the cycle ahead of her head, so they never make her less safe.
- **Moves:** free steps only, along the cycle, with the safe-snake shortcut rule. A step is taken only
  if following the cycle from it never reaches a body segment before that segment leaves, even if she
  eats on each of her next F + 1 steps (exact no-collision test as a first-step fallback). One step a
  turn with no pearl on the cycle, up to F towards one. A blocked tile ahead is routed round
  (PenDetour); no safe step at all hands the turn to the old code and the cycle is rebuilt next turn.
- **Broadcast:** pen word 0xBD (box of the cycle, her head, stamp, enemy-queen sighting) on even
  rounds, relayed by workers; her champion word on odd rounds (with it every round the feeders out of
  view lost her location and r430+ feeding halved).
- **Workers:** outside her box they leave it to her (Ring5Reserved); the conveyor runs while the pen
  word is heard (before r430 only within 12 tiles, only near the unit cap: FreeSlots <= 4, and while
  the enemy queen is not known dead); its zone reaches across her coil; walkers don't step on her line.
- QUEEN_SHORT's shed-to-six is off from r250 when PEN_MODE.

## Proof it fires (local, server maps, seeds 1-2, seat A, 34 traced games, this commit's code)
- 26 starts, 9 hunter drops, 105 rebuilds (31 failed: head boxed or nothing in reach).
- r250-429: 2782 pen turns, 2986 steps, 142 eats, 165 shortcuts, 100 turns handed back.
- r430+: 706 pen turns, 1227 steps, 205 eats, 107 shortcuts, 10 handed back; 163 feeder lays.
- Queen length r300/r400/r499 (pen v parent, same game): unsw-1 21/33/dead v 3/3/dead, islands-2
  8/37 v 4/2, slithery-1 7/37/57 v 2/2/33, maze-1 8/13/52 v 4/3/dead; but maze-2 29 v 46, autarky-1
  24 v 40, islands-1 dead v 34, portals-1 dead v 21. Seat-A W-L 21-13.
- Example (slithery seed 1, r263-r493, an earlier draft): she goes 2 -> 53 on lines laid beside her
  cycle; the parent's queen stays at 2 until r430.

## Results
| build | test | result |
|---|---|---|
| draft 1 (eph-0a926c54) | arena duel, server, random:2 (duel-e5bcb144) | 30-0-38 (44.1%); longer queen 11-18; queen@end 8.2 v 7.4 |
| draft 1 | arena duel, official, random:2 (duel-a5527163) | 8-0-12 (40%); queen alive at end 15% v 30% |
| draft 1 | local, fresh mapgen (seed 52404, 44 maps x 2 seeds) | 94-2-80; queen died 64% each |
| draft 2 (eph-f3397e9f, this commit) | arena duel, official (duel-8fb0cccb) | 9-0-11 (45%); stronghold 0/4 |
| draft 2 | arena duel, server (duel-d72e775c) | still playing at hand-back |
| draft 2 | local, server maps, seeds 1-2 | 36-0-32; queen-decided 10-15 |

Per map (draft 1, server duel): trauma 0/4 and portals 0/4 (longer queen), maze 1/4, australia 1/4.

## Fixes between drafts
1. Rebuild needs only a closing path back to any body segment (with the tail out of view, 319
   rebuilds had failed).
2. Ally heads on the cycle: route round them (detour) instead of rebuilding a smaller cycle.
3. Conveyor gate before r430: race = enemy queen not known dead (the r300-sighting gate never opened).
4. Feeding before r430 only at the unit cap (it drained maze's team 60 -> 2 dragons, devil's 13 -> 1).
5. Remembered hunters (3 rounds) in the threat test (queen12 attacks from just outside her view).
6. Ears for pearls beside cycle corners; nearest to her head first.
7. Pen word on even rounds only (feeders lost her location).
8. Feeder zone across her coil; walkers don't eat her line (trauma r452-455).

## Open problems (why it is not ready)
- **The r430+ queen race:** the parent's free-roaming queen converts the conveyor's lines faster.
  On stronghold (official) the parent goes 4 -> 77 in 70 rounds, ours 14 -> 57; trauma 12 v 24.
  Half of race-phase turns have a pearl in view that no square or ear reaches: a long, dense coil
  leaves holes between its rows, and the line stays "busy" so the next feeder waits.
  A compact cycle (need = L + F + 2, cap = need + 4F + 8) was tried and was worse (17-17, unsw 12 v 30);
  reverted.
- **Small queens on long cycles:** a 2-long queen on a 22-tile cycle wanders and gets boxed
  (stronghold r270, queen trapped); the parent's 6-ring keeps workers off (ring word, yield).
- **Queen deaths in official maps r250-400** (alive@300 0.64 v 1.0 in the 20-game duel).
- Next: a re-route move that may replace arc tiles (not only add them) so holes in the coil are
  reachable; keep the 6-ring broadcast while the pen fits a 2x3; then re-measure the race phase.
