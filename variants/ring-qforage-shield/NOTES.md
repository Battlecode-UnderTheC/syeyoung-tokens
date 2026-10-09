# ring-qforage-shield (g53-13, arena ring-qforage-4)

Base: ring-qforage-1 (arena #3, queen forage on 3+-dragon starts). Strategist A g53 proposal #5.

## Idea
ring-qforage-1 has no lunge field at all (only the queen's QueenRisk). A-g53 decoded ring-sealed-2's arena
losses: 104 of ~160 late (r380+) head-to-head deaths of 15+ workers had the killer in view and in its TRUE
lunge reach, ~2/3 with a free escape the bot already has. ring-sealed-shield (ring-shield-3, arena #1) cut
those 63 -> 33. This ports that behaviour to a second economy.

## Change (one behaviour)
- strategy.cpp `LungeShield` + `BuildShieldField` (end of file), called in plugin.cpp just before
  ProtectQueenAction (so it covers gst, judge, r250 lookahead and champion harvest moves).
- Non-queen dragon of 12+: if the final move's end tile is within reach ceil(Le/4)+Le-2 (in-view BFS over
  free tiles) of a head seen this turn that we won't trade with (WorthTrading exempt), a partly visible
  enemy counting as max(seen+1, 8), take the single step or free run (2..ceil(L/4), no segment paid) whose
  end is furthest from the nearest such head (outside every reach best), only if further than the chosen
  end, not Exposed, room >= L+2, EscapeLevel >= 1, path free; at most 16 room/escape evaluations.
- Queen economy untouched. Debug: -DSHIELD_DBG counters, -DSHIELD_DBG -DSHIELD_OFF = parent behaviour.
- `check maps/official`: one avoidable wall death on schooltime r364 (dragon 566, len 14): the final
  move was the base's "late: lookahead step" (search7), shield did not act; the base passes on its own
  trajectory, so this is a latent base bug surfaced by divergence.

## Local (v ring-qforage-1)
- Fresh mapgen --family all --seed 53131, 44 maps x seeds 1,2: 85-4-87; decided by elimination 56-55,
  queen 8-5, longest 19-26; lunged by enemy 10+: 7.3/match (len 120) v 9.6 (len 150).
- Per family: anti-centre 8-8, crowded 8-8, maze 6-4-6, mixed 10-6, off-axis 8-8, open 7-9,
  rich-near-home 6-10 (recheck on fresh seed 53777, 8 maps: 17-15, longest 5-3), ring 8-8, rooms 8-8,
  sealed-spawn 8-8, sparse 8-8.

## Arena duels (eph-61417daa)
- server random:2: 36-0-32 (elim 14-13, queen 16-13, longest 6-6; 3+ starters 22-18, <=2 14-14).
  Decoded all 68: late (r380+) h2h deaths of 15+ non-queens, killer in view and in reach 18 v 25,
  out of view in reach 8 v 1, total 34 v 33; longest@490 16.8 v 15.1; free runs of 12+ workers with an
  enemy head in view (r200+) 8.5 v 1.4 per game.
- official random:2: 10-0-10. CPU max 40.5M.

## Arena verdict (ring-qforage-4)
- vs ring-qforage-1: 56-0-56 (even, within noise). By suite: server 36-32, custom 14-20 (3 maps lost on
  both seats: fixed-home-clusters, fixed-no-home-food, fixed-split; the rest 1-1), official 6-4.
  Decided by: elimination 28-27, longer queen 19-16, longest dragon 9-13.
- Others: ring-shield-3 51-61, ring-sealed-2 62-50, ring-field-3 49-63. CPU p99 24.7M, max 42.6M.
- Eval P(win) v parent: r100 0.498, r200 0.503, r300 0.497, r450 0.518.
- Targeted weak spot, all 112 games decoded, both sides: late (r380+) h2h deaths of 15+ non-queens
  82 v 125 (0.73 v 1.12/game); killer in view and in true reach 29 v 80; out of view in reach 24 v 12.
  Free runs of 12+ workers with an enemy head in view (r200+) 16.2 v 2.2 per game.
  Longest at r490 15.4 v 16.0: fewer big deaths did not turn into a longer champion.
- Decoded: 279401 (gen-rich-near-home) 330 such runs, e.g. r208 L21 runs 4 from a 2-stub (5->9),
  r354 L13 runs 4 from 15 and 10-long heads (4->8); still 9 late 15+ deaths in a dense fight, lost on
  longest 30 v 67. 279056/279054 (Big Empty) 364/285 runs, won 72 v 37 and 63 v 41.
