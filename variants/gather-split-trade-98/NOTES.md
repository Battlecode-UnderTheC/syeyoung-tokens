# gather-split-trade-98

## Idea

gst90 (our live bot) has the portal-crossing machinery (ping-then-cross an unknown portal while
exploring) but no notion of a "poor home region": on Trauma it loses both seats to
trading-swarm-2 because our 4 dragons spend 60+ rounds walking the empty left strip between
corner goals, never treating the maze behind the fence portals as a goal since it's out of
view. gst83 (a sibling lineage, crossover parent here) carries a sharpened version of gst78's
"poor home" flag: when reachable food is scarce relative to allies actually seen nearby (not
the whole team) and more seen-but-unreachable valued tiles exist than reachable ones (a locked
room, not just a spread-out team), a dragon multiplies an unknown portal's explore value 4x and
will cross a clean-pinged portal even with a valued goal still on the board. Ported that block
onto gst90 verbatim (renamed to gst98/GST98_*), since gst90 never had any version of it.

## What changed from the parents, and why

- Renamed `gst90`/`GST90_*` -> `gst98`/`GST98_*` throughout, per the brief.
- In `ChooseGoal` (`strategy.cpp`, the reachable-tile scan just before the cluster pass):
  - Added an `unreachable` counter: a tile with `paths.dist[t] <= 0` but `base[t] > 0` (seen,
    valuable, not currently reachable) counts as seen-but-unreachable.
  - Added the `poor_` block (mirrors gst83): `poor_` is true when `round_ < cross_until` and
    (reachable valued tiles < `max(poor_min, poor_per_unit * allies_here)`, where `allies_here`
    counts teammates actually seen this round plus ourselves, or nothing eaten for
    `starve_rounds`) **and** `unreachable > reachable`.
  - In the portal-explore scoring line: while `poor_` and `WillCross()`, an unknown portal's
    explore value is multiplied by `poor_portal_mult` (4.0), same as gst83/gst78.
  - In the cross-decision (`Decide()`, the "portal: explore" branch): added `(poor_ &&
    p_.poor_cross > 0 && paths.dist[goal] >= p_.poor_cross)` to the OR that lets a dragon step
    through a clean-pinged unknown portal even though it has a valued goal elsewhere.
  - Added `poor_ = false;` reset before `ChooseGoal(paths)` in `Decide()` (gst90 didn't have
    this state at all, so nothing to reset before; matches gst78/83's placement).
  - New params in `Params` (same defaults as gst78/gst83): `poor_per_unit=1`, `poor_min=3`,
    `starve_rounds=35`, `poor_portal_mult=4.0`, `poor_cross=6`. Added to the `GST98_SET` tuning
    macro list (build-time only, gated by `GST98_TUNE`, unused in the shipped binary).
  - Added `poor_` member (`mutable bool`, mirrors gst83's hpp) and `unreachable` local.
  - Added `GST98_DEBUG`-gated counters `g_poor` (turns with `poor_` set) and `g_poor_cross`
    (portal-crossing moves while `poor_`), printed alongside the existing debug line. Compiled
    out in the shipped binary (no `GST98_DEBUG` macro defined by `variant.toml`).

## What I measured

`build/tools/check gather-split-trade-98 maps/official`: all checks passed, slowest turn under
1ms on repeat runs (one run showed transient 15-30ms readings on schooltime/stronghold that
disappeared on rerun — machine contention from other agents, not a real regression: confirmed
against unmodified gst90 which also reads clean).

`build/tools/sim`, 2 threads:

- **Official maps vs gather-split-trade-90** (parent): 11-0-11 over 22 matches; roughly even
  (gst98 -1.0 longest, -3.1 total at game end). Trauma specifically: gst98 wins as B (20-18,
  41-34 total) and loses as A on the tiebreak (16-18 longest) but leads total length 67-21 —
  a much closer/healthier trauma result than the brief's baseline description of losing both
  seats badly.
- **Official maps vs trading-swarm-2** (the opponent named in the brief): 12-1-9 over 22
  matches. **Trauma: gst98 wins BOTH seats** (22-17/32-20 as A, 18-13/36-35 as B) — the goal
  the brief set.
- **Official maps vs trading-swarm-5**: 14-1-7 over 22 matches. Trauma: wins as B (24-13,
  60-41), a tied-longest total-length loss as A (15-15, 18-59) — still healthy overall.
- **20 worldgen maps, seed 5000, vs gather-split-trade-90**, both seats: 22-0-18 over 40
  matches; gst98 leads by 1.2 longest, trails by 12.2 total (mixed, close to even).
- **20 worldgen maps, seed 9000 (independent, >=4000 apart), vs gather-split-trade-90**, both
  seats: 21-0-19 over 40 matches; gst98 leads by 0.9 longest, 11.5 total.

Net: at least even with gst90 on official maps and both worldgen batches, and the Trauma win
against trading-swarm-2 (both seats) the brief asked for is achieved, without a matching
regression anywhere else I measured.

## What I'd try next

- Didn't get to the exact poor-turn/portal-crossing counts from a `GST98_DEBUG` build (the
  brief suggested this to confirm the flag fires by ~r30 on Trauma) — ran out of time budget
  after the W-D-L sims above. The win-both-seats-on-Trauma result is consistent with it firing
  correctly, but a debug build trace on Trauma specifically would confirm the ~r30 timing and
  rule out it firing purely from the starve-branch instead of the portal-maze branch.
  - Also didn't run trace/replay on trauma to eyeball the dragons crossing into the maze the way
    the brief's evidence describes; the aggregate numbers are the only evidence here.
- Didn't test gst68 or a second trading-swarm-2/5 worldgen comparison (only official maps for
  the swarm opponents, per time budget).
- If someone picks this up: gst83's NOTES flagged that `unreachable > reachable` (strict) vs
  `>=` or a ratio wasn't explored; same open question applies here unchanged.
