# ring-sprint-screen (g53-14)

- **Line:** ring-sprint. **Parent:** ring-sprint-intercept (arena ring-sprint-34).
- **The one change:** Strategist B g53 proposal #5: late stubs screen our grown queen so the
  interceptor (TryIntercept, g52-15) has a chance.

## Idea
g52-15: the trade works when it has a chance (263249 r224/r286, 263280 r370), but allies near a
dying queen were 3-7 path tiles from the killer and 2-3 long. arena-r27 S1a: 226 late losses where
our big queen is lunged by a stub with no ally within 3; allies within 6 of her fall 4.75 -> 1.56
over her last 40 rounds; 45/58 lungers come from just outside her view.

## Change (screen.cpp, plugin.cpp, strategy.hpp)
- `TryScreen`: from r400 (kScreenFrom), while our queen is 10+ (seen length or a fresh sonar
  report), a non-queen worker of length 3-6 that sees her head ranks itself among the fully seen
  3-6 allies whose heads are within Chebyshev 3 of hers (they see her) by distance, ties by id.
  The two nearest are screens.
- Threat side: the nearest enemy head seen in the last 5 rounds; with none, the centroid of the
  ground within 8 of her that was not seen in the last 10 rounds.
- Station: a reachable free tile in view at Chebyshev 2-3 from her head nearest the threat point
  (the second screen: the best one at least 2 from the first's). The path never enters a tile
  within 1 of her head (her next steps stay free; the conveyor's "a body beside her is a wall").
  On the station it circles to the best neighbouring station. The step goes through SafeStep; if
  SafeStep picks something else the normal action is kept.
- plugin.cpp: tried after TryQueenAttack/TryIntercept (same gate), not when rescue-committed or
  trapped. The screen step is kept against the late lookahead (like a bed wait); the rollout judge
  and ProtectQueenAction still review it. Reason: "screen: hold by her".
- `-DSCREEN_DBG`: gate counters and per-turn SCREEN lines.

## Local (v ring-sprint-intercept)
- `check ring-sprint-screen maps/official`: passed.
- Server maps seed 7: 18-0-16; queen died 41% v 41%, end length 12.9 v 11.8. Screen turns 61 in 34
  games. Gate (debug): 22383 late stub turns, 827 see the queen, only 64 with her 10+: in our sims
  the ring queen is mostly 2-4 long when stubs see her after r400 (she grows from r430).
- Fresh mapgen (seed 53141, 44 maps, seeds 1,2): 84-8-84, identical per-family to the parent
  (queens end 1.2 long on average; the screen never fired).

## Arena
- Ephemeral duels v ring-sprint-34: server (random:2) 34-0-34 (queen alive 57.4% v 58.8%, queen
  end 9.8 v 10.1); official 11-0-9 (queen alive 35% v 35%, end 13.5 v 10.8). Noise.
- Submitted as ring-sprint-35 (parent ring-sprint-34). Verdict: loses to parent within noise,
  54-0-58 (server 32-36, custom 17-17, official 5-5); decided by elimination 28-27, queen 16-22,
  longest 10-9. v ring-shield-3 51-61, ring-sealed-2 56-56, ring-field-3 53-59. CPU p99 24.7M, max 39.1M.
- Eval P(win) v parent: .500 at r100/r200/r300 (104/94/84 games): play is identical before r400.
- Targeted spot in the 112 games v parent (decoded replays, draft v parent):
  - our queen is never 10+ at r400 (0 of 112 either side): she grows from ~r430, so the screen only
    starts once she does. Games where she is 10+ some time after r400: 23 v 23, alive at end 21 v 20.
  - rounds (r400+, queen 10+) with a 3-6 ally 2-3 from her: 249 v 215 of 969 v 963 (+16%).
  - late (r400+) queen deaths 11 v 12, all lunged; of a 10+ queen 2 v 3; with a 3-6 ally within 3: 2 v 2.
  - 3-6 stub head-on deaths within 4 of the queen r400+: 5 v 4; stub deaths within 6 of a 10+ queen 102 v 100.
- Replays: 278596 (devil, draft seat A): queen reaches 10 at r467; 3-6 stubs 741 and 877 sit 4-6
  from her on the side of a 14-long enemy (868) and a 12-long one (892) and are lunged r469/r471;
  after that her only ally is 12 long and she is lunged r489 at 11. 278594 (mirror): our queen
  survives, but no enemy came within 7 of her after r463; her 20 -> 5 drop is her own split, not
  screening. 278966 (around_unsw): the parent's 38-long queen is lunged r496 with no ally within 3.
- Conclusion: the screen fires but changes little; the screens are too few and too short to stop
  long hunters, and the situation is rare (2-3 big-queen deaths per 112 games).
