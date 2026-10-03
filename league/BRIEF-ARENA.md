# Worker brief: {task} ({variant})

You make **one** change to one of our bots for UNSW Battlecode "dragons" and get it judged
by our team arena. You start with no context, so this brief is everything. Read it in full,
then follow it.

## The game in one paragraph

Rules: `docs/game.md`, unswbc 1.2.7.
- 500 rounds; two teams of snake-like dragons.
- Dragons eat pearls to grow and split to make more dragons (max 64).
- A dragon dies by moving into kelp or a body, or in a head-to-head trade.
- Your queen is your dragon with id 0 or 1.
- At r500 the winner is the team with the longer queen (a dead queen counts as 0), then the
  longest dragon, then total length. A team with no dragons left loses.
- In a multi-step move, the first ceil(L/4) steps are free; each extra step costs a segment.

## Your task

- **Base:** {base} (arena id `{base_id}`, line `{line}`)
- **Change:** {change}
- **Evidence it answers:** {evidence}
- **How to tell if it worked:** {success}

## Where things are

| What | Where |
|---|---|
| Your worktree | `{worktree}`, branch `task/{task}` |
| Your variant | `variants/{variant}/`, copied from the base. If it is empty, run `tokenz get {base_id} <dir>` and copy the sources in. |
| What we know | `league/KNOWLEDGE.md`: read its last ~8 sections first |
| Real-match findings | `{report_path}` |

## Steps

1. Read the base's code around the behaviour you are changing.
2. Make the ONE change. Don't sweep parameters or bundle a second idea.
3. Build and check:
   `cmake -S . -B build && cmake --build build -j 2 --target {variant} sim check`;
   `build/tools/check {variant} maps/official` must pass.
4. Quick local test against the base, both seats, 2 threads:
   `build/tools/sim --a {variant} --b <base> --maps maps/server --seed 1,2,3,4 --threads 2`
   (also `maps/official` and `maps/tournament-sprint`). Put the numbers in `NOTES.md`.
   - A clear local loss means stop and report; don't submit.
   - Local numbers are hints. The arena decides.
5. Make the bot and submit it:
   - `scripts/make-bot.sh variants/{variant} <out>`
   - `tokenz --agent {task} submit <out> --bot {line} --parent {base_id} --note "Hypothesis: ... Change: ... Local: ..." --wait`
6. If `verdict.vs_parent.beats_parent` is true, run `tokenz promote <id>`. Otherwise don't
   promote.
7. Commit `variants/{variant}/`: the code, `variant.toml` and `NOTES.md` (idea, change,
   local numbers, arena verdict).

## Report back (at most 12 lines)

- the arena id;
- the verdict vs the parent (W-D-L), and how games were decided;
- the results vs the other bots;
- the metric deltas (`curl -fsSL https://tokenz-arena.lonk-kelvin.ts.net/api/v1/submissions/<id>/metrics`);
- promoted or not;
- what you'd try next.

## Rules (non-negotiable)

- **Write only** `variants/{variant}/` in your own worktree. Don't push, don't tag, and
  don't run `scripts/submit.sh`.
- **No environment-variable reads**, no map-keyed logic (map names, exact sizes, lookup
  tables), no exploits of a specific opponent, no RL.
- **The queen plays safe:** she never enters mazes, corridors or pockets.
- **Never** start server games or upload to the competition server.
- **Never** look in `../syeyoung-tokens.league/maps/holdout/`.
- **Use at most 2 threads;** the machine is shared.
