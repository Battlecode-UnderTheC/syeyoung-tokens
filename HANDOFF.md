# Handoff: orchestrating the dragons bot (as of 2026-10-06, g44 running)

This brief is for a fresh session that takes over orchestration. Read it first, then `ORCHESTRATOR.md` in full (that's the binding workflow), then the latest `league/generations/g4*.md`.

## Goal
Reach **#10 on the competition server leaderboard** (game.battlecode.au, team UnderTheC, id 328) by slow, incremental improvement.
- **Now:** #22, rating 2099. #10 is about 2196.
- **Path so far:** #31 (2032) → #27 → #22.
- **How to judge:** by server Elo and the server's **per-map rating** (`python3 scripts/server/bcweb.py submissions --maps`). A raw server W-L means nothing: samples are small, and the user sometimes picks opponents.

## Your role
You are the orchestrator. You coordinate; you don't analyse strategy or write bot code yourself.
- **Analysts** diagnose losses: replays, clusters, image filmstrips. Only analysts look at images.
- **Strategists** design fixes for strategic losses.
- **Workers** each build ONE change.
- **You:** route work, record results, submit, deploy, and keep agents busy.
- Run routine steps without asking. Ask the user only for outward actions that aren't authorised, or for real strategy calls.

## The loop (details in ORCHESTRATOR.md)
1. **Analyse** the head's arena losses (and server losses) by replay. Split them into:
   - **stupid losses** (blunders): these go straight to fix workers;
   - **strategic losses**: these go to strategists.
   Analysts give diagnoses, not fixes.
2. **Workers make one change each on the current head** (`--parent <head>`), using the template `league/BRIEF-ARENA.md`. Each worker:
   - tests with light arena duels (`tokenz ephemeral` + `tokenz duel ... --suite server --seeds random:2`) and freshly generated maps (`tools/mapgen`);
   - submits with `tokenz --agent <task> submit <dir> --bot <line> --parent <head>`;
   - **checks its own draft's arena games** for the targeted weak spot, paired against the parent;
   - fixes only bugs or incompleteness. Side effects go to the next round.
3. **Advance the head continuously.** The newest draft that beats its parent becomes the base for the next workers.
   - **No bundled merges.** Winning changes from other drafts are ported onto the head one at a time, as ordinary worker tasks, each with its own verdict.
   - Before promoting in the same line, keep the old head alive with its own line or a resubmit, because promoting retires the parent.
4. **Deploy** the best bot to the server via the arena, at most every 4 hours: `curl -X POST https://tokenz-arena.lonk-kelvin.ts.net/api/v1/server/upload -H 'Authorization: Bearer orchestrator' -H 'Content-Type: application/json' -d '{"submission":"<id>"}'`. The user authorised this. You may also launch server games against opponents.

## Current state
| Line | Head | Notes |
|---|---|---|
| **ring-sprint** | **ring-sprint-1** (variants/ring-sprint-1) | **Live on the server.** Arena #1, about 1632. = ring-contest-1 + free extra steps for length-5+ dragons |
| ring-contest | ring-contest-4 (= ring-contest-1 code, variants/ring-contest-m1) | Previous head. ring-contest-1 itself is retired |
| ring-sched | ring-sched-17 (variants/ring-sched-m3) | Older reference |

**g44 workers running** (each one change on ring-sprint-1, parent ring-sprint-1):

| Task | Variant | Change |
|---|---|---|
| g44-1 | ring-sprint-founder | port the founder (ring-contest-16: Colosseum 8-0 in its arena games) |
| g44-2 | ring-sprint-portal | port portals (ring-contest-13: more portal use) |
| g44-3 | ring-sprint-guard | port the lunge guard + escape run (ring-contest-10: −23% long-dragon deaths) |
| g44-4 | ring-sprint-qdead | the queen stays parked when there's no live race (S1: queen dies after the enemy queen is dead; `EnemyQueenRace()` is true when she was never seen) |
| g44-5 | ring-sprint-nobleed | the queen doesn't split herself to death when boxed (S5) |
| g44-6 | ring-sprint-picket | frontier pickets: a role at birth that holds the contested band (T1) |

**Queued:** strategist g44 #1 (race feeding on the race gate, after g44-4). Then #3-5 in `../syeyoung-tokens.league/strategy/strategist-g44.md`.

## Key findings (latest analysis: `../syeyoung-tokens.league/replays/arena-r18/REPORT.md`)
- **Opening snowball on small and crowded maps** is the biggest loss pool. At r30 we have 5.8 v 8.0 dragons. We eat at home while they take the contested middle and our side. Several attempts (task board, contest-3, lonesplit) failed to move it.
- **Late queen race:** we have more material but don't turn it into queen length. Feeding needs a fresh sighting.
- **Queen dies after the enemy queen is dead** (C2): resign waves feed a queen with no race, and she walks out.
- **Portals:** gst-42's crossing fix was in our ancestry. queen-ring-portal-fixed-1's ring relay broke its "clean ping" (fixed in ring-contest-13).
- **Trophy and Colosseum** are the weakest maps. Colosseum is fixed by the founder; Trophy is not.
- **The server's #1, "dev test 1",** wins on early economy and spread, free sprints, cull-and-harvest, and playing on after its queen dies (`../syeyoung-tokens.league/replays/devtest1/REPORT.md`).

## User rules (binding)
- **Map knowledge:** no map keys, no geometric-centre targets, no exploits of one opponent, no env vars, no RL. **Maps are symmetric: never explain anything by the seat.**
- **The queen plays safe:** she never enters mazes, corridors or pockets.
- **Metrics are evidence, not goals:** confirm behaviour in the replays.
- **Line names** describe the real mechanism (e.g. ring-sprint, not ring-next).
- **Don't build tools or extras nobody asked for.** Keep mid-run messages to agents to new evidence or the user's rules, with no planting of your own conclusions.
- **Ordering and cleanup:** strategists come after analyses. Promoting retires the parent.
- **Never:**
  - print `~/.unswbc/keys.json` or `~/.unswbc-web/credentials`;
  - look in `../syeyoung-tokens.league/maps/holdout/`;
  - kill processes by pattern;
  - push or deploy `../tokenz-arena` (a separate session maintains it).
- **Disk is tight:** workers delete their temp output.

## Where things are
- **Workflow:** `ORCHESTRATOR.md`.
- **Worker brief template:** `league/BRIEF-ARENA.md`; briefs are written to `../syeyoung-tokens.league/briefs/`.
- **Round notes:** `league/generations/g4*.md`. **Knowledge:** `league/KNOWLEDGE.md`.
- **Reports:** `../syeyoung-tokens.league/replays/{arena,server}-r*/` (subagents can't write REPORT.md; save it from their final message).
- **Arena:** `tokenz` CLI (https://tokenz-arena.lonk-kelvin.ts.net, /llms.txt): `lb`, `show`, `games`, `lineage`, `promote`, `parent`, `server status`, `server versions`.
- **Server:** `scripts/server/bcweb.py` (read-only commands, plus `challenge` for games).

## Cloud-session caveats
This setup assumes the Mac:
- the sibling dirs `../syeyoung-tokens.league` and `../tokenz-arena`;
- the `tokenz` client and its auth;
- the bcweb credentials;
- the local build (`cmake`, `build/tools/sim`).

A cloud session needs the arena reachable over Tailscale, the `tokenz` client, and credentials provided by the user. Without them it can analyse code and plan, but not submit, duel or deploy.
