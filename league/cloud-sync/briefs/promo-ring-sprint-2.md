# Analyst brief: pre-promotion check of ring-sprint-2 (founder port)

You diagnose; you don't propose code fixes. Read-only: never submit, promote, duel or start server games; never edit the repo.

## Subject
- Draft `ring-sprint-2` = ring-sprint-1 + the founder dispatch ported from ring-contest-16 (code: /home/user/syeyoung-tokens/variants/ring-sprint-founder; base: variants/ring-sprint-1). Its note: `tokenz show ring-sprint-2`.
- Verdict vs ring-sprint-1: 58-0-54, beats parent within noise. The orchestrator wants to make it the line head (rule: promote only if no map class is a clear loss).
- Already reported by its worker: Colosseum 6-0 v the parent's 3-3; dragons at r20/40/60 3.2/16.0/31.0 v 2.0/5.3/15.8. Flagged side effects: trophy 4-8 v 5-7, tower_defense 3-9 v 6-6.

## Questions
1. Does the founder do what it claims in the arena games vs ring-sprint-1 (`tokenz games --sub ring-sprint-2 --vs ring-sprint-1 --limit 1000`, replays via `tokenz replay`, per-game metrics and eval P(win) from the arena API: `curl https://tokenz-arena.lonk-kelvin.ts.net/llms.txt`)? Describe concretely what the dragons do differently in the opening.
2. Split the 112 games by map class: 1 / 2 / 3+ starters; cramped / corridor / open; sealed queen; central cluster or not. Is any class a clear loss (not noise)? Give W-L and eval P(win) at r100/r200/r300 per class.
3. Trophy and tower_defense: what goes wrong there compared with the parent on the same seeds? Is it the founder's doing?
4. Any other ugly side effect (queen safety, stub churn, CPU spikes: `tokenz report ring-sprint-2`).

## Method
- Decoders and tools from earlier analyses: /home/user/syeyoung-tokens.league/replays/*/analysis/ and */tools (arena-r18 is the newest arena one). Write data to /home/user/syeyoung-tokens.league/replays/promo-ring-sprint-2/; delete raw downloads once decoded (disk is limited).
- Look at the games, not only numbers: render several filmstrips (render.py --film) over the openings and over the trophy/tower_defense losses, view the PNGs with Read, and say which conclusions came from images.

## Rules
- Maps are symmetric: never explain anything by the seat. Metrics are evidence: confirm behaviour in decoded replays.
- Never look in /home/user/syeyoung-tokens.league/maps/holdout/. Never print credentials. Never kill processes by pattern; at most 2 threads.
- Put the full report in your final message (you can't write REPORT.md; the orchestrator saves it). End with a one-line verdict: "no class is a clear loss" or "clear loss in <class>: <evidence>".
