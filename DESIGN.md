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

### 2. League service (built: `league/league.py serve`)

A background process on this Mac (`nohup nice -n 10 league/league.py serve`). Standard-library
Python; configuration is the constants at the top of `league.py`, in git. It owns
`../syeyoung-tokens.league/` (never in git):

| Path | What |
|---|---|
| `league.db` | SQLite: `variants` (commit, ABI, status, why rejected, variant.toml, Elo), `matches` (one row per match: players, suite, map, winner, the match tool's JSON, error, tools commit) |
| `tools/` | `match`, `check`, `sim`, rebuilt from `main` whenever main moves |
| `plugins/lib<name>.so` | each variant, built once from its tag's own tree: frozen |
| `maps/public/` | the official maps |
| `maps/rotating/e<N>/` | 20 worldgen maps (seeds N*1000..), a new set every 7 days |
| `maps/holdout/` | 60 pool maps + 40 worldgen maps from a secret seed range; agents never see these or per-map results |
| `maps/practice/` | worldgen seeds 900000+, for `league quick` |
| `maps/pool_maps.txt` | the RL generator's 4000-map pool, the holdout's source |
| `reports/` | `LEADERBOARD.md` and `<variant>.md`, rewritten after every batch |
| `logs/serve.log` | what the service did |

Each pass:
- **Ingest:** new `variant/*` tags. A variant whose Agent ABI (`ABI_FILES`: agent.hpp,
  plugin.hpp, observation.hpp, core.hpp) differs from main's is rejected: the tools could
  not call it. So those four files are frozen; changing them means rebuilding the league.
- **Gates:** `check` on the public maps (no crash, no avoidable wall/self death, board
  memory right), then at least 75% wins against anchor-random on public maps, both sides.
- **Matches:** one `match` process each (180 s timeout; a crash or hang is an error row,
  not a result), 6 at once, batches of 120. Up to 24 variants everyone plays everyone;
  past that, rating neighbours plus the anchors and the leader. Every pair plays every
  public and rotating map from both sides. When nothing is owed, the top 4 play each
  other and anchor-greedy on the holdout.
- **Ratings:** Bradley–Terry (MM iteration, draws half a win, a weak prior) on public +
  rotating results, in Elo with anchor-random fixed at 1000; ± from the Fisher information.
  The holdout never feeds the rating: it's for selection and spotting overfitting.
- **Anchors:** anchor-random, anchor-greedy (fixed, `variants/anchor-*`), and strategy-v1.
  The RL policy is not an anchor (RL was abandoned; its plugin would need the weights).

Not built yet: the judge-sandbox gate for champion candidates (CPU points, zip size),
traced losses and `.replay` files in reports, generation reports.

### 3. `league` CLI (built)

| Command | Does |
|---|---|
| `league status` | The leaderboard, match count, errors. |
| `league report <variant>` | A variant's report: W-D-L by opponent and side, by map (worst first), deaths by cause, reason tallies and the reasons behind fatal turns, turn times, errors, holdout aggregate. |
| `league quick <lib.so> [--vs V]... [--maps N]` | A work-in-progress build against league variants (default the top 3) on public + practice maps. Records nothing. |
| `league trace <A> <B> <map file>` | One match round by round (`sim --trace`). |
| `league rate` | Recompute ratings and reports from the database alone. |

Submitting stays `scripts/submit.sh` (check, commit, tag); the service picks the tag up.

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

Done: 1-3 (2026-09-28). Next: 4.

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
