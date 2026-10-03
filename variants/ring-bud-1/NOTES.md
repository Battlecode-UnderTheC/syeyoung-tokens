# ring-bud-1

Task g37-3. Arena id `ring-plus-9`, line `ring-plus`, parent `ring-plus-7` (code parent ring-explore-2).

## Idea

Port the teammate's "legend-bud" split from queen-ring17-first-then-grow-5. Our longest dragon
lags real teams (4.8 v 8.9 at r300 on the server), and ring-plus-4 went 6-13 on server games
decided by longest dragon. Budding a 2-long child off a long dragon, instead of halving it, keeps
the mass in the long dragon.

## What changed from the parents, and why

One line at the top of `GrowthSplit()` (strategy.cpp): before r360 a dragon of length >= 6 splits
off a 2-long child (`if (round_ < 360 && length_ >= 6 && SplitSafe(2)) return 2;`), otherwise as
before (halving). The split gates in the caller are unchanged.

## What should improve

A longer longest dragon at r300/r400 and more wins on longest-decided games, without fewer dragons
at r100.

## Local (sim, seeds 21-26, both seats, vs ring-explore-2 = ring-plus-7's code)

| maps | W-D-L | decided by (W-L) | dragons r100 (us v them) | total r40 |
|---|---|---|---|---|
| official, all | 85-5-90 | elim 37-41, queen 26-34, longest 22-14 | 21.8 v 21.4 | 25.6 v 25.5 |
| server, 2 starters | 42-0-42 | elim 19-19, queen 20-21, longest 3-2 | 13.4 v 13.4 | 13.9 v 13.9 |
| server, 3+ starters | 56-5-59 | elim 15-18, queen 25-31, longest 16-9 | 27.9 v 27.5 | 34.1 v 34.3 |

- The sim gives no pearl share; the opening total length (r10-r40) is the proxy and is identical,
  as expected: the bud only fires once a dragon reaches 6.
- Longest-decided games improve (22-14 and 16-9), but queen-decided games go slightly worse; overall a tie.
- Not run: the local comparison vs queen-ring17-first-then-grow-5 (the machine was overloaded). The arena covers that matchup.

## Arena verdict (ring-plus-9, complete)

- vs ring-plus-7: 43-0-35 (55.1%, CI 44-66), BEATS parent within noise.
  Decided by: longer queen 23-14, elimination 12-14, longest dragon 8-7.
- By suite vs ring-plus-7: server (fixed seeds) 37-31; official 6-4; rotating none played.
- vs queen-ring17-first-then-grow-5 42-36; queen-unified-1 45-33; queen-ring14-portal-safe-1 45-33.
  By suite vs grow-5: server 38-30, official 4-6.
- Economy (dragons alive): r50 12.6 v 11.8, r100 23.7 v 20.7. Longest dragon wins overall 29-19 (60%).
- Worst maps: server/portals 5-11, slithery_fight 4-7, Colosseum 3-5, default 6-10.
- Caveat: 68 of the 78 parent games are on the server suite (2 fixed seeds per map), and there were
  no rotating games, so the verdict mostly rests on the suite g37 says overrates bots.
