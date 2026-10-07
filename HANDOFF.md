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
- **The queen:** keeping her alive is important, but there's no queen rule. Risks that win games are allowed.
- **The goal is to win games, not to keep the queen alive at all costs.** Her survival is an advantage and evidence to weigh, never a cap or abort line by itself (2026-10-06: I wrongly held ring-qforage-1 back on a doubled early queen death rate while it won).
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

## State at stop (2026-10-07, cloud session stopped by the weekly usage limit) — READ THIS FIRST
The sections above ("Current state", "g44 workers running") are out of date; this section replaces them. Round notes: `league/generations/g44.md` (closed in substance) and `g45.md` (open). Everything is committed and pushed: this session's work is on branch `claude/task-t0ew7k` (main was fast-forwarded to it locally only; **origin/main is behind: merge or fast-forward `claude/task-t0ew7k` into main first**), and every task branch `task/g44-*`, `task/g45-*` is on origin.

**Ladder:** #19, 2146 (#10 is 2270). Live: ring-sprint-1 (no deploy this session: no candidate cleared the gate; ring-qforage-1's holdout 52.8% v the live bot's 62.1%).

**Line heads (arena):**
| Line | Head | Code | = |
|---|---|---|---|
| ring-sprint | **ring-sprint-16** | variants/ring-sprint-qroom | ring-sprint-1 + founder (ring-sprint-2) + feeding conveyor (ring-sprint-6) + the queen keeps room (ring-sprint-16). Each promoted after an analyst's replay check (replays/promo-ring-sprint-{2,6,16}/REPORT.md) |
| ring-qforage | **ring-qforage-1** | task/g45-2 first commit 4278391 (not merged to main) | ring-sprint-6 + the queen forages through the opening on 3+-dragon starts. Arena #1 ~1625. Early queen deaths x2 v other bots (replays/promo-ring-qforage-1/REPORT.md) |
ring-sprint-1 (live on the server) is retired from the arena (user's choice).

**League files written this session** (briefs, server-r19 and promo-* reports, strategist-a/b-g45) are copied in `league/cloud-sync/`: copy them into `../syeyoung-tokens.league/` before starting.

**User direction, latest first:**
1. **Focus on game-flipping losses** (losses we were winning: eval ≥0.65 after r50, then lost). Analyst flips-r20 was briefed (`../syeyoung-tokens.league/briefs/flips-r20.md`) and died before reporting: **relaunch it first**; new tasks come from its top causes.
2. **Winning is the goal; the queen's survival is an advantage, not a constraint.** There is no queen rule (user, 2026-10-07): keeping her alive is important, but risks are allowed.
3. Arena improvement has translated to the server; no need to launch server games before deploying.
4. Strategic v stupid losses, and per map, from server analyses (server-r19: `../syeyoung-tokens.league/replays/server-r19/analysis/REPORT.md`).

**Unfinished tasks (worktrees under ../syeyoung-tokens.worktrees exist only in the cloud container; resume from the pushed branches):**
| Task | Branch head | State | Next |
|---|---|---|---|
| g45-2 ring-qforage iter 2 | d651015 (WIP) | fixing lane/corner/pocket deaths (an ally takes her last exit) and parking; parent ring-qforage-1 | resume; brief briefs/g45-2.md (read the "Correction" section) |
| g45-4 ring-sprint-portalfood iter 2 | d26c20a | iter 2 code committed (doors, gated crossing); not yet verified/submitted. Iter 1 = ring-sprint-13, 55-57 | resume: duel, submit with parent ring-sprint-6, check Trophy and schooltime |
| g45-5 ring-sprint-giantwatch | abd5930 (WIP) | late concentration on an observed enemy giant; not submitted | resume (flip cause C2) |
| g45-7 ring-sprint-qdead2 | a1e228f | enemy-queen death detection (announce before trade), on ring-sprint-16; a duel was running | resume: check its duels, submit with parent ring-sprint-16 (flip cause) |
| g45-8 ring-sprint-guard2 | 604420f (WIP) | port of ring-sprint-17's guard onto ring-sprint-16 | resume: finish, submit with parent ring-sprint-16 (flip cause; guard cut long-worker deaths 15-24% on three bases) |

**Closed this session (target didn't move; details in g44.md/g45.md):** g44-2 portals, g44-4 parked queen (partial → g45-7), g44-5 no bleed, g44-6 pickets, g44-8 relayed sightings, g44-9 S1 hunter fix (57-55 but target unmoved; not promoted), g45-3 stale patrol. Shelved: g45-6 race-time conveyor (never fires in the arena: arena queens are 2-4 at r400). Collected without promotion: ring-sprint-3/10/17 (guard; continued as g45-8).

**Queue after flips-r20:** port ring-qforage (once iter 2 lands) onto ring-sprint-16 as one task; strategist b #2 (champion harvests material) after g45-5; b #4 (ring on beds); a #4 (home food as reserve); g44 strategist #5 (trade by local count).

**Practical notes:**
- Workers' `--wait` printouts have shown wrong numbers (another worker's duel; inflated totals): read verdicts with `tokenz show` / `tokenz duel show`. The games listing can repeat rows: count unique ids.
- Workers share one scratchpad: tell each to use its own subfolder; tell them not to leave wait loops running.
- The arena pool doesn't reproduce some server patterns (bed-waiting circling 8.5% v 13-17%; early-growing enemy queens), so some server-driven changes are inert in the arena.
- Cloud container setup (if another cloud session is used): Tailscale (tagged device; pass `--owner cyoung06@github`), `NO_PROXY=.ts.net` at the top of ~/.bashrc, tokenz from /tokenz.py, a text-only league copy, bcweb credentials, Linux sim fix 2cd97cf; use a non-auto permission mode.
