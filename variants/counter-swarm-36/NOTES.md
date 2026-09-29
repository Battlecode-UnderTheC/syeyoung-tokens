# counter-swarm-36

Mutate of counter-swarm-32 (`cs32` namespace renamed to `cs36`, task g18-5).

## Idea

counter-swarm-32's `ChooseGoal` calls any goal whose `RoomAround` count falls short of
`RoomNeeded()` a dead end, and only enters it when the pocket's pearls beat `trap_cost`.
But `RoomAround` floods forward only — it can't see a dragon following its own tail out,
so a 1-2 wide pocket or a 1-wide corridor that opens back into room just past the flood's
`need` cap reads as a dead end even when the dragon can walk in and turn around for free.
Real-server replays showed us skipping exactly these tiles against stronger opponents.

Replaced the `RoomAround < need` dead-end test with a projected-body escape test: when a
goal's `RoomAround` comes up short, walk the shortest open-tile path from the head to the
goal (backtracked through the turn's already-computed `paths.dist`, not a fresh flood — see
below), lay the body along that path (truncated/extended by the surviving old tail to
`length_` tiles), and run the existing tail-following `EscapeLevel` BFS on the *projected*
body. If it escapes (level >= 1), the goal is not a dead end at all: it is scored normally,
and `SafeStep` is told (`Goal::escapes`) to relax its `room >= need` requirement for that
specific step so it isn't rejected as "unsafe" on the way there. If the projected body can't
escape, we fall through to the old trap-value scoring (or `no_trap` skip) unchanged.

## What changed from the parents, and why

- `Goal` gained `bool escapes`; `ChooseGoal`'s dead-end branch now tries
  `EscapeLevel(ProjectedBody(c.tile, paths), EscapeCap(length_))` before falling back to the
  trap-value math.
- New `ProjectedBody(goal, paths)`: reconstructs the path via `paths.dist` backtracking
  (always pick a neighbour one step closer), *not* a fresh BFS — that was tried first and blew
  the turn budget on open maps (86ms/turn on `big_empty`, since it re-flooded the whole board
  per trap candidate). Backtracking is O(path length) and reuses the search already done for
  the turn.
- `SafeStep` takes a third `escape_ok` parameter; when set, its "toward goal" branch accepts
  `room < need` as long as the one-turn `EscapeLevel` check still passes.
- Tried and reverted: also dropping `!Exposed(t)` from `SafeStep`'s own `RoomAround` predicate
  (an orchestrator note from g18-4 said this helped gst90's line, since an enemy head near an
  interior tile otherwise seals a flood that `ChooseGoal`'s Occupied-only flood thinks is open).
  On counter-swarm's line it made `labyrinth` *worse* (15-25 vs 23-17 without it), so it's not
  in this variant. Left as a note for whoever revisits `SafeStep`'s room predicate here.

## Measured (vs counter-swarm-32, `--threads 2`)

- official maps: 12-0-10 (was 13-0-9 with the reverted Exposed tweak also in — kept the
  version without it since it's better where it matters, see below)
- 20 labyrinth (worldgen `--style labyrinth --seed 20`): 23-0-17
- 20 caverns (worldgen `--style caverns --seed 20`): 18-0-22
- 20 default worldgen (`--seed 20`): 23-0-17
- combined: 76-0-66 (net positive, roughly even to slightly ahead on the corridor-heavy sets
  this task targets)
- vs gather-split-trade-90, official maps: 13-0-9
- vs trading-swarm-5, official maps: 17-0-5
- `check counter-swarm-36 maps/official`: all checks passed, slowest turn <= ~0.7ms (the
  early 86ms figure was the pre-backtrack ProjectedBody; fixed before measuring the numbers
  above). "toward goal (escapes)" fires ~100 times per 22-match official run, so the new path
  is genuinely exercised, not dead code.

## What I'd try next

- caverns (18-22) is the weakest set; worth a trace on a caverns loss to see whether the
  projected-body path itself is unsafe in a way EscapeLevel's optimistic no-growth assumption
  misses (portals or enemy heads along the path aren't modelled).
- Re-examine the `!Exposed(t)` SafeStep predicate question directly on counter-swarm's line
  with a dedicated set of trace matches — g18-4 measured it on gst90, not this line, and it
  went the other way here.
