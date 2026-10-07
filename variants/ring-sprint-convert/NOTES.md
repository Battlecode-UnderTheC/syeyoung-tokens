# ring-sprint-convert (g46-3), iteration 2: champion branch only

- **Line:** ring-sprint. **Parent:** ring-sprint-guard2 (arena ring-sprint-21).
- **The one change:** late conversion from r300 into ONE champion at full throughput when our queen
  can't be the target (strategist g46 C #1, champion branch; B #2). The queen-target branch waits
  for queen armour (separate worker) and can be ported back later.

## Idea
s21 server losses: 18 Schooltime losses with both queens sealed at 3, lost on longest (22 v 54)
while holding 314 v 162 total; in 51 of 73 longest losses our r400 material at 45% conversion beats
their final longest. The parent feeds no champion while our queen is (thought) alive, and starts
champion concentration only at r400/430.

## Change
- **Target (UpdateLateChampion):** from kConvertFrom = 300, a champion unless our queen was heard
  alive within kQueenAliveFresh (80) rounds. "Heard alive" is a team heartbeat: her reports, her ring
  words (stamp within 12), sightings of her that are not walled off from us, and a relay in report
  word bits 28-30 (age in tens of rounds, +10 per hop so a dead queen's echo dies; lengths capped at
  511). This replaces the parent's per-worker test, which missed parked queens (a ring queen sends
  only ring words; on autarky no worker had a report of her at r300).
- **Sealed:** sonar stops at kelp, so a sealed queen is never heard; a sighting across kelp doesn't
  count (`WalledOff`: flood over known open edges from her head, unseen edges closed, must reach our
  head within 200 tiles). Traced on Schooltime: workers saw her through her 2x2 kelp room and kept
  the team from converting.
- **Feeding (conveyor.cpp):** while a champion is the target from r300, 3 lines at once (3 walkers,
  "busy" at 3 sprint-turns of pearls); guard cap: lay only while units-1 >= max(3, their count as seen,
  half our count at first conversion). The champion takes its harvest sprint from r300; the rest of
  late concentration (no growth splits) stays at r430 (from r300 it shrank Schooltime B 56 -> 6
  dragons). Reserved ground around the champion from r300.
- **Queen:** unchanged (r430 ring exit, conveyor to her). "queen: escape split" no longer fires (0).
- **Lookahead safety (plugin.cpp):** the late lookahead step is taken only onto a known open edge.
  `check` failed on schooltime r362 (worker stepped N into kelp via "late: lookahead step", an
  existing bug exposed by the new trajectory).

## Local (sim vs ring-sprint-guard2)
- check maps/official: passed.
- Server maps: seed 1 16-0-18, seed 2 18-0-16 (34-34). Schooltime 3-1 (longest 53-34, 72-29, 51-26,
  20-26). Longest-decided 7-4. Queen died 47-50% v 44%.
- Fresh mapgen (seed 46303, 44 maps, 11 families x 8 games): 43-0-45; every family 3-5 to 5-3.
- Iteration 1 (queen also fed from r300 out of her ring): 12-22 / 13-21, queen died 62-65%.
