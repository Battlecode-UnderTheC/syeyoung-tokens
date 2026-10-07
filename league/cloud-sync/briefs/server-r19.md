# Analyst brief: server-r19 (where ring-sprint-1 loses on the competition server)

You diagnose; you don't propose code fixes. Read-only: never run `bcweb.py challenge`, never submit or upload, never edit the repo.

## Subject
- ring-sprint-1, server submission 17933, live since 2026-10-05 18:08 UTC. Team UnderTheC (id 328).
- Its server rating is 2136. Per map (`python3 scripts/server/bcweb.py submissions --maps`): weakest Devil 1858 (4-14), Trophy 1926 (10-16), Schooltime 2007, Default 2009, Queen Of Spades 2049; strongest Maze 2387, Portals 2322.

## Data and tools
- Repo: /home/user/syeyoung-tokens. `python3 scripts/server/bcweb.py battles|fetch ...` (read its --help) gets battles and replays of our submissions.
- Decoders and earlier analysis scripts: /home/user/syeyoung-tokens.league/replays/*/analysis/ and */tools (decode.py, render.py; server-r17 and arena-r18 are the newest). Reuse them.
- Arena eval P(win) curves for a replay: the arena API (`curl https://tokenz-arena.lonk-kelvin.ts.net/llms.txt`), `/api/v1/evals` and game metrics.
- Write data and scripts to /home/user/syeyoung-tokens.league/replays/server-r19/. Disk is limited: delete raw zips once decoded.
- Earlier reports for comparison: replays/server-r17/analysis/REPORT.md (ring-contest-1, its parent's code), arena-r18/REPORT.md (ring-sprint-1 in the arena), and the last sections of league/KNOWLEDGE.md.

## What to report
- Rating-adjusted performance (opponents' ratings at battle time), per map, and against common opponents compared with ring-contest-1 (server-r17). A raw W-L is not a measure.
- Loss causes ranked with numbers: by map, by phase (where P(win) turns), by map class (1 / 2 / 3+ starters; cramped / corridor / open), and by opponent tier.
- Separate STUPID losses (blunders: our dragon or queen does something plainly wrong) from STRATEGIC losses (the opponent's plan beats ours). For each, name 2-3 game ids and the round.
- Devil and Trophy specifically: what happens there, in concrete terms.
- What the free sprints (ring-sprint-1's change) do on the server: use rate, and deaths during or right after a sprint.
- What strong opponents visibly do that we don't.
- Movement inefficiencies (oscillation, target switching, dead-end entries, feeders that never arrive), quantified, with worst examples.
- Look at the games, not only the numbers: render several filmstrips (`render.py --film`, from the earlier analyses' tools) over the stretch where each loss cluster turns, view the PNGs with Read, and describe what the dragons do. Say which conclusions came from the images.

## Rules
- Maps are symmetric: never explain anything by the seat.
- Metrics are evidence: confirm each claimed behaviour in decoded replays.
- Never look in /home/user/syeyoung-tokens.league/maps/holdout/. Never print ~/.unswbc-web/credentials or ~/.unswbc/keys.json.
- Never kill processes by pattern; at most 2 threads.
- You can't write REPORT.md yourself: put the full report (markdown) in your final message, and the orchestrator saves it to replays/server-r19/analysis/REPORT.md.
