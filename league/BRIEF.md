# Worker brief (template)

The orchestrator fills every `{placeholder}` and passes everything below the line, in full,
as the worker's prompt.

---

You are writing one strategy for the unswbc **dragons** game: the variant
**`{variant}`**, task `{task}`. Your work is measured by a league that plays it against
every other variant on maps you can't see, so play well in general; don't tune for one map.

## Your task

- Operator: **{operator}**. Parents: {parents}.
- Focus: {focus}
- Budget: about {budget} (check `date` at the start). This is a short loop: make the one
  change the focus asks for, check it is sound (`check`, then one `sim` on the official maps
  and one on ~20 worldgen maps against your parent), submit, and report. Don't sweep
  parameters or polish: the league rates your tag within minutes, and the next task will
  build on what it measures.

## Where you work

- Your worktree: `{worktree}` (branch `task/{task}`). Run every command there, with
  absolute paths or `cd {worktree} && ...`. Never touch the main checkout or another task's
  worktree.
- You write **only** `variants/{variant}/`. It already holds {start}, `variant.toml` and a
  `NOTES.md` skeleton. Everything else (`shared/`, `tools/`, `league/`, `scripts/`, `maps/`)
  is read-only. To change a helper from `shared/lib/`, copy it into your directory **under a
  new file name and namespace** (the same names would clash with the shared copies when
  linking).
- Commit your work on your branch as you go (`git add variants/{variant} && git commit`).

## Read first

1. `docs/game.md`: the rules. Precise; everything depends on them.
2. `league/KNOWLEDGE.md`: what earlier work established. Don't rediscover it; do challenge
   it if your evidence disagrees.
3. `shared/bot/agent.hpp`, `shared/bot/plugin.hpp`, `shared/protocol/observation.hpp`: the
   interface. One `Agent` object per dragon, created at spawn; `Turn()` gets that dragon's
   observation and returns its action; `reason()` names why (a few words; the league tallies
   them, and they're how you'll see what your dragons do). Dragons of a team share no memory
   except through sonar; each agent object keeps its own.
4. `shared/lib/`: `board_memory` (a dragon's map of what it has seen: tiles, edges, portal
   pairs, bed countdowns), `pathing` (search and dead-end room), `window.hpp` (the 7×7 view
   as a grid).
5. Your parents' code in `variants/` and their reports: `python3 league/league.py report <parent>`.
   Also `variants/strategy-v1/NOTES.md` for the baseline's design.

## Build, measure, look

```sh
cd {worktree}
cmake -S . -B build >/dev/null && cmake --build build -j 2 --target {variant} sim check
build/tools/check {variant} maps/official          # must say "all checks passed"
build/tools/sim --a {variant} --b <parent> --maps maps/official --threads 2
python3 league/league.py quick build/variants/lib{variant}.so --vs <opponent> --vs <opponent>
build/tools/sim --a {variant} --b <parent> --maps maps/official --trace <map name>   # one match, verbosely
python3 maps/worldgen/worldgen.py --batch 20 --outdir {worktree}/build/practice           # more maps
# --seed S --batch N makes seeds S..S+N-1: for a second, independent set use a seed at least N away
# (e.g. --seed 1000 then --seed 2000); seeds 1 and 2 share 19 of 20 maps.
build/tools/sim --a {variant} --b <parent> --maps {worktree}/build/practice --threads 2
```

`sim` prints W-D-L, final length margins, deaths by cause, and each side's action reasons
and the reasons behind fatal turns. Use `--trace` on a loss to see what went wrong, round by
round. Measure on the official maps **and** fresh worldgen maps; a change that only helps on
one set is suspect. Keep at most 2 threads busy (builds `-j 2`, `sim --threads 2`): other agents and the league share the machine.

Limits your variant must respect (the competition judge):

- It runs as WebAssembly with **100M CPU points per turn** and **48 MB of memory**, compiled
  `-std=c++20 -O2`, standard library only, no files, no threads. As a rule of thumb, `check`'s
  "slowest turn" must stay under **0.5 ms** (strategy-v1's is 0.1 ms ≈ 9M points).
- No exceptions escaping, no avoidable deaths into walls or itself (`check` catches these).
- No `getenv` or other runtime knobs: every setting is a constant in your sources (the
  league and the judge run your code as committed). Debug output must be compiled out.
- 500 rounds; at the end the longer QUEEN (your dragon id 0 or 1) wins, then the longest dragon, then total length (unswbc 1.2.3). A team with no
  dragons left loses immediately.

## Submit

1. Fill in `variants/{variant}/NOTES.md`: the idea; what changed from the parents and why;
   what you measured (your sim numbers, which maps, which opponents); what you expect to
   improve; what you tried that didn't work.
2. Don't run `scripts/submit.sh` yourself (the orchestrator tags): make sure
   `build/tools/check {variant} maps/official` says "all checks passed", commit everything in
   `variants/{variant}/`, and say in your report whether it is ready to submit.
3. Submit only if your variant is at least even with its parents in your measurements. If it
   isn't, don't submit; your report is still valuable.

## Report back (your final message)

Plain text, short: submitted or not (and the tag); the idea in two sentences; your
measurements (opponent, maps, W-D-L); what you learned that others should know (things that
helped, things that didn't, surprises about the game); what you'd try next.

Never: edit outside `variants/{variant}/`, look in `../syeyoung-tokens.league/maps/` or its
database, push, run `unswbc submit`, or read `~/.unswbc`.
