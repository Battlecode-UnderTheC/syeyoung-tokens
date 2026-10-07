# Analyst brief: flips-r20 (game-flipping losses of the current heads)

You diagnose; you don't propose code fixes. Read-only: never submit, promote, duel or start server games; never edit the repo.

## What to find
A **flip** is a loss we were winning: our eval P(win) was high (≥ 0.65 at some round after r50) and we lost. The user's goal now is to get rid of these. For each flip, find the event that turned it: the window where P(win) falls the most, and what happened there (which of our dragons died or did what, to whom, why).

## Games
1. Arena: every loss of `ring-sprint-16` and `ring-qforage-1` (the line heads) and of `ring-sprint-6` (their parent) against the other bots. `tokenz games --sub <id> --limit 2000` (count unique game ids; the listing can repeat rows); per-game metrics and eval from the arena API (`curl https://tokenz-arena.lonk-kelvin.ts.net/llms.txt`: GET /api/v1/games/{id}/metrics has game.eval every 10 rounds); replays via `tokenz replay`.
2. Server: ring-sprint-1's server losses (live; server-r19 decoded all 370 games: /home/user/syeyoung-tokens.league/replays/server-r19/, data/ and analysis/rows.json have eval curves and features). Real opponents matter most.

## Report
- How many losses are flips (by the ≥0.65 definition; also give ≥0.8), by map class and phase of the flip.
- The flip events, clustered by cause and ranked by count, each with 3+ game ids and rounds, and how much P(win) dropped. Separate blunders (our dragon does something plainly wrong: walks into reach, boxes itself, splits to death, a feeder lays toward enemies, the queen leaves safety) from being outplayed.
- For each top cause: what our dragons observed at the time (was the killer visible, how far, our room, allies around), so a worker can build a fix from observations, not map knowledge.
- Which causes the running tasks already target (g45-7: enemy queen death detection; g45-8: lunge guard on long workers; g45-5: late concentration on an observed enemy giant; g45-6: earlier conveyor in a queen race) and which are untouched.
- Look at the games, not only numbers: render filmstrips (server-r19/tools/fl.py or render.py --film) over the flip window of the top causes, view the PNGs with Read, and say which conclusions came from images.

## Rules
- Maps are symmetric: never explain anything by the seat. Metrics are evidence: confirm in decoded replays.
- The goal is winning games; the queen's survival is an advantage, not a constraint.
- Write data to /home/user/syeyoung-tokens.league/replays/flips-r20/; delete raw downloads once decoded (disk is limited).
- Never look in /home/user/syeyoung-tokens.league/maps/holdout/. Never print credentials. Never kill processes by pattern; at most 2 threads. Don't leave background wait loops running when you finish.
- Put the full report in your final message (the orchestrator saves it).
