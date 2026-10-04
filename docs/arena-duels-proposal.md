# Proposal: duels and ephemeral bots for tokenz-arena

For the arena maintainer. This is a request and a sketch only; nothing has been implemented.
It is based on reading tokenz-arena `main` at e98e305.

## Why

Our agents test each bot change with many local sims on one 10-core Mac, which overloads it.
The arena has the capacity to do this instead: `GET /api/v1/queue` shows 20 local and 344
remote slots, with the remote slots idle (`totals.left` 0; the local slots play holdout
filler). But the arena only plays scheduled games. A draft automatically queues games against
its parent and the top 3 bots, enters Ideas and lineage, and is judged mostly on fixed seeds
(`cfg.seeds(suite)[:1]` and the server suite's seeds). We want ad-hoc A-vs-B testing on fresh
seeds that never touches the ladder.

## Ephemeral bots

`POST /api/v1/ephemeral` (multipart: `file`, optional `name`, `ttl_hours`, `agent`) returns
`{id: "eph-<hex>", status: "checking", expires}`. `GET /api/v1/ephemeral[/{id}]` lists or
shows them, and `DELETE /api/v1/ephemeral/{id}` (owner only) removes one.

- It goes through the same `submit._validate_zip`, `_unpack`, env-read check, `build_wasm` and
  check game as a draft, run in the daemon's `check_slots`. It ends `ready` or `rejected`.
- It is a `Submission`-shaped record with `lane="ephemeral"` and an `expires` field. It is
  stored under `data_dir/ephemeral/<id>/` and kept in its own dict (`arena.ephemeral`),
  **not** in `arena.subs`. It is unpacked to `cfg.bots/<id>` so that the local judges,
  `/api/v1/workers/bot|wasm/{id}` and `Registry._describe` (which needs the sha256) work
  unchanged, once they look up both dicts.
- The TTL defaults to 24 h, with a configurable maximum (`[ephemeral] max_ttl_hours`, say 168)
  and a cap of live ephemeral bots per owner (`max_per_owner`, say 20). A daemon task
  (`daemon.every(300, ...)`) deletes expired ones. A duplicate zip from the same owner
  returns the existing bot and renews its TTL.
- An ephemeral bot can only be used in duels.

## Duels

`POST /api/v1/duels` takes JSON:

```json
{"a": "swarm-7", "b": "eph-3f9a", "suites": ["official", "rotating"], "maps": null,
 "seeds": "random:8", "games": 120, "both_seats": true}
```

and returns `{id: "duel-<hex>", status, games_planned}`.

- `a` and `b` are any submission id (drafts included) or ephemeral id. Rejected or external
  bots are refused. If a bot is still checking, the duel is `waiting` until it is ready.
- `suites` can be official, tournament, server, custom or rotating. The holdout and check
  suites are refused, so holdout maps never leave the arena.
- Seeds are `"random:N"` (N fresh seeds per map, the default) or an explicit list. The
  schedule goes one seed round at a time over a shuffled map list, with both seats played back
  to back, and is cut off at `games`. Each game's seed is recorded.
- Other endpoints: `GET /api/v1/duels?owner=`, `DELETE /api/v1/duels/{id}` (cancel), and
  `GET /api/v1/duels/{id}/games/{n}[/replay|/metrics]`. A `/duels/{id}/watch/{n}` page would
  use `viewer.page()`.

`GET /api/v1/duels/{id}?wait=S` returns, from A's side:

- `status` (waiting, queued, playing, done, cancelled, expired), with played, running, left
  and error counts;
- `record`: W-D-L, score and 95% CI overall, and `by_suite` (and `by_map`), using
  `report._row`/`report.interval` (Wilson), so the numbers mean what draft verdicts mean;
- `by_end`: the "decided by" split, using `report.decided_by` (elimination, the tiebreaks,
  draw);
- `by_starters`: W-D-L split by starting dragons per side, `≤2` vs `3+` (or `mixed`). This
  comes from the round-1 entry of `stats["dragons"]` or from the map's start positions;
- `sides.{a,b}`: the existing team metrics, averaged at r100/r200/r300/r400 and at the end
  (as `metrics/views.submission` does), plus:
  - the longest dragon at r300 and r400;
  - queen alive at the end, with the death round and cause (`metrics["queens"]`, using the
    `compare.summarise` logic);
  - deaths by cause, errors and CPU overruns (`stats.teams`);
  - CPU max and p99 (`stats.teams[*].points`, local judge games only);
- `games`: for each game, `n`, suite, map, seed, which bot sat in seat A, the result, rounds,
  reason, and replay and watch URLs.

## Priority and caps

- Add `DUEL = 4` / `"duel"` to `scheduler.LANES`, and give `Job` an optional
  `duel=(duel_id, n)` field that is **included in `Job.key`**. That way a duel game never
  collides with a draft's game of the same (a, b, suite, map, seed) in `arena.running` or
  `GameQueue.played`.
- `GameQueue._pick` fills duel jobs **last**, after the owed games (draft 0, placement 1,
  rated top-up 3) and the idle filler (holdout and `_continuous`). A small `DuelBook`
  (`arena/duels.py`, like `queue.get()`) holds each duel's schedule and recorded results. Within
  the tier, the order is the owner with the fewest running duel games, then the oldest duel,
  then `n`. Because the remote slots are idle today, duels would mostly run there. The
  maintainer could choose to put duels before the filler (`[duels] before_filler`).
- Caps (`[duels]`): `owner_running` (concurrent duel games per owner, say 128), `max_running`
  (all duels; 0 means no cap), `max_games` per duel (say 1000; larger requests get 400), and
  `max_active_per_owner` (say 10).
- `Daemon.record_result`: a job with `duel` set is handed to the `DuelBook` instead of
  `arena.add_game`. It is deduplicated by `(duel, n)`, as `is_played` does today.
  `GameQueue.release` (an expired lease) needs no extra work: a duel game that isn't recorded
  and isn't running is pending again. After a restart, unfinished duel games are simply
  pending.

## Isolation

Duel games are **not** `Game`s in `arena.games` or `games/<id>.json`. They are stored under
`data_dir/duels/<id>/{meta.json, games/<n>.json, metrics/<n>.json, replays/<n>.replay}`. This
matters because several readers don't filter through `is_rated`:

- the champion gate (`bridge.py` ~L485 uses `g.lane != "server"`);
- `map_stats`, `find_games` and `metrics/views.submission`;
- the evals and the metrics backfill;
- the queue's `played` set.

With separate storage, nothing that feeds ratings, `report.verdict`/beats_parent, Ideas,
lineage, the leaderboard, the auto-submit gate or the evals can see them. Ephemeral bots stay
out of `arena.subs`, so `ranking`, `_usable`, `owed_games` and `/submissions` never list them.
Duel replays sit outside `cfg.replays`, so they don't count against `retention.prune`'s cap. A
whole duel directory is deleted after `[duels] keep_days` (say 7).

The website process (`arena web`) is a read-only replica that doesn't load duels. Its
`to_engine` middleware should forward all of `/api/v1/duels` and `/api/v1/ephemeral` to the
engine, as it already does for `/api/v1/server`.

## Budget and autoscaler

The scaler caps spending at $1/h, $10/day and 6 instances (today: 344 rented slots, $1.94
spent). `scaler.plan` sizes rentals from `totals.left` and `len(running)`. Duels should not buy
machines:

- `GameQueue.view()` reports duel work separately (`totals.duel_left`, `by_lane.duel`, and one
  `waiting` row per duel so the Queue page shows them). It adds at most `[duels] scale_games`
  (default 0) of duel work to `totals.left`.
- `scaler.plan` counts only non-duel running games.

So duels use slots that are already rented or local, and never trigger scale-up. The
maintainer can allow a little scale-up with `scale_games`.

One side effect: duel games running on a rented machine refresh its `busy_t`, so a machine
that is busy only with duels is never idle and is not returned. The scaler could skip
duel-only leases when updating `busy_t`, or accept the cost, which is capped either way.

## CLI and MCP

The CLI goes in `client/tokenz.py`, as new `cmd_*` functions with `add_parser` entries:

- `tokenz ephemeral ./bot [--ttl 24h] [--name x]` zips the bot like `cmd_submit` and prints
  the id. `tokenz ephemeral ls|rm <id>` lists or removes them.
- `tokenz duel A B --suite official,rotating [--maps m1,m2] --seeds random:8 --games 120
  [--one-seat] --wait` prints a compact table: record and CI, `by_suite`, `by_end`,
  `by_starters`, and the side metrics. `tokenz duel show|cancel <id>`.

For MCP, `arena/api/mcp.py` would get `ephemeral` (WRITE: a directory path or base64 zip, like
`submit`), `duel` (WRITE), `duel_result` (READ, with `wait`) and `duel_cancel`. Like the
existing tools, these call the HTTP API in-process. `docs/AGENTS.md`, `llms.txt` and a
`docs/DUELS.md` would describe them.

## Open questions for the maintainer

1. **Tier placement:** strictly last (after the holdout and continuous filler), or before the
   filler? Local slots almost always have holdout filler, so with "last", duels in practice
   run only on remote slots.
2. **Scaling:** should `scale_games` stay at 0? And should duel-only work keep rented machines
   busy (`busy_t`)?
3. **Deployment:** this is new state only (`data_dir/ephemeral`, `data_dir/duels`) plus an
   optional `Submission.expires` field, so no migration is needed. Older remote workers ignore
   the `lane: "duel"` field. Do workers need a version bump to fetch ephemeral bots, or is
   `/workers/bot/{id}` with lookup in both dicts enough?
4. **Rotating maps:** a duel on rotating maps that spans an epoch change can lose maps. Should
   those games be played on the stored path (only local judges can do that), or skipped?
5. **Retention:** what should `keep_days` and a disk cap for duel replays be, or should duels
   keep replays only for losses?
6. **Limits and access:** are the default caps right? Can anyone duel anyone's drafts (they are
   readable anyway), or only their own?
