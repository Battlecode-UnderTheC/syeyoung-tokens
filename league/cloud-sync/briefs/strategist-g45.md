# Strategist brief: g45 (strategic losses of the ring-sprint line)

You propose; you don't write code, submit or run games. Read-only.

## Read
- /home/user/syeyoung-tokens.league/replays/server-r19/analysis/REPORT.md: ring-sprint-1's 370 server games (the newest and most important evidence). Its data and scripts are beside it if you need a number it doesn't give.
- /home/user/syeyoung-tokens.league/replays/arena-r18/REPORT.md: ring-sprint-1's arena losses.
- /home/user/syeyoung-tokens.league/replays/promo-ring-sprint-2/REPORT.md: what the founder (now in the head) does and where it fails (tower_defense, trophy).
- /home/user/syeyoung-tokens/league/KNOWLEDGE.md (last ~10 sections) and league/generations/g44.md: what has been tried, with results. Don't re-propose a dead end unless you name what is different.
- /home/user/syeyoung-tokens.league/replays/devtest1/REPORT.md: how the server's #1 plays.
- Arena metrics if useful: `curl https://tokenz-arena.lonk-kelvin.ts.net/llms.txt`.

## The head
ring-sprint-6 (code /home/user/syeyoung-tokens/variants/ring-sprint-conveyor) = ring-sprint-1 + the founder dispatch (2-starter openings) + a late feeding conveyor. Already being worked on, so not yours: the queen killed while circling beside a visible hunter (S1), relayed long-enemy sightings, frontier pickets, the no-live-race queen gate.

## Task
Propose 3-5 ranked SINGLE changes for the STRATEGIC losses: above all the opening snowball (51% of server losses decided by r60: Devil, Trophy, Stripes; we eat at home and never reach the middle or the portals), then the late queen race and longest tiebreak. For each:
- the evidence (numbers and game ids from the reports);
- the base (ring-sprint-6) and the mechanism, concretely enough for a worker to build ONE change;
- how to tell whether it worked: the target metric in the draft's own arena games, and what must not get worse.

## Rules for proposals
- No map knowledge: no map names, board-size gates, geometric-centre targets, fixed positions or constants fitted to known maps. Mechanisms must react to what dragons observe. Maps are symmetric: never explain anything by the seat.
- The queen plays safe: she never enters mazes, corridors or pockets.
- No exploits of one opponent, no RL, no env vars. Prefer general mechanisms (valuing food, room, danger, numbers) over new 'if X starters' rules.
- Put the full proposal in your final message; the orchestrator saves it to strategy/strategist-<x>-g45.md.
