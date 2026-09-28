# body-wall-19

## Idea

body-wall-15 plus one SafeStep gate: a dragon of length >= 4 that can't count on splitting
out of a pocket (in practice: at the unit cap) also measures its room with the tiles another
head (ally or enemy) could step onto next round left out. If the room is enough only through
such a tile, the goal step is refused, and the fallback ranks such steps below others (after
escape level, before liberties). The aim was pockets whose one way in gets plugged within a
round.

## What changed from the parents, and why

- Namespace `bw15` became `bw19`, and every `BW15_*` macro became `BW19_*`.
- `Params::strict_exit` (1) and `strict_exit_min_length` (4): the gate above (in `SafeStep`).
- Debug only (`BW19_DEBUG`, compiled out; switch it on in strategy.hpp): `BW19TRAP` tallies.
  For every `trapped: no move` they record length, cap state, and for each of the 3 turns
  before it the reason, the room after that step (a flood capped at 60 that leaves out
  occupied and exposed tiles), and the heads within Manhattan distance 4. The trapped-death
  `Dump` moved behind `BW19_DUMP`.

## What I measured

Study, body-wall-15 logic (4 official maps: stronghold, schooltime, trauma, default, plus 20
worldgen maps, both seats; 48 matches each):
- vs trading-swarm-5: 185 `trapped: no move` a match. By length: 2 → 53%, 3 → 24%, 4–5 → 12%,
  6+ → 11% (1,010, most of the length lost). Half were at the unit cap. vs gather-split-trade-51 the
  shares were the same (225 a match).
- **Every trapped length ≥ 4 death was at the cap with no split possible** (2,047 of 2,047).
- The step 1 turn before the death was almost never "toward goal" (42 of 8,860). It was "no goal:
  roomiest" (47%), "detour" (22%), or the stub a `trapped: split` left behind (23%). "Toward goal"
  dominates 2–3 turns before (t-3: 3,663 of 8,860).
- **Rooms flip, they don't shrink:** 3 turns before, most doomed dragons had 60+ room (4,056). At t-1,
  rooms were either 60+ (1,813) or 1–2 (3,725), and almost never 8–40. Most (4,670 of 8,860) had 3+
  other heads within 4 tiles. For length ≥ 4 the pattern is the same: 60+ room and 3+ heads at t-3,
  then below L+3 by t-2. So the pocket isn't a dead end the goal walked into. A crowd closes open
  ground within a round. This matches the KNOWLEDGE entry "The cap trap is set before the cap is
  reached".

Fix, study set vs trading-swarm-5 (baseline 36-0-12, +4.6 longest, 185 trapped a match):
- Gate for every dragon that can't split out (short dragons too): 30-0-18, -0.7 longest, trapped
  deaths down 9% (167 a match). The length 6+ deaths didn't move (996), and the short dragons lost
  gathering. Rejected.
- Gate for length ≥ 4 only (shipped): 35-0-13, +3.3 longest. Trapped deaths unchanged (1,033 at
  length 6+).

Shipped version vs body-wall-15, both seats:
- official: 12-0-10, +1.0 longest, -0.4 total. Trapped deaths 46.1 a match vs 45.2.
- worldgen set 1 (20 maps): 22-0-18, +0.9 longest, +14.2 total. Trapped 281.0 vs 274.5.
- worldgen set 2 (20 maps, fresh): 23-0-17, +4.6 longest, +16.9 total. Trapped 281.9 vs 273.2.
- Overall 57-0-45. `check`: all checks passed, slowest turn 0.3 ms (stronghold).

## What should improve

Little, honestly. The gate doesn't reduce trapped deaths. The 57-45 is at the edge of noise, and
may come from long dragons at the cap staying out of crowded bottlenecks in other ways.

## What didn't work / lessons

- Most trapped deaths aren't "walked into a dead end chasing a goal". The goal step 2–3 turns
  earlier was into wide-open ground (60+ room) inside a crowd (3+ heads nearby), and the room
  collapsed within one round. A room/exit check at step time can't see that coming. The gated
  variant for short dragons cut trapped deaths 9% and still lost games (less gathering).
- For long dragons, being at the cap is necessary and sufficient: every length ≥ 4 trapped death
  had no split available. The lever is on the supply side (free slots for escape splits, fewer
  idle stubs), not caution.
