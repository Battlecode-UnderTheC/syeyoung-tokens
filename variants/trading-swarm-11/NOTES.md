# trading-swarm-11

## Idea

This is trading-swarm-10, the clone of the real server opponent, with three fixes to how its
dragons die, all measured against the 45 server games on per-game maps (`clone-eval/`).

## Changes from trading-swarm-10

1. **Tail check before a split** (`kSplitGuard`, head check off). SPLIT only when the tail,
   which becomes the child's head, has a free neighbour. A newborn with no move dies on its
   first turn (~33 a game). Checking the head as well (ts10's trial) cut splits; the tail
   alone doesn't.
2. **Extra room for 2-3-long dragons** (`kSmallExtra = 2`). The first step toward a pearl or
   bed is swapped when the window pocket behind it holds fewer than L+2 tiles; longer dragons
   still need L. A 2-3-long dragon can't split its way out of a pocket, so it was the main
   victim of the pearl -> pearl -> stuck chain.
3. **Loop breaker** (`kLoopBreak`). After 24 rounds on at most 6 tiles with nothing eaten,
   ignore pearls, beds and portals for 20 rounds and drift. A lone 3-long dragon circled a
   2x2 block for 400 rounds on Portals, alternating "nearest pearl" and "to unknown portal".

## Tried and dropped

- **Dispersal** (`kDisperse`). Code kept, set to 0 (centre drift). Every form hurt: a random
  target per dragon, a random target only before r60, and no drift at all (wander). With an
  own random target the clone ate 134/188/178 per century instead of 167/251/243, and
  agreement fell to 18/45. The centre drift is where the pearls are. The real opponent's
  early spread along the top edge of Schooltime is not a random walk.
- **Picking the goal by step safety** (`kGoalGuard`, `RoomP`: a pocket with a portal edge
  counts as open). This skips pearls whose first step is unsafe, instead of swapping the
  step. Agreement: 23/45 at extra 2, 27/45 at extra 1. But the clone then won only 25 of 90
  against gst96 (seeds 1-3), much weaker than ts10.
- **Extra room of 3 or 4:** it cut eating and splits below the real opponent's.

## Fidelity (45 server games, per-game maps, exact seed and seats)

| | ts10 | ts11 | real |
|---|---|---|---|
| result agreement | 21/45 | 26/45 | |
| opponent W-L | 15-30 | 20-25 | 31-14 |
| r100 count | 18.5 | 16.5 | 20.5 |
| r200 count | 22.7 | 21.9 | 30.7 |
| r300 count | 22.6 | 23.1 | 38.5 |
| final longest | 15.0 | 12.8 | 21.6 |
| splits per century, r0-399 | 73/103/102/77 | 48/78/82/62 | 51/76/76/54 |
| deaths per century, r0-99 | 58 | 35 | 34 |

The split and death rates now match the real ones closely, and so does the eating. Our
bot's eating against the clone matches its server numbers.

The mean count is still about 22-25 at r200-300, against the real 31-39. It is concentrated
on a few games:

- Portals M547330 and M580809: the clone is down to 1-3 dragons by r25-50. This comes from
  early "portal: escape" self-deaths: stepping through a portal onto its own body.
- Schooltime M588437: the real opponent had 47 dragons at r100, the clone 12-15.

## Measurements

- gst96 vs clone, 15 official maps, seeds 1-3 (90 games): vs ts11 53-3-34, vs ts10
  52-3-35. The server series was 14-31.
- ts11 vs ts10: 28-6-26 on the official maps (seeds 1-2), 20-0-20 on 20 worldgen maps (seed
  26700).
- `check`: all checks passed. The slowest turn read 96 ms at load average 180 (ts10 reads
  the same under that load). Re-time on an idle machine.
- No environment variables are read.

## Next

- Portal escape: only step through when the landing tile is in view and free, or else
  choose the least bad death.
- Why the real opponent's count keeps growing on Schooltime and Default.
