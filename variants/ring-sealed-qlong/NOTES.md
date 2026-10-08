# ring-sealed-qlong (g52-17)

Base: ring-sealed-wallin (arena ring-sealed-2, line ring-sealed). Strategist A g52 proposal #5.
The parent's notes are in git history (variants/ring-sealed-wallin/NOTES.md).

## Idea
server-r26: our queen is 2-3 from r100 to r400 and leaves her ring only at r430 (Ring4Exit), while
opponents' queens are 17-56 at r300-350 in our queen-race losses (75 queen-tiebreak losses, Trauma
lost all 15). Leave the ring earlier only when staying parked is a near-certain tiebreak loss: when
we have SEEN their queen long.

## Change
- `enemy_queen_len_`: the longest the enemy queen (id 0/1 of the other team) was seen from
  kQGateFrom = r300 (seen_[q].length, a lower bound when partial), by this dragon or heard.
- Relayed in bits 36-41 of the ring word (0xBA). Those bits carry the opening call only before
  kCallUntil + kCallFresh (~r48); a reader takes them as a length only from r300 and only when the
  call-surplus bits 48-51 are 0.
- `QueenExitFrom()` = r370 if enemy_queen_len_ >= 12 (3x the 2-4 she is parked at), else r430.
  It replaces 430 in: the queen's Ring4Exit gate (QueenMove); her post-exit safety checks
  (RING11 closed-branch, RING6 continuation, RING6 unseen-endpoint; `QueenLateFrom()` only while
  she is growing); and the workers' feeding (Conveyor, QueenRace, Ring5Reserved).
- `qlong_exit_` keeps an r370 exit from being read as the opening's early roaming (EARLY15
  settle logic reset queen_growing_ for any growing queen before r430: first trace exited every
  second round and stayed at 2).
- Without the feeding gate she exited at r370 and stayed at 2 until r430 (no pearls left near her
  on slithery_fight v ring-bigq-1); with it she grew 2 -> 16 by r430 and ended 36 (base: 2 at r430).
- Debug: -DQLONG_DBG prints QLONG (r370/r430 state), QLONGEXIT, QLD (queen trace after an early exit).

## Local (sim, 2 threads)
- v ring-sealed-wallin, server s1: 17-0-17, identical mirror (the gate never fires: the base's
  queen is <= 5). Fresh mapgen (--family all --seed 5217, 44 maps, s1-2): 87-2-87, identical.
- Gate fire rate, server s1-2: v ring-bigq-1 1 early exit in 49 games with our queen alive at r370;
  v cross-queengrow-1 0/44; v giant-8 4/45. W-L unchanged (49-19, 58-10, 64-4 for both);
  queen at end 11.8 v 11.7 (bigq), 8.0 v 7.4 (giant-8).

## Arena duels (eph bot before the qlong_exit_/feeding fix; neither matters when the gate never fires)
- v ring-sealed-2: server 34-0-34, official 10-0-10, metrics identical on both sides.
- Same seeds (11,12) as the parent's duels: v codex-queen-value-1 58-10 (parent 56-10, 2 errors),
  v spar-field-2 35-33 (=), v ring-qforage-1 33-35 (=).
- Why: decoded replays show no arena bot's queen is >= 12 at r370 (qforage/spar/queen-unified/
  queen-ring17/queen-ring14/cs-queen-split all 0-6; codex 2 of 68 at 19-22, and our views saw only
  2-3 segments of them). The arena cannot test this gate; only server opponents that grow early can.

## Arena verdict (ring-sealed-4, parent ring-sealed-2)
- v parent 56-0-56 (even; server 34-34, official 5-5, custom 17-17); decided by elimination 26-26,
  queen 19-19, longest 10-10, total 1-1. Eval P(win) v parent 0.500 at r100/r200/r300: exact mirrors.
- v ring-field-3 59-53, v ring-qforage-1 55-57, v ring-sprint-21 56-56.
- CPU: p99 22.7M, max 47.4M.
- The targeted weak spot (server queen races v opponents whose queen is long by r300-370) does not
  occur in the arena, so these games can't show whether it is fixed; the server's own games can.
