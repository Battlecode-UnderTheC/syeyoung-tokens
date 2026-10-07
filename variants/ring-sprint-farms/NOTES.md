# ring-sprint-farms (g48-2, arena ring-sprint-31)

- **Line:** ring-sprint. **Parent:** ring-sprint-guard2 (arena ring-sprint-21).
- **Brief:** strategist-g48 #2 (MID economy): bed rooms are farms, not traps.

## Idea
arena-r22's P2 growth stall (157 losses): even at r60, then +1 v +6 dragons by r100, bed share
.46 -> .27. In the code every pocket smaller than RoomNeeded (max(L+3, room_min 8)) is a dead end
charged the flat trap_cost (3 pearls), so a 2-3 long worker skips small bed pockets.

## Change (one)
`StrategyAgent::FarmPocket` (strategy.cpp, FARM_MODE): in ChooseGoal's dead-end check, a 2-3 long
non-queen worker values a pocket as a farm (cost 0, grow) when
- it holds 3+ live beds (pearl now, or a bed due within 10 rounds);
- our other heads touching the pocket are fewer than beds/2 (children born inside count);
- capacity: pocket >= L + beds + 2 tiles, or a free unit slot and L + beds >= 4 (split at 4 inside);
- no enemy head seen this turn within its rule reach (ceil(Le/4)+Le-2) of the pocket or its door.
SafeStep takes the farm step without the room_min check ("farm: ..." labels; the plugin's judge
skips dead-end steps as before). A child with no capacity left fails the check and walks out.

## Local
- `check ring-sprint-farms maps/official`: passed.
- Server maps seed 1 v ring-sprint-guard2: 17-0-17. Trapped L2-3 no-move resigns 190 v 203.
- Fresh mapgen (seed 48207, 44 maps, seeds 1,2): 92-0-84. Per family: anti-centre 8-8, crowded
  8-8, maze 8-8, mixed 8-8, off-axis 9-7, open 8-8, rich-near-home 10-6, ring 8-8, rooms 9-7,
  sealed-spawn 8-8, sparse 8-8. The farm step fires only ~1-4 times a match: most walled bed rooms
  (incl. film 212730's 12-bed room, 45+ tiles with its ring) are not pockets under RoomAround at all.

## Arena
- Duels v ring-sprint-21: server random:2 34-0-34; official random:2 11-0-9 (both within noise).
- **Verdict (ring-sprint-31):** 57-0-55 vs ring-sprint-21 (50.9%, CI 41.8-60.0, within noise; the
  arena flags beats_parent). By suite: server 33-35, custom 19-15, official 5-5. Decided by:
  elimination 28-27, queen 18-16, longest 11-11, total 0-1. Others: ring-field-3 50-62,
  ring-qforage-1 54-58, spar-field-2 47-65.
- CPU: p99 24.7M, max 40.9M.
- Eval P(win) vs parent: r100 0.497, r200 0.491, r300 0.489 (104/98/89 games).
- Replays (105 of 112 games, draft side v parent side in the same games; /tmp tool, door pockets =
  regions under 8 tiles behind one door tile): pearls eaten in pockets r20-100 by 2-3 long workers
  7.9 v 7.5 a game; all pocket eats 13.1 v 12.9; eats r60-100 66.1 v 67.7; dragons r60 17.3 v 17.5,
  r100 22.8 v 23.4; length-2 resigns before r300 63.7 v 63.2.
- Film 226697 (Stripes, draft = seat A): draft stubs enter the 2x2 bed boxes at (7,1-2) and
  (20-21,11) from r24 and re-farm them every ~10 rounds; the parent's first box eat there is r73.
  Most other boxes still stand full for both sides (an enemy head in reach, or not yet seen), and
  the parent still leads on dragons by r60 (8 v 5). 226724 (maze): draft farms two pockets from r45.
- Conclusion: the behaviour is what the brief asked for, but it is too rare to move results. The
  big walled bed rooms the evidence names (212730's 12-bed room) are not pockets under RoomAround,
  so this change never touches them; what keeps them full is that nobody goes there (exploration,
  strategist-g48 #1), not the trap cost.
