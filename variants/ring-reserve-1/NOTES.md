# ring-reserve-1 (g37-7, arena ring-plus-11, parent ring-plus-7)

## Idea
Reserve growers (strategist B, server-r11). In all 19 of ring-plus-4's longest-decided server
games our longest dragon at r300 was 3-7 against an opponent's 8-40. At the unit cap almost
every worker is 2-3 long. While the queen lives, Champion() returns her, so no worker ever stops
splitting: each worker splits at length 4 until r320, or r400 for teams of 48 or more.

## What changed from the parents, and why
- `strategy.cpp` growth split (rule 4): from r250 a non-queen dragon of length >= `kReserveLen`
  makes no growth split. Nothing else changed. There is no coordination between dragons.
- `kReserveLen` = 5, not the brief's 6. With an instrumented sim trace (seed 22; schooltime,
  default, dilemma), at r260 the longest non-queen dragon was 3-4 on both teams, so 0 dragons
  were >= 6 or even >= 5. The brief says: use 5 if fewer than 2 dragons are >= 6 at r260.
  With 5, schooltime at r300 had 2 dragons >= 5 (longest 7, base 6) and at r400 longest 13 v 15.
- The give-up-slot resign (rule 4b) already only fires at length <= 3 (`kGiveUpMaxLen`), so
  reserve dragons (>= 5) were already exempt. No code change was needed there.

## Local (seeds 21-26, both seats, 2 threads; ring-explore-2 = ring-plus-7's code)
Pearl share before r60 and dragons at r100 are identical to the base: the change starts at r250.
The r300/r400 figures come from a temporary sim build with extra snapshot rounds (uncommitted).

| match | W-D-L | 2-starter | 3+-starter | longest r300 | longest r400 | total r400 |
|---|---|---|---|---|---|---|
| vs base, server maps | 94-4-106 | 41-0-43 | 53-4-63 | 6.4 v 4.6 | 14.4 v 11.7 | 87.8 v 82.8 |
| vs base, official maps | 86-4-90 | 40-0-44 | 46-4-46 | 7.4 v 5.2 | 16.4 v 12.4 | 119.7 v 99.2 |
| rr-1 vs qr17-grow-5, server | 104-0-100 | 54-0-30 | 50-0-70 | 6.9 v 4.8 | 15.4 v 12.8 | 103 v 80 |
| base vs qr17-grow-5, server | 114-0-90 | 55-0-29 | 59-0-61 | 5.0 v 4.9 | 11.6 v 13.2 | 91 v 78 |

- vs base, decided by: server: elimination 35-36, queen 50-61, longest 9-9;
  official: elimination 36-38, queen 34-40, longest 16-12.
- The longest dragon rose as intended: +2 at r300 and +3-4 at r400. Total length did not fall.
  But win rate didn't move, and against the teammate's bot the reserve build did slightly
  worse than the base (104-100 v 114-90), with fewer elimination wins.

## Arena verdict (ring-plus-11, 312 games, 163-0-149)
- vs parent ring-plus-7: 41-0-37 (52.6%, CI 41.6-63.3), "beats parent" but within noise.
  Decided by: longer queen 19-17, elimination 15-14, longest dragon 7-6.
- vs queen-ring17-first-then-grow-5 37-41, ring-plus-9 42-36, queen-unified-1 43-35.
- By suite: server (fixed seeds) 142-130; official 31-0-29 (40 games, 21-19 vs the four bots);
  the rotating suite had no games. Official, by end: elimination 12-12, longest 3-5, queen 6-2.
- Over all games: longest-dragon games 29-22 (56.9%), elimination 65-52, queen 69-75.
