# syeyoung-tokens

**New here (teammate, Codex, or any agent)?** Start with [`docs/ONBOARDING.md`](docs/ONBOARDING.md).

Strategies for the unswbc dragons game, written by LLM agents, measured in a league,
and bred by selection. See `DESIGN.md` for the whole plan; this file is the practical
part: the layout and the git workflow every agent follows.

## Layout

| Path | What | Who writes it |
|---|---|---|
| `shared/engine/dragons/` | The game engine (a port of the official one, lockstep-tested). | humans only |
| `shared/protocol/` | Parses the judge's text into an `Observation`. | humans only |
| `shared/bot/` | The `Agent` interface, the plugin entry point, the stdin/stdout protocol loop. | humans only |
| `shared/lib/` | Helpers any variant may use: `BoardMemory` (what a dragon knows of the map), `pathing` (search, dead ends). | humans only |
| `variants/<name>/` | One strategy: its source, `variant.toml`, `NOTES.md`. | one agent task each |
| `tools/` | `sim` (many matches, a summary), `match` (one match as JSON), `check` (the gate); `play.*` is what they share. | humans only |
| `league/` | The league service and CLI (`league/league.py`; DESIGN.md section 2). | humans only |
| `scripts/` | The git workflow (below) and building a variant as an unswbc bot. | humans only |
| `maps/official/` | The 11 official maps. More suites live outside git (see DESIGN.md). | — |
| `maps/worldgen/` | A procedural map generator (vendored, see `SOURCE.md`): unlimited practice maps. | — |

`shared/VENDORED.md` records where the shared code came from.

## A variant

```
variants/<name>/
  *.hpp, *.cpp     the strategy; one file defines LEAGUE_VARIANT(ClassName) (shared/bot/plugin.hpp)
  variant.toml     name, parents, operator, archetype, author, created
  NOTES.md         the idea, what changed from the parents and why, what should improve
```

It builds two ways from the same source: a plugin (`build/variants/lib<name>.so`) the
simulator loads, and an unswbc bot (`scripts/make-bot.sh <name> <dir>`) for the server.
A variant may copy and change anything from `shared/` into its own directory; it never
edits `shared/` in place.

## Git workflow

- **`main`** holds the shared layer, the tools, and every accepted variant.
- **One task, one branch, one worktree.** Every agent task works on its own branch
  `task/<task-id>` in its own worktree under `../syeyoung-tokens.worktrees/<task-id>`,
  so parallel agents never touch the same checkout.
- **Accepted variants are tagged.** When a task submits, its variant gets an annotated tag
  `variant/<name>` on the commit that holds it. A tag never moves: a changed strategy is a
  new variant with a new name (`<archetype>-<n>`), and the old one as parent.
- **Branches are kept** after merging, so every task's history stays reachable.

```sh
scripts/new-task.sh <task-id> [--from <variant>]   # branch + worktree; optionally copy a parent variant to start from
# ... the agent works in ../syeyoung-tokens.worktrees/<task-id>, committing as it goes ...
scripts/submit.sh <variant-name>                   # (in the worktree) checks, commit, tag variant/<name>
scripts/accept.sh <task-id>                        # (in main) merge the task branch into main
scripts/variants.sh                                # every tagged variant: name, parents, commit
```

## Building and playing

```sh
cmake -S . -B build && cmake --build build -j
build/tools/sim --a strategy-v1 --b strategy-v1 --maps maps/official
build/tools/check strategy-v1 maps/official        # memory audit, no avoidable wall/self deaths
```

## The league

```sh
nohup nice -n 10 league/league.py serve >> ../syeyoung-tokens.league/logs/serve.log 2>&1 &   # the service
league/league.py status                        # leaderboard
league/league.py report <variant>              # what to improve
league/league.py quick build/variants/lib<name>.so --vs strategy-v1   # a draft, before submitting
league/league.py trace <A> <B> <map file>
python3 maps/worldgen/worldgen.py --batch 20 --outdir /tmp/practice   # practice maps
```

Tag a variant (`scripts/submit.sh`) and the service builds, gates and plays it within a minute.
