# late-surge-18

## Idea

Port gather-split-trade-42's known-portal fix onto late-surge-17. `SafeStep` caps a blind
step (landing tile out of view) at 1 liberty, and a goal step needs 2 liberties; a portal's
far end is always out of view, so every goal step onto a *known* portal was refused as blind,
turning it into a detour. gst42 fixed this for known portals (distinct from the existing
unknown-portal explore-probe) by remembering a goal step refused only for blindness and
pinging through the portal next time we stand beside it, lifting the cap when the ping came
back clean, the exit was seen empty within 2 rounds, or the same goal was refused twice within
30 rounds without a dirty ping.

## What changed from the parents, and why

Parent late-surge-17 (namespace renamed ls17 -> ls18, `LS17_*` -> `LS18_*`, files renamed
ls17.{cpp,hpp} -> ls18.{cpp,hpp}). Ported gst42's diff against gst39 verbatim (late-surge's
movement layer is gst39's, per late-surge-15), behind two new params matching gst42's:
`known_portal = 1`, `blind_fallback = 2`.

- `SafeStep`: a blind step whose cap would otherwise apply is lifted when (a) last turn's ping
  from here through that portal side echoed clean, (b) the exit tile was seen empty within the
  last 2 rounds, or (c) the same goal's blind step was already refused `blind_fallback` times
  within 30 rounds and our last ping didn't find a dragon. A goal step refused only for
  blindness is remembered (tile, portal direction, goal, streak).
- `Turn`: when stepping back onto the remembered tile, ping only through that remembered
  portal direction (replaces the champion broadcast that turn), so next turn's echo can lift
  the cap.
- New per-agent state: `blind_tile_`, `blind_goal_`, `blind_round_`, `blind_dir_`,
  `blind_streak_`, `exit_dirty_`, `cur_goal_` (mirrors gst42's fields exactly).

No late-surge-specific logic (feed, cluster bonus, wait factor, open-centre) touched; this is
purely the movement-layer port gst42 already validated.

## Measurements

- `check late-surge-18 maps/official`: all checks passed, 0 avoidable deaths.
- vs late-surge-17, official maps (both seats, 11 maps x2): 10-0-12, longest -3.2, total -12.2.
- vs late-surge-17, 20 fresh worldgen maps (both seats): 20-0-20, longest +0.1, total -4.5.
- vs gather-split-trade-42, official maps (both seats): 12-0-10.
- Portal action counts appeared as expected: `portal: pinged clear`, `portal: exit seen empty`,
  `portal: fallback` all fire (low single digits per match on official maps, matching gst42's
  own measured rate), with no new avoidable deaths flagged by `check`.

Combined official + worldgen: 30-0-32 (W-D-L), essentially even with late-surge-17 within
noise (small margin deficit, same shape gst42 reported against gst39: "even within noise").

## What should improve

Portal-heavy maps and any opponent farming portal-adjacent stores, per gst42's notes
(Default/Trauma are mostly portal-only beds). Should stop the ~100-turn portal-circling
gst42's report described on schooltime-like layouts once a portal is known.

## Concerns / what didn't work

Did not re-derive gst42's ~2.5 deaths/match risk from newly-lifted crossings independently
(budget); trusting gst42's measurement since the ported code is verbatim. Did not sweep
`blind_fallback` or add the two-liberty-exit-neighbour safeguard gst42's own notes suggested
as a follow-up — out of scope for this port; worth trying in a future task if schooltime
losses recur.
