# ring-capgrow-1

Arena id: `ring-plus-19` (line ring-plus, parent ring-plus-16 = code of variants/ring-bud-cpu). Task g38-4, strategist B r14 #5.

## Idea
At the unit cap we fill the last slots with 2-3-long stubs: workers keep making growth splits until the
cap, then stubs resign ("give up slot") to free room, and the cycle repeats. Our longest stays short
while the opponent builds a giant (server-r12: 60+ dragon games, longest 12.2 v 24.0 at r400;
longest-decided 8-17; Schooltime 0-8).

## What changed from the parents, and why
One rule, in strategy.cpp "4. growth": when `UNIT_COUNT >= UNIT_LIMIT - 4` (observed `unit_count`,
`spawn.unit_limit`), a worker of length 4 or more makes no growth split, so it keeps growing.
The give-up-slot resign (4b) already applies only to length <= 3 (`kGiveUpMaxLen = 3`); a
static_assert pins that. Trapped, doomed, escape, queen and feed splits are unchanged.

## What should improve
A longer longest dragon at r300/r400 in cap games; a better longest-decided record; Schooltime.

## Local numbers (instrumented scratch copy of tools/sim, seeds 101-108, both seats)
"capsplits" = all splits made while the team had 60+ dragons; "giveups" = give-up-slot resigns (per game).
A cap game is one where our side reached 60+ dragons (a biased filter: it favours games we were winning).

vs ring-bud-cpu (base):

| maps | group | W-D-L | pearl share <r60 | dragons r100 | longest r300 | longest r400 | splits at cap | give-ups |
|---|---|---|---|---|---|---|---|---|
| server | all | 140-2-130 | 0.50 | 21.4 v 21.5 | 6.4 v 4.7 | 12.7 v 11.7 | 49 v 127 | 10 v 71 |
| server | 2-starter | 55-0-57 | 0.50 | 13.5 v 13.5 | 5.0 v 4.8 | 12.5 v 12.1 | | |
| server | 3+-starter | 85-2-73 | 0.50 | 26.9 v 27.2 | 7.0 v 4.6 | 12.8 v 11.5 | | |
| server | cap games (92) | 60-0-32 | | | 9.0 v 4.9 | 13.6 v 11.9 | | |
| official | all | 125-2-113 | 0.50 | 18.6 v 18.9 | 7.4 v 5.2 | 15.1 v 13.1 | 34 v 97 | 8 v 53 |
| official | 2-starter | 56-0-56 (identical games: cap never reached) | | | | | | |
| official | 3+-starter | 69-2-57 | | 25.5 v 25.9 | 8.5 v 5.1 | 15.6 v 12.6 | | |
| official | cap games (56) | 41-0-15 | | | 13.7 v 6.0 | 20.1 v 14.7 | | |

Longest-decided: server 18-14, official 21-18. Schooltime 10-6 on each map set.

vs the teammate's cs-queen-split-tail-risk-v1-7 (seeds 101-104), ours vs the base on the same games:
- server: 51-85 (base 52-84). Longest at r300 6.5 v 5.0 (base 4.8 v 4.9); at r400 12.1 v 12.9 (base 12.0 v 12.9).
- official: 48-72 (base 49-71). Longest at r400 13.3 v 14.4 (base 14.1 v 15.7).
- Schooltime: 2-6 on each map set (base 5-3 on each; n = 8).
The mechanism works: splits at the cap fall from about 130 to 50 per game, and give-ups from 70 to 10.
The lead in longest at r300 fades by r400 against cs7, and the win rate doesn't move.

## Arena verdict (ring-plus-19)
- vs parent ring-plus-16: 42-0-36 (53.8%, CI 42.9-64.5): beats_parent, within noise.
  Decided by longer queen 20-17, elimination 15-15, longest dragon 7-4.
- by suite vs parent: server maps 37-31, official 5-5. All opponents: server 148-0-124, official 20-0-20, no rotating games.
- vs cs7 39-39; vs queen-ring17-first-then-grow-5 44-34; vs queen-unified-1 43-35.
- Overall 168-144, longest-dragon games 24-22. Schooltime 10-6, portals 5-11 (worst).
- CPU: worst p99 22.3M, max 34.7M (100M cap).

## Next
The longest at r300 grows, but against cs7 the gain fades by r400. Try pairing this rule with
consolidation, so that one of the now-longer workers becomes a real champion after r300.
