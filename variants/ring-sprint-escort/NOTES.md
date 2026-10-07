# ring-sprint-escort (g47-2) = arena ring-sprint-30

- **Line:** ring-sprint. **Parent:** ring-sprint-guard2 (arena ring-sprint-21).
- **The one change:** escorts for the late queen (escort.cpp; hooked in Decide before the conveyor,
  and conveyor.cpp doesn't count escorts as "first in line").

## Idea
g46-4 (ring-sprint-24/27): in 17 of 18 late deaths of 5+ queens the killer was 4-5 tiles off, out
of her 7x7 view, and in 17 of 18 no ally head was within 3 of her. Top teams' queens travel with
about two escorts (docs/analysis-2026-10-07/STYLE.md).

## Change
From r400, a worker computes from its own view the kEscorts=2 eligible ally heads (length 3-5)
nearest the queen within 8 steps; if it is one of them it is an escort:
- **trade:** an enemy head seen this turn that could reach her (rules' reach ceil(L/4)+L-2, floor
  3, cap 8, +1 because she moves first next round) and that we reach this turn with our free+paid
  steps through free tiles in view is hit head-on;
- **station:** otherwise walk to the tile 2-3 steps from her head whose 7x7 view covers the most of
  her blind ground (tiles within 7 path steps of her, more than 3 Chebyshev away, not in view of
  another ally head), enemy heads seen in the last 5 rounds weighted 6x; escorts don't step on
  pearls within 3 of her (her feed lines) or into her ring.
- **iteration 2 (threat gate):** the role is only taken while an enemy head was seen within 10 of
  her in the last 3 rounds. v1 took her nearest feeders: 5.6 v 7.6 feed lines beside her a game
  from r430, she ate 13.6 v 18.5 pearls, longer-queen games 9-18.
- **iteration 3 (sticky role):** an escort that eats keeps the role up to length 8 (same 2 free
  steps as 5). Seen in duel game: a 5-long escort ate one pearl 4 tiles from her, dropped the role
  and left; she died to the 11-long head it had been shadowing.

## Local
- check maps/official: passed.
- Server maps seed 1 v guard2: v1 16-18 (queen end 8.1 v 10.9); v3 19-15 (queen end 10.5 v 8.7,
  died 47% v 50%). v3 escort counters: 256 station turns over 17 games (~15 a game: the threat gate
  makes the role rare), 0 trades.
- Fresh mapgen (seed 47207, 44 maps x 2 seeds, v1): 89-87; every family 8-8 except rich-near-home
  9-7 (most fresh-map games end before r400, so play is identical).

## Duels v ring-sprint-21 (server suite, random:2, 68 games)
| version | W-L | longer queen | queen alive end | queen len end |
|---|---|---|---|---|
| v1 | 31-37 | 9-18 | 51.5 v 52.9% | 7.5 v 9.5 |
| v2 threat gate | 31-37 | 12-17 | 52.9 v 55.9% | 8.9 v 9.2 |
| v3 sticky | 35-33 | 16-15 | 58.8 v 55.9% | 9.9 v 9.3 |
- v1 official (random:2): 10-10.
- v3 replays (same seat, seat-swapped pairs): r400+ queen deaths 6 v 8; eph saved her where the
  parent's died in 3 seat-pairs (18A, 30A, 58A), lost her where the parent's lived in 2 (0B, 58B).
  Share of late turns with an ally head within 3 of her: 0.60 v 0.60 (the threat gate keeps the
  role off most of the time); her late feed lines 8.3 v 8.7, pearls eaten r430+ 20.0 v 18.6.

## Arena (ring-sprint-30)
- vs parent ring-sprint-21: 57-0-55 (50.9%, BEATS parent, within noise). By suite: server 35-33,
  custom 17-17, official 5-5. Decided by: elimination 28-27, longer queen 20-19, longest 9-9.
- vs ring-field-3 52-60, ring-qforage-1 58-54, spar-field-2 48-64.
- CPU p99 24.7M, max 40.9M.
- Eval P(win) v parent: .500 at r100/r200/r300/r400 (identical play before r400), .529 at r450,
  .540 at r490.
- Replays (111 of 112 h2h games kept): r400+ queen deaths 11 v 13 (games with her alive at r400:
  50 each); queen length at end in those games 17.2 v 14.3, pearls eaten r430+ 16.2 v 14.5, feed lines
  beside her 7.3 v 6.4; late turns with an ally head within 3 of her 0.65 v 0.61 (mean 1.31 v 1.20).
  Escort-type trades (our 3-8-long head into an enemy head that could reach our queen, r400+): 14 v
  14, mostly the same trades in both seats (pre-existing kamikaze/lunge). Escort-only ones: 226122
  r489 (7-long into a 15-long 4 from her), 226297 r430, 226456 r449, 226481 r409/r498.
- The paired "saved" queens are mostly confounded: in islands 226332/226348 the B-side queen's
  positions are identical in both games, and the parent's queen died to OUR attacker (732), not
  because escorts were missing.
- Verdict on the weak spot: the late out-of-view killer is not clearly fixed. The threat gate keeps
  the role off most of the time (local: ~15 station turns a game, 0 trades), escorts lag her
  sprints (often 4-8 tiles off), and the trade rarely finds a reachable killer. The net effect is
  a small late gain (eval .54 at r490, queen length up) within noise.
