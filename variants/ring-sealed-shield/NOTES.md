# ring-sealed-shield (g53-1)

Base: ring-sealed-wallin (arena ring-sealed-2). Strategist A g53 proposal #1 (big-dragon lunge shield).

## Idea
A-g53 decoded ring-sealed-2's arena losses: 104 of ~160 late head-to-head deaths of our 15+ workers had
the killer in view and inside its TRUE lunge reach; 51 of 64 such killers were partly visible, and
BuildLungeField gave them reach from seen length + 1 (a 9-long stub seen as 3: reach 3 not 10).
Killers' true lengths at the victim's last turn: 2-8 in 139/170 (median 5-6), Manhattan 2-4.

## Change (one behaviour: the lunge shield for non-queen dragons of 12+)
- BuildLungeField: for our length >= 12 (kShieldMin), a partly visible enemy counts as length
  max(seen+1, 8) (kShieldPartialLen; QueenRisk's 8 for partial enemies) in lunge_mark_, so LungeAvoid,
  SafeStep's lunge_guard and its escape-run trigger see it. A per-tile nearest-head distance
  (shield_dist_) is kept with it.
- plugin.cpp, just before ProtectQueenAction: StrategyAgent::LungeShield. If the final move's end tile
  is in that reach, take the single step or free run (2..ceil(L/4), no segment paid) whose end is
  furthest from the nearest untradeable head (outside every reach best), only if further than the
  chosen move's end, not Exposed, room >= RoomNeeded, EscapeLevel >= 1, path free; at most 16
  room/escape evaluations. WorthTrading enemies are not in the field (trades still happen).
- Iterations: v1 = proposal as written (unknown length, clear step else LungeEscapeRun): on fresh maps a
  clear end existed in 10 of 12229 exposed turns (with unknown length the whole 7x7 view is in reach),
  so it was inert. v2 = furthest end instead of clear-only. v3 (submitted) = v2 with partial length 8.
- Debug: -DSHIELD_DBG prints SHIELD counters; -DSHIELD_DBG -DSHIELD_OFF is the parent's behaviour with
  the same counters.

## Local (fresh mapgen --family all --seed 53101, 44 maps x seeds 1,2, v ring-sealed-wallin)
| build | W-L | longest-decided | lunged by enemy, 10+ (/match) | shield moves |
|---|---|---|---|---|
| parent (OFF) | 88-88 | 25-25 | 9.2 (len 137) | - |
| v1 | 89-87 | 25-26 | 8.7 (len 127) | 10 total |
| v2 | 84-92 | 21-29 | 7.6 (len 113) | 7596 (43/match) |
| v3 | 86-90 | 24-26 | 7.7 (len 114) | 7523 |
Server seed 1 smoke (v1): 14-20 (OFF mirror 17-17), mostly seat-mirrored chaos.

## Arena duels v ring-sealed-2 (server suite random:2 = 68 games; official 20)
- v1: server 36-32 (longest-decided 10-3), official 10-10.
- v2: server 37-31 (longest 6-4), official 10-10.
- v3: server 32-36 (longest 6-9).
All within noise. CPU max 31-47M.

## Arena verdict (ring-sealed-7, v3)
- vs ring-sealed-2: 58-0-54 (51.8%, BEATS parent within noise). By suite: server 37-31, custom 17-17,
  official 4-6. Decided by: elimination 27-26, longer queen 22-17, longest dragon 9-11.
- Others: ring-field-3 63-49, ring-qforage-1 59-53, ring-sprint-21 53-59. CPU p99 23.3M, max 50.4M.
- Eval P(win) v parent: r100 0.501, r200 0.500, r300 0.501, r450 0.502. Longest at r490 27.5 v 29.6.
- Targeted weak spot (A-g53 big.py method, both sides of the same 112 games): late (r380+) head-to-head
  deaths of 15+ non-queen dragons, us v parent: 95 v 110; killer visible and in true reach 33 v 63
  (share 35% v 57%); but killer out of view and in reach 21 v 8 (runs end near unseen stubs).
- Decoded: free multi-step moves of our 12+ workers with an enemy head in view, r200+, us v parent:
  267176 (fixed-home-clusters) 161 v 10, 267182 (slithery) 13 v 0, 267167 (slithery) 11 v 5. The
  shield runs 2-4 free steps away from stubs of 2-10; deaths in view fall, but longest at r490 does not rise.
