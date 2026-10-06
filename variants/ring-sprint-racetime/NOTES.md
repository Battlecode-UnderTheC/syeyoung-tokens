# ring-sprint-racetime (g45-6)

Base: ring-sprint-6 (variants/ring-sprint-conveyor), line ring-sprint.

## Idea

Strategist a g45 #3: in the queen race the conveyor starts at a fixed r430, from a queen of
length ~2, and adds about +24 by the end. In server-r19's 24 queen races lost with both queens
alive the enemy queen was 11 at r400, 23 at r450 and 37 at r490 (ours 2/3/15.5), with material
level. When the gap to the enemy we already know about is larger than what the conveyor can add
in the rounds left, waiting for r430 cannot win; start the conveyor (her ring exit plus the
feeders) as soon as that is true.

## What changed from the parents, and why

- `UpdateRaceTime` (conveyor.cpp), every dragon every turn:
  - lengths: the longest enemy queen (id 0/1) and the longest enemy of any id this dragon has
    seen (segments in view, a lower bound), or heard in a ring word;
  - from kQGateFrom (300) to kRaceFrom (430), while the enemy queen's sighting is evidence of a
    race (seen or heard at or after r300 and not seen dead; a never-seen queen, which
    EnemyQueenRace() also counts as live, is not evidence here: g44-4), the race starts early
    (`race_early_`, latched) once `gap > (500 - round) / 3`, gap = enemy queen's longest
    sighting (else the longest enemy known) minus our queen's length (the queen: her own
    length; a worker: the longest of its sighting and the reports of her; unknown: no start).
  - 1/3 a round (kConveyorGrowNum/Den) is the conveyor's own measured growth: ring-sprint-6's
    pre-promotion check, 150 sides with our queen alive at r430, +24.1 over r430-r499 (0.34 a
    round); 191697 4 -> 15 by r471 (0.27). Not fitted to maps.
- The relay: the ring word's bits 36-47 (free when it carries no cluster call, i.e. after the
  opening) carry the two lengths (6 bits each, capped at 63), so the queen in her ring learns
  what workers saw. Read only when bits 48-51 (the call's surplus) are 0.
- `RaceTime()` = r430+ or the early race: replaces `round_ >= 430` in the gates of the race
  phase only: the conveyor, her ring exit (Ring4Exit, still under EnemyQueenRace()), and the
  growth-phase queen-safety vetoes (closed-branch veto RING11, continuation/branch penalties
  RING6, last-portal escape RING6, workers keep off the tiles beside her head RING6/Ring5Reserved).
  `early_roaming` (the opening's forage/settle logic, `round_ < 430`) excludes the race growth so
  she doesn't settle-split back into a ring.
- She still grows only by the conveyor's lay-beside-her rule and her own harvest beam, with the
  existing vetoes. No sighting of a length (or no race evidence): exactly the base.
- Label: her ring exit before r430 is `racetime: early exit` (else `ring4: explicit exit`).

## Local

- check maps/official: all passed.
- sim v ring-sprint-conveyor, maps/server seed 1: 17-0-17 (mirror; identical games, the gate
  doesn't fire in self-play with equal queens). Smoke v cross-queengrow-1 (8 maps): no change.

## Arena
