# Analyst brief: pre-promotion check of ring-qforage-1 (the queen forages through the opening)

You diagnose; you don't propose code fixes. Read-only: never submit, promote, duel or start server games; never edit the repo.

## Subject
- Draft `ring-qforage-1` (new line ring-qforage) = ring-sprint-6 (the ring-sprint head) + the queen foraging and splitting through the opening on 3+-dragon starts, then parking (code: /home/user/syeyoung-tokens.worktrees/g45-2/variants/ring-qforage; base: /home/user/syeyoung-tokens/variants/ring-sprint-conveyor). Its note: `tokenz show ring-qforage-1`.
- Verdict vs ring-sprint-6: 61-0-51 (beats parent, within noise; server 35-33). Its worker reports eval P(win) .550/.536/.536 at r100/200/300 (3+ starters .573/.552/.553), 1-2-starter games identical, 3+ starters 43-33; queen circling r0-60 on 3+ starters 0.36 v 0.69; queen pearls by r100 2.36 v 1.11.
- **The risk:** queen death before r100 on 3-4-starter maps 14% v 11% (6 v 5 of 44), over the brief's ~12% abort line; two new self-made deaths ("no valid move": 199077, 198834) where she foraged among her own dragons and an ally head took her last exit.

## Questions
1. Does she do what it claims in the arena games vs ring-sprint-6 (`tokenz games --sub ring-qforage-1 --vs ring-sprint-6 --limit 1000`; count unique game ids, the listing can repeat rows; `tokenz replay`; per-game metrics and eval from the arena API, `curl https://tokenz-arena.lonk-kelvin.ts.net/llms.txt`)? Describe concretely what she and the team do differently in r0-120.
2. Pair its games v the other bots with ring-sprint-6's on the same opponent, seed and seat (as replays/promo-ring-sprint-6/REPORT.md did; reuse its analysis/ scripts). Which games differ, and how?
3. By map class (1 / 2 / 3+ starters, and 3-4 v 5+; cramped / corridor / open; sealed queen; central cluster or not): is any class a clear loss (not noise)? W-L and eval P(win) at r60/r100/r200 per class.
4. **Queen safety:** every queen death before r150 in the differing games, both bots: cause (enemy lunge, self-made no-valid-move, boxed by allies), where she was and why. Is the rise noise or a pattern? How often does an ally take her last exit, and does the parent suffer the same?
5. Any other side effect (worker economy when she takes home food, stub churn, CPU: `tokenz report ring-qforage-1`).

## Method
- Reuse decoders and tools from replays/promo-ring-sprint-6/, promo-ring-sprint-2/, server-r19/. Write data to /home/user/syeyoung-tokens.league/replays/promo-ring-qforage-1/; delete raw downloads once decoded (disk is limited).
- Look at the games, not only numbers: render several filmstrips over r0-120 of differing games and of every early queen death, view the PNGs with Read, and say which conclusions came from images.

## Rules
- Maps are symmetric: never explain anything by the seat. Metrics are evidence: confirm behaviour in decoded replays.
- Never look in /home/user/syeyoung-tokens.league/maps/holdout/. Never print credentials. Never kill processes by pattern; at most 2 threads.
- Put the full report in your final message (the orchestrator saves it). End with one line: "no class is a clear loss" or "clear loss in <class>: <evidence>", and one line on queen safety: "noise" or "pattern: <cause>".
