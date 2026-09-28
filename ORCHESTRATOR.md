# Orchestrator instructions

You are the orchestrator of an evolutionary search over strategies for the unswbc
**dragons** game. You don't write strategies yourself. You run generations: read the
league's standings, decide what to breed, brief worker subagents who each write one new
variant, merge their work, wait for the league to rate it, record what was learned, and
repeat. Your context starts clean, so this file is the whole job. Read it all before acting.

Your job is to raise the **holdout champion's** strength, the best variant as measured on
maps no agent sees, and to leave behind a record (generation reports, `KNOWLEDGE.md`)
that a later session can pick up from.

## The setup (all on this Mac: M1 Pro, 10 cores, 32 GB, disk nearly full)

| Path | What |
|---|---|
| `~/Documents/postech/unswbattlecode/syeyoung-tokens/` | this repo, branch `main`. You work here. |
| `../syeyoung-tokens.worktrees/<task>/` | one git worktree per worker task, branch `task/<task>` |
| `../syeyoung-tokens.league/` | the league's data (not git): `league.db`, `reports/`, `plugins/`, `maps/`, `logs/serve.log` |
| `docs/game.md` | the rules. Read it once. |
| `DESIGN.md`, `README.md` | the plan and the repo layout |
| `league/BRIEF.md` | the worker brief template you fill in |
| `league/ARCHETYPES.md` | strategy ideas for fresh variants |
| `league/KNOWLEDGE.md` | what is known, with evidence. You curate it. |
| `league/generations/g<N>.md` | one report per generation. You write them. |

**The league service** (`league/league.py serve`) runs in the background. It picks up
every new `variant/<name>` tag within a minute, builds it frozen from the tag, gates it
(the `check` tool, then ≥75% wins against anchor-random), and plays it against the others
on public maps (11 official) and rotating maps (20 worldgen maps, changed weekly), both
sides. It rates by Bradley–Terry (Elo, anchor-random = 1000), plays the top 4 on a
**hidden holdout** (100 maps), and rewrites `reports/` after every batch. Commands (run
from the repo):

```sh
league/league.py status              # leaderboard (Elo, ±, games, holdout W-D-L)
league/league.py report <variant>    # by opponent, by map (worst first), deaths, reasons, fatal turns, errors
league/league.py pending             # "tags waiting: 0 / matches owed: 0" = everything rated
league/league.py quick <lib.so> [--vs V]   # an untagged build vs league variants; records nothing
league/league.py trace <A> <B> <map file>  # one match, round by round
scripts/variants.sh                  # every tagged variant with its parents and operator
```

Check it is running before anything else: `pgrep -fl "league.py serve"`. If it isn't:
`nohup nice -n 10 python3 league/league.py serve >> ../syeyoung-tokens.league/logs/serve.log 2>&1 &`.
If you change `league.py`, restart it. Look at `logs/serve.log` if ratings stop moving.

**Variants** live in `variants/<name>/`: C++ sources implementing `bot::Agent`
(`shared/bot/agent.hpp`), one `LEAGUE_VARIANT(Class)` line, `variant.toml` (name, parents,
operator, archetype, author, created, task) and `NOTES.md`. A tagged variant never changes;
an improvement is a new variant with the old one as a parent. At the start the league holds
`strategy-v1` (Elo ≈1740, the hand-written baseline, also our current competition
submission), `anchor-random` and `anchor-greedy`.

## The loop: short and continuous

The user wants a **very short loop**: rapid prototype, rapid arena, rapid fix. So:

- **Tasks are small:** one idea or one fix per task, about **20 minutes** of worker time. The
  worker checks it is sound (`check`, a quick `sim` or `league quick` against its parent),
  submits, and reports. The league rates a new tag in about 3 minutes (it polls once a
  minute; a batch of 120 matches takes ~20 s), so the league does the thorough measuring.
- **Slots stay full:** keep **6 workers running**, each at most 2 threads (builds `-j 2`,
  `sim --threads 2`). Workers mostly write code, so CPU is not the bottleneck.
- **No barriers:** when a worker reports, collect it (step 4), and as soon as its tag is
  rated, read the fresh standings and launch the next task into the free slot (steps 1–3 for
  that one task). A fix for a child's measured weakness can go out minutes after it is rated.
- **A generation is a bookkeeping unit:** every 6 launched tasks form `g<N>`. Add each task to
  `g<N>.md`'s Plan table as you launch it (commit), and close the report (steps 5–6) once its
  6 tasks are rated. Update `KNOWLEDGE.md` whenever a result is clear, not only then.

Run until told to stop or until the stopping rule below. Steps 1–6 describe one task's life
and a generation's report.

### 1. Look

- Wait until the tag you are building on is rated (`league/league.py pending`; ~3 minutes).
  Other tags still pending don't block you.
- Read `status`, the reports of the top ~4 variants and of last generation's children,
  `league/KNOWLEDGE.md`, and the last `league/generations/g*.md`.
- Also check `df -h .`: below 10 GB free, delete finished worktrees' `build/` dirs first.

### 2. Plan (write it down before launching anything)

Choose each task's operator and parents:

- **mutate** (~50%): one strong parent, fix its biggest measured weakness. The report
  says where: its worst maps, the deaths it suffers most, the reasons behind fatal turns,
  the opponents it loses to. Name the weakness in the brief.
- **crossover** (~20%): two strong parents with different styles (different archetypes,
  or different death/reason profiles). Combine what each does well.
- **fresh** (~20%): an archetype from `ARCHETYPES.md` nobody has tried or that failed for
  a fixable reason. This keeps the population diverse: the holdout rewards generality, and
  one lineage can hit a ceiling.
- **tweak** (~10%): a parameter change to a parent, as its own variant. Cheapest; use it
  to test one claim from `KNOWLEDGE.md`.

Selection: favour the top by Elo and holdout, but keep at least one parent per generation
from outside the top lineage when there is a candidate with a distinct style. Never breed
from a variant with errors in its report.

Names: `<archetype>-<n>` (for example `swarm-trade-3`), unique. Check
`scripts/variants.sh` and `git tag -l 'variant/*'`. Task ids: `g<N>-<k>` (for example
`g3-2`).

Write `league/generations/g<N>.md` with a **Plan** section (the table: task, variant,
operator, parents, the brief's focus, and why), and commit it on main before launching.

### 3. Launch

For each task, from the main checkout:

```sh
scripts/new-task.sh g<N>-<k> <variant> [--from <parent> [--from <second parent>]]
```

This makes the worktree `../syeyoung-tokens.worktrees/g<N>-<k>/` on branch `task/g<N>-<k>`,
copies the first parent into `variants/<variant>/` (fresh: an empty directory), and writes
`variant.toml` and a NOTES.md skeleton. Fill in `variant.toml`'s `archetype` and `author`
(the worker's model) in the worktree before launching.

Then spawn a worker subagent with the Agent tool: `general-purpose`, **in the background**,
without the tool's own worktree isolation (the task already has its worktree). Its prompt is
`league/BRIEF.md` with every `{placeholder}` filled in: write the filled brief to a file in
your scratchpad and give the worker its path ("read it in full first and follow it"). The
worker starts with no context and must not depend on this conversation. Use the strongest model
for fresh and crossover tasks; a cheaper one is fine for tweaks.

Launch into any free slot at once; keep 6 running.

### 4. Collect

When a worker reports back:

- Check the result yourself; don't trust the summary. `git tag -l variant/<name>` shows
  whether it submitted. `git -C <worktree> log --oneline main..` shows its commits.
  `git diff main...task/<task> --stat` must touch only `variants/<name>/`.
- Workers don't tag (the permission classifier blocks `submit.sh` in their worktrees). When a
  worker reports its variant ready and the diff is clean, run `scripts/submit.sh <variant>`
  in its worktree yourself (the user allowed it: `Bash(scripts/submit.sh:*)`).
- If submitted: `scripts/accept.sh g<N>-<k>` (merges the branch into main; the branch is
  kept). Then remove the worktree (`git worktree remove ../syeyoung-tokens.worktrees/g<N>-<k>`).
- If it didn't submit (it ran out, or its variant lost to its parent): record what it
  tried and why it failed; that is knowledge too. Remove the worktree; keep the branch.
- Note each worker's outcome and the token use the Agent tool reports.

### 5. Measure

Wait until `league/league.py pending` shows 0 tags and 0 matches, then read `status` and
every new variant's report. For each child, compare it against its parents, not just the
table: `sqlite3 ../syeyoung-tokens.league/league.db` has every match (see the `matches`
columns in `league.py`). Head-to-head W-D-L between a child and its parent is the cleanest
signal. A child with Elo within its ± of the parent's didn't improve.

Rejected variants show why in their report (`## Why it is not active`).

### 6. Record (commit on main)

Finish `league/generations/g<N>.md`:

- **Results:** per child: operator, parents, status, Elo ± (and the parents'), head-to-head
  against each parent, holdout if played, and one line on what changed.
- **What we learned:** which changes helped or hurt, with the evidence (Elo, head-to-head,
  the report sections).
- **Next:** what the next generation should try.

Then update `league/KNOWLEDGE.md`: add a claim when a result supports one, attach the
evidence (variants, numbers, generation), mark a claim confirmed or contradicted when a
generation tests it again, and delete what is disproved. Keep it short enough to read in
five minutes; workers read it before they start.

Commit both files: `git add league/generations league/KNOWLEDGE.md && git commit`.

### Stopping

Stop when the user's budget is spent, when told to, or when the holdout champion (the best
of the top 4 on the holdout) hasn't changed in **3 generations**. Then write a summary for
the user: the champion, how it compares to `strategy-v1` (head-to-head and holdout), what
worked, and what to try next.

## Rules

- **The league is the only judge.** A worker's own numbers are hints. Decisions use league
  results.
- **Never look at the holdout**, and never tell a worker about it:
  `../syeyoung-tokens.league/maps/holdout/`, and `matches` rows with `suite='holdout'` apart
  from the aggregate W-D-L the reports show. Tuning to it would make it worthless.
- **Workers only write `variants/<their variant>/`** in their own worktree. `shared/`,
  `tools/`, `league/`, `scripts/`, `maps/` and the ABI headers are yours to change, and only
  when needed. Changing `shared/bot/agent.hpp`, `plugin.hpp`, `shared/protocol/observation.hpp`
  or `shared/engine/dragons/core.hpp` makes every stored variant unplayable; don't.
- **Configuration is code.** No command-line tuning knobs; every setting that matters is in a
  committed file, and every result is in the database or a committed report.
- **Nothing outward-facing without the user.** Don't push, and never submit to the
  competition server (`unswbc submit`) on your own. When a champion clearly beats
  `strategy-v1` (head-to-head and on the holdout), *propose* a submission to the user. To
  prepare one: `scripts/make-bot.sh <variant> <dir>`, then check it in the judge's sandbox
  (CPU under 100M points a turn; ask the user how to run `unswbc`). The team's API key is
  in `~/.unswbc/keys.json`; never print or copy it.
- **Disk:** keep over 10 GB free. Finished worktrees go; their branches stay.
- If a worker goes silent for a long time, check its worktree for progress before deciding
  it is stuck. Don't start a replacement for the same task while one is still running.
