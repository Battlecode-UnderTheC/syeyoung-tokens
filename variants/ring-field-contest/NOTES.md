# ring-field-contest (g52-7), arena ring-field-20

- **Line:** ring-field. Base ring-field-freecost (arena ring-field-18).
- **Brief:** league/briefs/g52-7.md, strategist A g52 proposal #1.

## Idea
`contest_` (InMiddle: Exposed() ignores enemy-adjacent tiles at local head parity, WorthTrading
accepts 1-for-1 trades when ours > theirs within 4) was only set for a frontier explorer holding a
contested find (rush_state_ 2). Field-mode dragons are rush_state_ 3 (rush_deferred_), so the
dragons that actually arrive at a field never contested it (server-r26 cluster A; Devil "we arrive
with them but stand off at the wall").

## Change
- rush.cpp UpdateRush: before r60, any non-queen worker in rush_state_ 3 runs the contested test on
  beds (BedWeight > 0) within kContestRadius (4) of its head: an enemy head seen this turn within
  kContestReach (4). The nearest such bed becomes rush_target_ with rush_state_ 2, so contest_ is
  on while within kContestRadius; held to r60 like an explorer's find.
- field.cpp: field arrival clears rush_deferred_.
- Debug only (#ifdef CONTEST_DBG): CONTESTSET / CONTESTDBG / CONTESTEXP / CONTESTTRADE lines.

## Local
- InMiddle turns: ~76 a game for our side on maps/server (base: explorers only, which field mode defers).
  Devil/trophy/islands/stripes seeds 1-3: 1848 contest turns, 498 tiles accepted only by the
  contest parity rule, 54 trades accepted by it (24 games).
- Fresh mapgen (seed 52741, 44 maps, seeds 1,2): 88-88. Families 7-9 anti-centre, 6-10 crowded,
  7-9 maze, 9-7 mixed, 11-5 off-axis, 9-7 open, 10-6 rich-near-home, 9-7 ring (+14-10 on seeds 3-5),
  7-9 rooms, 7-9 sealed-spawn, 6-10 sparse.
- Light duels v ring-field-18: server 38-30 (3+ 25-15, <=2 13-15), official 10-10 (<=2 6-10),
  targeted arena/default_small/maze/stripes/trophy/devil x3 seeds 16-20.
- Paired duels v cs-queen-split-tail-risk-v1-7 (devil, trophy, islands, stripes, seeds 11-44):
  ours 22-10, base 26-6; opening eats r0-60 66.5 v 66.1 and dragons r60 20.0 v 19.7: no change.

## Arena verdict (ring-field-20)
- vs parent 56-0-56 (even); server 35-33, official 4-6, custom 17-17. vs ring-field-3 57-55,
  ring-qforage-1 55-57, ring-sprint-21 52-60. CPU p99 23.1M, max 40.5M.
- eval P(win) v parent: r60 .494, r100 .482, r200 .517, r300 .501.
- Opening from replays (112 games): eats r0-60 59.2 v 59.1, r20-40 21.0 v 21.2, eaten in their
  half 15.8 v 15.7, dragons at r60 17.9 v 17.8. The behaviour fires but doesn't move the opening.
- Trophy 0-4 (eats r0-60 34.5 v 62.2). Sim seed 17910617060193542843: a starter took a hold at r7
  on a bed by home and stayed tethered to it to r60. Tried releasing a worker's hold when no bed near
  it is contested any more (v2): Trophy seeds 5-5, but the fresh ring family went 14-26 (v1 23-17),
  duels server 35-33, official 10-10, fresh 84-92. Not adopted; this directory is v1 (= ring-field-20).

## Next
The parity/trade relax is a weak lever: the stand-off is probably decided by goal choice (where
arriving dragons aim), not by Exposed(). Measure against a stand-off opponent (TTT-like), not our own line.
