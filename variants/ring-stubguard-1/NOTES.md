# ring-stubguard-1 (g42-5)

Base: ring-sched-17 (code: variants/ring-sched-m3). Arena draft: **ring-sched-32**.

## Idea
arena-r16 found 165 mid-game queen deaths with an enemy of length 4 or less within 3 tiles at her
last turn. In 103 of them no ally head was within 3. QueenRisk only vetoes the queen's tiles, and
WorthTrading rarely approves an even trade, so escorts let enemy stubs sit beside the queen.

## Change (one mechanism: assign an interceptor; stubguard.cpp)
- **Threat:** a visible enemy head within `kGuardRadius` = 4 BFS steps of our queen. The queen's
  position is her head in view, or otherwise her broadcast ring (fresh for 12 rounds). The
  threshold is one more than the 3 tiles in the evidence, which gives an escort a turn to arrive.
- **Trade:** a worker that can reach the threat's tile this turn trades into it head-on. Its reach
  is ceil(L/4) free steps plus one paid step per segment above 2; the paid steps cost nothing it
  keeps, because it dies in the trade. The cap is `kGuardMaxSteps` = 6, the Manhattan reach of the
  7x7 window. The worker defers when a shorter ally (length 4 or less, `kGuardShort`) can also reach
  the threat and acts after it in id order.
- **Approach:** when no visible ally can reach the threat this turn, the nearest ally takes one free
  step towards it. Ties go to the shorter ally, then the lower id. The step is never into the ring
  and needs EscapeLevel > 0.
- **Hooks:**
  - StubGuard runs in `Turn()` after RingWorker and Supply14Yield. It is skipped when another queen
    protection already committed the turn.
  - `ProtectQueenAction` passes the guard's action through unchanged, since the enemy head can sit
    on a ring tile.
  - A visible enemy queen in reach is left to TryQueenAttack.
- **Unchanged:** QueenRisk and the queen's own evasion. When she is in danger, she already leaves
  the ring through the risk-penalised beam.
- **Dropped (v2):** the queen named the threat in her 0xBA ring word, and allies that could not see
  it walked towards it from up to 8 steps away. Two server duels against ring-sched-17 went 32-36
  and 32-36, so it was dropped.

## Local / duel numbers (v1, the submitted code)
- **Duels vs ring-sched-17:**
  - server suite (random:2): 38-30 and 36-32;
  - official suite: 9-11.
  - Eval P(win) at r100/200/300: 0.495/0.502/0.515 and 0.500/0.506/0.502.
- **Duel vs cs-queen-split-tail-risk-v1-7 (server):** 34-34. ring-sched-17 went 44-24 in its own
  duel against it, on different seeds.
- **Fresh mapgen** (seed 742019, 44 maps, seeds 1 and 2) vs ring-sched-m3: 85-0-91 overall.

  | family | W-L | our queen died | m3's queen died |
  |---|---|---|---|
  | open | 6-10 | 94% | 81% |
  | rich-near-home | 8-8 | | |
  | maze | 8-8 | | |
  | rooms | 5-11 | 88% | 75% |
  | sealed-spawn | 7-9 | | |
  | ring | 11-5 | | |
  | off-axis | 6-10 | | |
  | sparse | 8-8 | | |
  | crowded | 11-5 | | |
  | anti-centre | 8-8 | | |
  | mixed | 7-9 | | |

  - Over all 176 games our queen died in 77%, against 72% for m3.
  - Most of these games are seat-mirrors that play out identically. The differences come from a few
    games that diverge. For example, on rooms-036 seed 1 a guard trade at r36 changed the whole game.
- **How often it fires:** rarely. In sims about 0.2-1 trades a match. In the duels, reachable stub
  episodes (an enemy head that can reach the queen within 4 and an ally that can reach it) came up
  only 3-17 times in 68 games.

## Arena verdict (ring-sched-32)
- **vs parent ring-sched-17:** 55-0-61, 47.4%. The arena says it "loses to parent" (within noise).
  - By suite: server 36-36, custom 14-20, official 5-5.
  - Decided by: elimination 22-26, longer queen 22-26, longest dragon 11-8.
- **vs others:**
  - ring-s17-1: 55-57;
  - ring-rush-1: 67-45;
  - cs-queen-split-tail-risk-v1-7: 68-44.
- **Eval P(win) vs the parent:** 0.511 at r100, 0.500 at r200, 0.491 at r300.
- **CPU:** p99 23.9M, max 38.4M.
- **Target metric in the 116 games vs the parent, ours v parent:**
  - queen deaths before r430: 49 v 44;
  - hazard at d=1-2 (r60-429): 17.9 v 13.6 per 1,000 turns;
  - trades by non-queens within 5 of their own queen: 75 v 66, costing 278 v 195 length.
  
  It did not drop.

## Replays
- **Duel ed6fde6f, game 56** (Default, r97-99): this is the intended behaviour. Escort 8:3 sits
  ahead of our 2-long queen in a corridor and trades head-on with enemy stub 16:3, 2 tiles from her.
  The queen then eats the corpse pearls (2 → 4).
- **Duel 4787673a, game 40** (around_unsw, r368-369): escort 1204:3 trades into 1227:2, which sat
  beside the queen. Other stubs (1224, 1319, 1261) still surround her.
- **Duel 7aa091e1, game 21** (Australia, r256-262): a 2-long escort chases enemy stub 440:2
  alongside the queen. The stub moves after it in id order, so the gap stays at 2 and it never
  catches it. A 2-long dragon can't take a paid second step.
- **Schooltime episodes** (duel 4787673a, game 23): the enemy circles the queen's sealed pen.
  These are not reachable threats, so the guard correctly does nothing.

## Next
- Keep non-queen starters (leaders) out of the trade. Rooms-036 lost its leader 2 in a guard trade
  before the queen fell.
- Prefer 3+-long escorts, which can close 2 tiles in one move, over 2-long ones that can only shadow
  a stub.
