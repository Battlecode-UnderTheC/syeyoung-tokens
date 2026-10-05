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
4. **Test with arena duels, not heavy local sims.** The Mac is shared; the arena has hundreds of judge slots.
   - Local sim: a quick smoke run only, e.g. one seed on maps/server.
   - Then `scripts/make-bot.sh variants/{variant} <out>` and `tokenz ephemeral <out>`, which gives an `eph-...` id. It isn't on the ladder and expires in 24h.
   - Keep duels LIGHTWEIGHT, about the size of the old local check: `tokenz duel <eph-id> {base_id} --suite server --seeds random:2 --wait` (17 maps × 2 seeds × 2 seats = 68 games). Optionally add one more light duel on `--suite official --seeds random:2`, or against `cs-queen-split-tail-risk-v1-7`. Never run full-suite duels: the draft's own arena games are the thorough test.
   - Report each duel's W-L by suite, the 2 v 3+ starter split, and the per-side metrics it returns.
   - A clear duel loss against the base means stop and report; don't submit.
5. Make the bot and submit it:
   - `scripts/make-bot.sh variants/{variant} <out>`
   - `tokenz --agent {task} submit <out> --bot {line} --parent {base_id} --note "Hypothesis: ... Change: ... Local: ..." --wait`
   - If the permission system blocks the submit, don't work around it. Report "ready" with
     the bot dir and the note, and the orchestrator submits it.
6. Don't promote. The orchestrator checks each verdict and promotes. Report `beats_parent`
   as the arena gives it.
7. Commit `variants/{variant}/`: the code, `variant.toml` and `NOTES.md` (idea, change,
   local numbers, arena verdict).

## Watch replays (metrics are not the goal)
The metrics in this brief (eval P(win), counts at round N) are evidence, not the target. A change can move a number without fixing the behaviour (Goodhart's law).
- Before and after your change, render and watch replays of your bot in the situation the change is for. Use the render tools in `../syeyoung-tokens.league/replays/*/tools`, or arena game replays.
- Describe concretely what the dragons now do differently, and whether it is what was intended and sensible.
- If a metric moved but the replays don't show the intended behaviour (or show a side effect), say so: that is not a success.

## CPU
A turn over the judge's 100M CPU-point cap kills that dragon. Keep the worst turn under 80M: check `tokenz report <your id>` (the `cpu:` line, max) after the verdict, and don't add per-turn work without a cap.

## Report back (at most 12 lines)

- the arena id (or "ready, submit blocked" with the bot dir and note);
- the verdict vs the parent (W-D-L), and how games were decided;
- the results vs the other bots, and the metric deltas;
- the arena verdict SPLIT BY SUITE (`tokenz games`): server (fixed seeds) vs official and rotating;
- the local numbers;
- the CPU line from `tokenz report <id>` (p99 and max);
- the eval P(win) vs the parent: average `game.eval` at r100/r200/r300 over your games against the parent, from `GET /api/v1/games/{id}/metrics`. It is calibrated on about 8k real games a day, and has less variance than W-L;
- what the replays show: 2-3 concrete game ids and what changed in them;
- what you'd try next.

## Rules (non-negotiable)

- **No hidden map knowledge** (user, 2026-10-05).
  - Every new threshold or trigger must name the OBSERVED cause it responds to and why that value. 'It fixed Stripes' is not a reason.
  - No board-size gates, no aiming at fixed board positions (e.g. the geometric centre), and no constants picked to fit particular maps.
  - Prefer general mechanisms (valuing food, room, danger and numbers from what dragons see) over new 'if X starters and queen ≤ N' rules.
  - The orchestrator reviews your diff for this and rejects violations.
- **Report per map class**, not only the total: 1 / 2 / 3+ starters; cramped / corridor / open; sealed queen; central cluster or not. A clear loss in any class means not ready. Also run one light check on freshly generated maps (`tools/mapgen` once it exists, otherwise the sim's worldgen).

- **Write only** `variants/{variant}/` in your own worktree. Don't push, don't tag, and
  don't run `scripts/submit.sh`.
- **No environment-variable reads**, no map-keyed logic (map names, exact sizes, lookup
  tables), no exploits of a specific opponent, no RL.
- **The queen plays safe:** she never enters mazes, corridors or pockets.
- **Never** start server games or upload to the competition server.
- **Never** look in `../syeyoung-tokens.league/maps/holdout/`.
- **Use at most 2 threads;** the machine is shared.
- **Never kill processes by pattern** (`pkill -f`, `killall`): other workers' sims run on the same machine. Kill only PIDs you started.
