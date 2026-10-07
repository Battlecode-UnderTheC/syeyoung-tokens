# Analyst brief: pre-promotion check of ring-sprint-16 (the queen keeps room as her space shrinks)

You diagnose; you don't propose code fixes. Read-only: never submit, promote, duel or start server games; never edit the repo.

## Subject
- Draft `ring-sprint-16` = ring-sprint-6 (the line head) + the queen's route choice charged for the room it leaves her, outside the opening (code: /home/user/syeyoung-tokens.worktrees/g44-10/variants/ring-sprint-qroom, see the g44-10 comment block in strategy.hpp; base: /home/user/syeyoung-tokens/variants/ring-sprint-conveyor). Its note: `tokenz show ring-sprint-16`. An earlier version, ring-sprint-12, lost 55-57.
- Verdict vs ring-sprint-6: 58-0-54 (beats parent, within noise; server 36-32). The worker reports, paired with the parent over 224 games (208 identical): queens boxed or bled to death 9 v 13 (after r400 1 v 3); fixed in 202279 (portals) and 202361 (stripes). **Side effect:** four late long queens lunged in open ground where the parent's (bled but) survived: 202344, 202536, 202555, 202407.

## Questions
1. Does she keep room as claimed in its arena games vs ring-sprint-6 and v the other bots paired with ring-sprint-6's games (same opponent, seed, seat; reuse replays/promo-ring-sprint-6/analysis scripts; count unique game ids, the listing can repeat rows)? Describe concretely what she does differently and when.
2. By map class (1 / 2 / 3+ starters; cramped / corridor / open; sealed queen; central cluster or not): is any class a clear loss (not noise)? W-L and eval P(win) at r300/r400/r500 per class.
3. The side effect: every queen death after r300 that differs from the parent: cause, where she was, how far the killer came from, whether the room rule put her there. Net, are queen deaths and queen-decided results better, worse or even?
4. Any other side effect (her growth, the conveyor, CPU: `tokenz report ring-sprint-16`).

## Method
- Write data to /home/user/syeyoung-tokens.league/replays/promo-ring-sprint-16/; delete raw downloads once decoded (disk is limited).
- Look at the games, not only numbers: render several filmstrips over the late game of differing games and of every differing queen death, view the PNGs with Read, and say which conclusions came from images.

## Rules
- Maps are symmetric: never explain anything by the seat. Metrics are evidence: confirm behaviour in decoded replays.
- Never look in /home/user/syeyoung-tokens.league/maps/holdout/. Never print credentials. Never kill processes by pattern; at most 2 threads.
- Put the full report in your final message (the orchestrator saves it). End with one line: "no class is a clear loss" or "clear loss in <class>: <evidence>", and one line on queen deaths: "net better", "net even" or "net worse: <evidence>".
