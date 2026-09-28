# League: evolving strategies with LLM agents

The plan for throwing many LLM agent sessions at the dragons game: agents write
strategy variants, a league measures them, selection decides who breeds, and the
best goes to the competition server. It is infrastructure meant to outlive any one
session. Nothing here depends on RL (abandoned 2026-09-28).

## Principles

- **The league is the only judge.** An agent never reports its own score; the league
  measures every variant the same way, on maps agents can't tune against.
- **Nothing lives only in a terminal.** A configuration is committed code; every match
  is a database row keyed by variant and commit; every generation has a committed
  report. There are no command-line tuning knobs.
- **Variants are immutable.** Once submitted, a variant never changes; an improvement is
  a new variant with the old one as parent. Lineage is always recoverable.
- **Agents are cheap and replaceable, knowledge is not.** What agents learn goes into a
  curated knowledge file, not only into their code.

## Components

Everything runs on one machine: this Mac (M1 Pro, 10 cores, 32 GB). The repository
is local; git tags and branches are the interface between agents and the league.

```
  ┌──────────────────────────────────── this Mac ─────────────────────────────────────┐
  │                                                                                    │
  │  orchestrator ── briefs ──► agent workers (Claude Code, headless, one worktree     │
  │  (scheduler)                and task/<id> branch each; tag variant/<name>)        │
  │       ▲                              │ tags                                        │
  │       │ standings                    ▼                                             │
  │  league service: ingest tags → gates → matches → ratings → reports (SQLite)        │
  │       ▲                                                                            │
  │       └──── league CLI ◄── agents query their standing, run quick matches          │
  └────────────────────────────────────────────────────────────────────────────────────┘
        champion ──► unswbc submit ──► competition server ──► results, replays ──► league
```

### Running on this Mac

- **CPU:** the league's matches use at most 6 threads, leaving cores for the agents'
  builds and `league quick` runs, and for the person at the machine. The native engine
  plays about one full match per second per thread, so a generation (~8 new variants
  × ~60 maps × both sides against a handful of opponents) takes on the order of an hour.
  The orchestrator runs ~2-4 agents at once (most of an agent's time is thinking, not CPU).
- **Disk:** the disk is 96% full (38 GB free on 2026-09-28). Each worktree is small
  (~1 MB of source plus a build), but builds, replays and logs add up: the league deletes
  a task's build directory when the task ends, keeps `.replay` files only for sampled
  losses of the current top variants, and warns below 10 GB free.
- **Processes:** the league service and the orchestrator run as ordinary background
  processes (or launchd agents), started by hand; nothing needs a server.

### 1. Variant contract (`variants/<name>/`)

| File | Content |
|---|---|
| `*.hpp, *.cpp` | The strategy: a `bot::Agent`, with one `LEAGUE_VARIANT(Class)` line (`shared/bot/plugin.hpp`) exporting `league_make_agent`. Builds from its own directory plus `shared/`. |
| `variant.toml` | `name`, `parents` (0-2 names), `operator` (fresh / mutate / crossover / tweak), `archetype`, `author` (model, session id), `created`, `task`. |
| `NOTES.md` | The idea, what changed from the parents and why, what the author expects to improve. Written before evaluation. |

- **Identity:** the annotated git tag `variant/<name>` on the commit that holds it. Tags
  never move; `scripts/submit.sh` refuses an existing name.
- **Shared layer (read-only for agents):** `shared/` (engine, protocol parser, the
  `Agent` interface and loop, `BoardMemory`, `pathing`). Agents may copy and modify any
  of it inside their own variant, never in place (`submit.sh` refuses changes outside the
  variant directory). Shared-layer changes are made by a human.
- **Two builds from one source:** a plugin (`build/variants/lib<name>.so`, hidden
  symbols, loaded by `sim`, `check` and the league) and an unswbc bot
  (`scripts/make-bot.sh`, flat directory, the judge's flags).

### 2. League service (a background process on this Mac)

A long-running process; everything it learns goes into `league/league.sqlite` (copied
aside daily) and committed reports.

- **Ingest:** polls the repository for new `variant/*` tags; builds each new variant.
- **Gates**, in order; a failure is recorded with its log and the variant is rejected:
  1. builds as a plugin and as an unswbc bot with the judge's flags; zip under 4 MB;
  2. safety: memory audit and "no wall or self death with a safe move" (today's
     `strategy_tests`), no crash in 20 matches, no turn over a CPU cap (native
     instruction count, calibrated against the wasm meter);
  3. sanity: beats random and greedy on the official maps.
  Champion candidates also pass the judge's sandbox (`unswbc run --sandbox`, p99 CPU
  under 50% of the 100M points per turn).
- **Matches:** every match runs in a forked child process with a wall-clock and memory
  limit, so a crashing or looping variant can't hurt the league. Pairing is continuous:
  new variants first play the anchors, then opponents chosen by rating uncertainty
  (TrueSkill), both sides of each map.
- **Maps**, three suites:
  - *public:* the 11 official maps. Reports show per-map results.
  - *rotating:* ~200 pool maps, a new slice each generation. Reports show aggregates.
  - *holdout:* ~100 maps agents never see results for. Used for selection and champion
    decisions. Replaced if they leak.
  Structured generated maps (rooms, corridors, pearl fields) are added so the pool
  resembles the official ones.
- **Anchors** (fixed, always playing): strategy-v1 (server v3), the RL policy at update
  50753 (as a plugin via `policy_agent`), greedy, random, and every former champion.
  They keep ratings comparable over time.
- **Match row:** variant ids, their commits, map id and suite, side, engine commit, result,
  rounds, final longest and total per side, deaths by cause per side, reason tallies,
  CPU per turn (p50, p99, max), wall time.
- **Ratings:** TrueSkill per variant, on holdout plus rotating maps; also win rate against
  each anchor, mean length margins, and behaviour descriptors (below).
- **Reports** (`league/reports/<id>.md`, regenerated as matches arrive): standings, results
  by map (public) and by opponent, deaths by cause, reason tallies, the reasons behind
  fatal turns, and 2-3 traced losses (`--trace` output) with official `.replay` files
  (recorded commands replayed through the unswbc engine).
- **Generation reports** (`league/generations/<n>.md`, committed): what was bred from
  what, results, what worked, the new leaderboard.

### 3. `league` CLI (for agents and humans)

The only way agents touch the league. Every call's results are recorded.

| Command | Does |
|---|---|
| `league status` | Leaderboard, current generation, gate queue. |
| `league report <id>` | A variant's report (as above). |
| `league quick <dir> [--vs <id>...]` | Builds a work-in-progress variant and plays it against given opponents on public + rotating maps (never holdout). Recorded as a "draft" result, so an agent can iterate within its budget without polluting ratings. |
| `league trace <dir> <map> --vs <id>` | One verbose match (the simulator's `--trace`). |
| `league check <dir>` | Runs the gates locally. |
| `league submit <dir>` | Freezes the variant, pushes `variant/<id>`, enters it in the league. |

### 4. Orchestrator (scheduler)

Runs on a machine holding the Claude credentials (a cron job or a small service);
generation-based, with a hard token budget.

- **Selection.** Keeps an elite set (top ~4 by holdout rating) and a MAP-Elites archive
  over behaviour descriptors, so different styles survive even when one leads:
  average dragon count × head-to-head trade rate, bucketed. Parents come from tournament
  selection over elites and archive cells.
- **Brood per generation** (~8 children):
  - ~50% *mutate:* one parent plus its report: fix its biggest measured weakness.
  - ~20% *crossover:* two strong parents from different cells.
  - ~20% *fresh:* an archetype from `league/ARCHETYPES.md` (swarm-and-trade, one giant
    turtle, territory walls, sonar-coordinated packs, pearl-farm camping, portal
    ambushes, ...), with the knowledge file.
  - ~10% *tweak:* parameter changes as a variant, the cheapest operator.
- **Launching workers:** headless Claude Code (`claude -p` with the brief) or the
  Agent SDK, on this Mac, each in its own git worktree on a fresh branch
  (`scripts/new-task.sh`), with allowed tools limited
  to the repo, the league CLI and the build. Concurrency ~4-8. Per-task caps on turns,
  tokens and wall time. Model tiers: a cheaper model for tweak and mutate, the strongest
  for crossover and fresh ideas.
- **Bookkeeping:** every task's brief, transcript id, token use, outcome (submitted /
  rejected at gate n / timed out) and resulting variant go into the database.
- **Stopping:** stops a run on budget, or when the holdout champion hasn't improved for
  N generations. Next run resumes from the database.

### 5. Agent brief (template, `league/BRIEF.md`)

Every worker gets the same frame, plus its task:

- the goal (beat the league, measured on maps it can't see), the variant contract and
  the rules (only write your variant directory; never edit the shared layer; `league
  check` must pass; write NOTES.md before submitting);
- reading list: the game rules (`dragons-env/game.md`), `strategy/README.md`,
  `league/KNOWLEDGE.md`, the parents' source and reports;
- tools: the league CLI (`quick`, `trace`, `check`, `submit`);
- its budget, and to submit what it has rather than run out.

### 6. Knowledge (`league/KNOWLEDGE.md`)

Curated after each generation by a librarian agent (a cheap model reading the
generation report). It holds findings with evidence: a claim, the match ids behind it,
and when it was last confirmed. Examples from `strategy/EXPERIMENTS.md`: numbers win the
one-for-one attrition on crowded maps; even trades on a guess of outnumbering lose;
fleeing loses. Stale claims get re-tested or removed. Agents read it so they don't
rediscover the same things.

### 7. Ground truth: the competition server

- Every few generations, or when the holdout champion clearly beats the last submission:
  build the champion as an unswbc bot and submit it (`unswbc submit`, team key).
  Submitting is outward-facing, so the orchestrator proposes and a human approves.
- Fetch the server's match results and replays through its API and store them. A large
  gap between league rating and server results means the league lacks the right
  opponents: study the losses and add counter-archetypes (as fresh-idea tasks, or as
  scripted anchors mimicking what beat us).

### 8. Engine parity

The league trusts our engine port. Keep it honest in CI: the lockstep test against the
official wasm engine (`dragons-env/tests/lockstep.py`) runs on every engine change and
nightly on random maps. When a new unswbc version ships (1.1.0 is out; we use 1.0.0),
re-run parity before trusting results.

## Build order

1. **Plugin contract:** `league_make_agent`, the `.so` build, strategy-v1 as the first
   variant, the simulator loading plugins. Also the RL policy and greedy/random as plugins
   (anchors).
2. **League service:** database schema, ingest, gates, forked matches, TrueSkill,
   map suites (with holdout), reports. Run it with strategy-v1 and the anchors only.
3. **`league` CLI:** status, report, quick, trace, check, submit.
4. **Orchestrator:** selection, briefs, worker launching, bookkeeping, budgets.
   `BRIEF.md`, `ARCHETYPES.md`, empty `KNOWLEDGE.md` seeded from `EXPERIMENTS.md`.
5. **Generation 0:** strategy-v1 plus ~6 fresh archetypes. Check that the loop runs end to
   end, then scale up the concurrency.
6. Server loop: submission proposals, result and replay fetching, parity CI.

## Open decisions

- Where the orchestrator runs, and with which credentials (it spends tokens unattended).
- Token budget per generation and in total; model tiers per operator.
- Whether the orchestrator may submit to the competition server without approval.
