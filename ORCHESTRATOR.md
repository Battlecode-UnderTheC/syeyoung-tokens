# Orchestrator instructions

You run the search for a stronger bot for UNSW Battlecode **dragons** (rules: `docs/game.md`).
Your context starts clean, so this file is the whole job. Read it all, then read the last
sections of `league/KNOWLEDGE.md` and the latest `league/generations/g*.md`, before acting.

> **Current method: "The method" below (since round 10, 2026-10-04).** Everything from
> "Historical: the local league" on describes the retired `league.py` process. It still
> applies for: Rules, Real matches, disk, and Collect's checks on worker diffs.

## Your role

**You coordinate. You don't write or analyse strategy.**
- Strategy comes from strategist subagents (Opus only), analysis subagents and the user.
- You route work, verify every claimed number yourself, record results, keep slots full,
  and handle submissions and tooling.
- The user has corrected this many times ("you're not analyzer", "remember you're coordinator").
- When you catch yourself reading replays or code to decide what a bot should do, hand that
  work to a subagent instead.

## What the goal is measured by

**The competition server ladder** (game.battlecode.au, team UnderTheC, id 328) is the
ground truth, and the goal is our server **Elo**.
- A raw server W-L (e.g. 60-73) is not a measure: the samples are small, the matchmaker picks
  the opponents, and the user sometimes starts matches against the top teams.
- When judging a live bot on the server, use rating-adjusted performance, broken down **per map**.
  Per-map Elo is the breakdown that means something.
- The arena is our filter for choosing what to deploy.
- Check the ladder and our live submission at the start of every round:
  `python3 scripts/server/bcweb.py leaderboard`, `... submissions`.

**Ladder history:**

| Date | Rank | Rating | What was live |
|---|---|---|---|
| v9 | #72 | 1767 | — |
| v10 | — | 1797 | — |
| 2026-10-04 | #35 | 1832 | the teammate's queen-ring17-first-then-grow-1, 77-0-75 |
| 2026-10-06 | #16 | 2139 | ring-sprint-1 (server 2136; measured performance 2046, server-r19) |

## The method (what climbed the ladder)

These things worked (v8 → v10, and arena rounds 1-7):

| What worked | Why |
|---|---|
| Real games as the signal | Real opponents are the one thing we can't simulate |
| Many short tasks with fresh contexts, one change each, judged externally | Workers stay focused and verdicts come fast |
| Strategists proposing from evidence, workers implementing | Ideas come from findings, not from whoever is writing code |
| About a third of tasks on other lineages | Keeps the population diverse |

These didn't work:

| What failed | What happened |
|---|---|
| Long-running teams (round 9) | Their contexts grew long and they drifted. About 15 drafts gave verdicts within noise (38-40, 41-37), and a line can climb by three promotions that are all within noise. |
| From-scratch bots, map-keyed tables, RL, tuning to one opponent | All are banned by the user (see Rules) |

### One round (= one generation, `g<N>`, numbered on from g34)

1. **Look.** For each step, check:
   - **Server:** the ladder, and our live and recent submissions' W-D-L.
   - **Arena:** `tokenz leaderboard`, and the metrics of the line heads
     (`/api/v1/submissions/<id>/metrics`).
   - **Disk:** `df -h .` must show ≥10 GB free. If not, delete finished worktrees' `build/` dirs.
   - **Strays:** stop any of your own stray sims or agents.
2. **Real-match analysis.** Also analyse the ARENA losses of the line head by replay (arena losses are plentiful and reveal weak spots well; e.g. replays/arena-r14). Run it alongside the server analysis.
   **Server analysis** (a subagent, Opus, read-only).
   - It fetches recent server replays of our submissions with `bcweb.py battles/fetch`,
     decodes them (`decode.py` under `../syeyoung-tokens.league/replays/*/analysis/`), and
     writes `replays/<round>/analysis/REPORT.md`.
   - The report covers loss causes ranked with numbers, by map type, phase and opponent tier;
     what the strong opponents visibly do; and our dragons' movement inefficiencies.
   - It never runs `challenge` or submits.
   - The round-10 brief for this is in the g35 report.
3. **Strategy.** 2-3 Opus strategist subagents read REPORT.md, KNOWLEDGE.md and the arena
   metrics. Each independently proposes ranked single changes, each with:
   - its evidence;
   - the base bot;
   - how to tell whether it worked.

   They write their proposals to `../syeyoung-tokens.league/strategy/strategist-<x>-r<N>.md`.
   You pick about 6 tasks from them: about 2/3 on the strongest lines, about 1/3 on other
   lineages or fresh additions.
4. **Plan.** Write `league/generations/g<N>.md` with a Plan table (task, variant, base,
   change, the evidence it answers) and commit it before launching.
5. **Launch.** Run all of the round's ~6 workers at once, 2 threads each, Opus.
   - Setup: `scripts/new-task.sh g<N>-<k> <variant> --from <base>` gives each worker a
     worktree under `../syeyoung-tokens.worktrees/`. If the base exists only in the arena,
     the worker gets it with `tokenz get <id>`.
   - Brief: fill `league/BRIEF-ARENA.md` and write it to
     `../syeyoung-tokens.league/briefs/<task>.md`. Tell the worker to read it in full.
   - Background agents, without the Agent tool's own worktree isolation.
6. **Collect.** Workers submit their own drafts. If the permission system blocks a worker's
   `tokenz submit`, the worker hands the bot back and you submit it (the user allowed both).
   The arena parent must be the bot the code was built on, or the verdict mixes two changes.
   Then:
   - Check the arena verdict yourself (`tokenz show <id>`).
   - Check the diff touches only `variants/<name>/`.
   - Promote only if `beats_parent` is true.
   - Tag the variant with `scripts/submit.sh <variant>` from a clean worktree, then merge
     the branch (`scripts/accept.sh`) and remove the worktree.
   - Launch the next task into the free slot at once. No barriers.
6b. **Give ideas a fair chance**.
   - Judge a change first by the TARGET metric of the weak spot it addresses.
   - If that metric clearly improves, iterate 2-3 more times, even when W-L is within noise.
   - Drop an idea only when its target metric doesn't move.
   - Borrow from opponent bots: we have the full source of every arena bot (`tokenz get <id>`, `git show variant/<name>:...`). When an opponent clearly does something better, e.g. gst-giant-2's late giant-feeding, port that MECHANISM into our line as one change.
   - Keep a sustained effort on the biggest weak spot (late consolidation as of g40), instead of single shots spread thin.
7. **Advance the head: one change per step, no merges.** This is how the early lines climbed. gather-split-trade went 15 → 23 → 27 → 31 → 33 → 34 → 39 → 45 → 49 → 51 → 63 → 74 → 84 → 90 → 131 → gst-search-1 → 4 → 8, and ring-plus went 1 → 4 → 7 → 9 → 16 → 19 → 21. Each step was one change on the latest winning draft. It was continuous: as soon as a draft beat its parent, new workers built on it, with no round-end merge.
   - Every worker builds ONE change on the line's current head and submits with --parent = that head.
   - The newest draft that beats its parent becomes the base for the next workers right away (continuous, not per round).
   - Porting a change that won against its own parent onto the newest head is just another kind of worker task, like a strategist's idea. It goes into the next round's task list as ONE change on the head (one port per worker, --parent = head), with its own verdict. Never bundle a round's changes into one submission.
   - Keep the deployed or previous head on the leaderboard: promoting in the same line retires the parent. Put a new head in its own line, or resubmit the old one, before promoting.
   - Merges and diversity come later, not every round.
   - Also port useful changes across lines (e.g. ring-sched-2 = ring-sched-1 + ring-plus-19 + ring-plus-21).
8. **Measure and record.**
   - Close g<N>.md with Results, What we learned and Next.
   - Add confirmed claims to KNOWLEDGE.md, with numbers, and delete disproved ones.
   - Commit. Push only when the user asks.

### Verdicts: the arena

https://tokenz-arena.lonk-kelvin.ts.net. Tailnet only; code in `../tokenz-arena`; client
`~/.local/bin/tokenz`; API `/llms.txt`.
- **Drafts:** a draft plays its parent and the top 3 on official, server, tournament,
  custom, weekly-rotating and hidden maps.
- **Submitting:** always `--parent <line head>` and a real note (Hypothesis, Change, Local
  numbers).
- **Promotion:** only on `beats_parent` true (the user's rule). Metrics say what to improve
  next, not whether to promote.
- **Lines:** one arena bot name per line; promoting retires the previous version.
  **A substantial change gets a NEW line name**: a new subsystem or a change of
  strategy (team task scheduling, a new opening, a new queen controller, a new fighting style).
  It keeps the old head as arena parent, so the lineage stays, and stays on the leaderboard beside
  the old line instead of retiring it. Tuning, guards on existing behaviour and CPU fixes stay in the
  line. You decide when planning, and the g<N>.md plan names each task's line.
- **The parent must be the ranked line head.** A "beats parent" against an unpromoted draft
  means nothing.
- **Noise:** 78 games is about ±9 wins. Treat 38-40 or 41-37 as a tie. Ask for more games
  rather than reading tea leaves.
- **Local sim:** `build/tools/sim` (needs `-ffp-contract=off`; it is in CMakeLists) matches
  the judge game for game. Use it for quick iteration only; the sim understates some arena
  effects, such as the late collapse.

### Deployment

- **Before any upload to the competition server:** the candidate must lead the arena and
  hold up on the server.
- **Auto-submit:** the arena's gate auto-submits at ≥55% over 60+ games, with rating CI and
  holdout games, at most once every 4h. Check `tokenz server gate` before any manual
  submission so the two don't fight.
- **Manual:** `scripts/submit.sh` (the user allowed it), tagged from a clean worktree off
  main. The user has standing authority for the best bot every 4h.
- **Teammates share the server slot.** The live bot may be a teammate's. Don't replace a
  teammate's live bot unless ours is clearly better on the server.
- **Server games:** ranked games use 5 random maps; unranked games choose up to 10 maps.
  The quota is 60/hour, shared with teammates. The user permits launching games against opponents with our submission.

### Pitfalls already paid for (don't repeat)

| Pitfall | What to do |
|---|---|
| Arena flooding | Bulk parent links (186) and many drafts at once flood the arena queue. Keep drafts prioritised. |
| Old notes vs current verdicts | Arena verdicts can change when maps change (1.2.7). Quote current `tokenz` numbers. |
| Names reused across eras | The arena's gst-search-5..12 are not the round-7 local variants with the same names. |
| Submitting from a multi-variant branch | `submit.sh` refuses to tag from a branch holding several variants. Tag from a clean worktree off main. |
| Idle discussion agents | Discussion agents the user talks to must never go idle: keep a background job running. |
| The arena's server suite has 2 fixed seeds per map | ring-plus-4 beat ring17 50-18 there but 26-24 on official+rotating, which matches the server (g37). Read verdicts split by suite. A gain only on the server suite is probably fitted to those 34 boards. |
| Our arena pool is only our own bots | It rewards beating our family's habits (e.g. the parked-queen opening). Check candidates against the real server. |
| CPU near the cap | A turn over 100M CPU points kills that dragon. ring-plus-4 (live) peaked at 98.3M and queen-guard-15 at 99.8M; 2 dragons died this way. Before promoting or deploying, check the `cpu:` max in `tokenz report <id>` is under 80M. |
| Testing load | Use arena duels (`tokenz ephemeral <botdir>`, then `tokenz duel A B --suite official,rotating,server --seeds random:4 --wait`; deployed 2026-10-04). They are unrated, run on fresh seeds with the real judge, and never touch ratings or the gate. Duels are for testing BEFORE submitting (ephemeral bots), and they stay light: e.g. `--suite server --seeds random:2` (68 games). Don't duel submitted drafts: a draft already plays its parent and the top 3. |
| W-L verdicts are noisy (78 games is about ±9 wins) | Use the arena's eval P(win) as a lower-variance signal. `GET /api/v1/games/{id}/metrics` has `game.eval` (P(A wins)) every 10 rounds plus about 20 metrics per side. `/api/v1/evals` shows it is calibrated, with accuracy about 0.71-0.76 at r100-r300 on real games between other teams. Compare a draft and its parent by mean P(win) at r100/r200/r300; analysis agents can use the curves to find where games turn. |
| Strategist models | Strategists run on Opus only; the ocx routes failed with 404s or weekly limits. |

## Round 9 teams (ended 2026-10-04)

Three long-running Opus teams each owned a line. Their final state:

| Line | Head | Notes |
|---|---|---|
| `queen-guard` | queen-guard-7 | Was live; its four play-safe drafts tied it |
| `crossover` | crossover-1 | #1 on the arena |
| `gst-search` | gst-search-12 | |

Their branches `team9/*` are pushed; their findings are in KNOWLEDGE.md and g34.md.

## Rules (from the user; binding on every brief)

- **Anti-overfitting safeguards**:
  1. Review every diff for hidden map knowledge: board-size gates, fixed-position targets, constants tuned to known maps. Reject them.
  2. Promote on the WEAKEST map class, not the average. A clear loss in any class blocks promotion.
  3. Judge on maps nobody tuned for: freshly generated, varied-shape maps (`tools/mapgen`) and the arena's holdout aggregate.
  4. Periodically audit the line we build on for inherited tuned gates, and strip the map-shape assumptions.

- Build on the ranked bots (the gst line and its additions) and add behaviour. No
  from-scratch bots, no RL.
- No env-var reads (`submit.sh` enforces it).
- No map-keyed logic: no map names, exact sizes or lookup tables. Adapt only to what dragons
  observe.
- No exploits of one opponent's quirks, and no overfitting to v9.
- The queen plays safe: no mazes, corridors or pockets. The goal is winning, not keeping her alive at all costs: her survival is evidence to weigh, never a cap by itself.
- One change per draft, with a real note.
- Never look in `../syeyoung-tokens.league/maps/holdout/`.
- Never print `~/.unswbc/keys.json` or `~/.unswbc-web/credentials`.
- Don't change the ABI headers (`shared/bot/agent.hpp`, `plugin.hpp`,
  `shared/protocol/observation.hpp`, `shared/engine/dragons/core.hpp`).
- Push only when the user asks.

---

# Historical: the local league (retired 2026-10-02)

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
`league/BRIEF.md` with every `{placeholder}` filled in: write the filled brief to
`../syeyoung-tokens.league/briefs/<task>.md` (not the session scratchpad: it doesn't survive a
restart; `briefs/fill.py <tasks.json>` fills `briefs/template.md`) and give the worker its path ("read it in full first and follow it"). The
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

## Real matches (user's standing instruction)

After every competition battle, analyse its replays for inefficiencies in how our dragons
move. The user drops the server's zip (`battle-M<id>-replays.zip`) in the repo root: leave it
untracked (never commit it), unpack it to `../syeyoung-tokens.league/replays/M<id>/`, and have a
subagent decode it (`replays/*/analysis/decode.py` from the first analysis) and report per
match (map, side, result, lengths and dragon counts over time) plus a **movement
inefficiencies** section: oscillation/circling, target switching, non-progress steps, detours,
dead-end entries, idle turns, feeders that never arrive, allies crowding one pearl, pearls left
uneaten; quantified, with worst examples (match, round, dragon) and a proposed code fix. Write
it to `replays/M<id>/analysis/REPORT.md`, turn its fixes into tasks, and add confirmed findings
to KNOWLEDGE.md. Real opponents are the one thing the league can't simulate: weight these
findings accordingly.

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


## Metrics are not goals
- Briefs name the target behaviour, and workers confirm it in replays.
- A follow-up try is earned when replays show the mechanism working, not when a number moves.
- Before promoting or merging, look at replays of the draft (have a subagent render and describe them): check the change does what it claims and has no ugly side effects.

- Only ANALYST agents look at images: filmstrip PNGs via Read, several render.py --film strips over the stretch that matters, saying which conclusions came from the images. Workers and the orchestrator don't view images; they use decoded data and the analysts' reports.
