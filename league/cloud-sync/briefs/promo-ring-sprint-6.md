# Analyst brief: pre-promotion check of ring-sprint-6 (feeding conveyor)

You diagnose; you don't propose code fixes. Read-only: never submit, promote, duel or start server games; never edit the repo.

## Subject
- Draft `ring-sprint-6` = ring-sprint-2 (the line head) + the feeding conveyor (code: /home/user/syeyoung-tokens/variants/ring-sprint-conveyor; base: variants/ring-sprint-founder). Its note: `tokenz show ring-sprint-6`. Code is identical to the parent until r430.
- Verdict vs ring-sprint-2: 58-0-54, beats parent within noise (server 36-32). The orchestrator wants to make it the line head (rule: promote only if no map class is a clear loss).
- Reported by its worker, in the 44 games vs the parent where both queens were alive at r400: queen growth from r400 +13.5 v +11.2; longer-queen (margin ≥5) losses 11 v 13; **queen deaths after r400 14 v 11** (a side effect to examine). It replaces QueenRace and the r450 feed14 wave.

## Questions
1. Does the conveyor do what it claims in the arena games vs ring-sprint-2 (`tokenz games --sub ring-sprint-6 --vs ring-sprint-2 --limit 1000`, `tokenz replay`, per-game metrics and eval from the arena API, `curl https://tokenz-arena.lonk-kelvin.ts.net/llms.txt`)? From r430: feeders one at a time beside the queen, and she eats the line. Describe concretely.
2. Also pair its games v the other bots with ring-sprint-2's on the same opponent, seed and seat, as the ring-sprint-2 check did (replays/promo-ring-sprint-2/REPORT.md and its analysis/ scripts). Which games differ, and how?
3. By map class (1 / 2 / 3+ starters; cramped / corridor / open; sealed queen; central cluster or not): is any class a clear loss (not noise)? W-L and eval P(win) at r450/r480/r500 per class.
4. The extra queen deaths after r400: what kills her, and is it the conveyor's doing (e.g. she leaves her ring to meet feeders, feeders crowd her)?
5. Any other ugly side effect (stub churn, CPU: `tokenz report ring-sprint-6`).

## Method
- Reuse the decoders and tools of earlier analyses (/home/user/syeyoung-tokens.league/replays/promo-ring-sprint-2/, arena-r18/, server-r19/). Write data to /home/user/syeyoung-tokens.league/replays/promo-ring-sprint-6/; delete raw downloads once decoded (disk is limited).
- Look at the games, not only numbers: render several filmstrips (render.py --film or server-r19/tools/fl.py) over r420-r500 of conveyor games and of the late queen deaths, view the PNGs with Read, and say which conclusions came from images.

## Rules
- Maps are symmetric: never explain anything by the seat. Metrics are evidence: confirm behaviour in decoded replays.
- Never look in /home/user/syeyoung-tokens.league/maps/holdout/. Never print credentials. Never kill processes by pattern; at most 2 threads.
- Put the full report in your final message (the orchestrator saves it). End with a one-line verdict: "no class is a clear loss" or "clear loss in <class>: <evidence>".
