# swarm-hunt-18

## Idea

Crossover: swarm-hunt-17's roles/movement (namespace swh17 -> swh18) plus
gather-split-trade-42's known-portal fix, ported onto swh17's copy of the same SafeStep
(both trace back to gather-split-trade-38's movement layer, so the port is close to
mechanical). A known portal is not a one-way door: `SafeStep` capped a blind step (landing
tile out of view) at 1 liberty, but a goal step needs 2, and a portal's far end is always out
of view, so every goal step onto a known portal became "detour (goal step unsafe)" -- dragons
circled portals instead of crossing them.

## What changed from the parents, and why

All new code is behind `known_portal = 1` / `blind_fallback = 2` (matching gst42's defaults;
`known_portal = 0` reproduces swh17 exactly).

- `SafeStep`: when the preferred (goal) step is blind only because of a known portal, the cap
  is lifted when last turn's ping through that side came back clean ("portal: pinged clear"),
  the exit tile was seen empty within 2 rounds ("portal: exit seen empty"), or the same goal's
  blind step was already refused `blind_fallback` (2) times within 30 rounds and our ping did
  not echo a dragon ("portal: fallback"). A refused blind goal step is remembered (tile,
  portal direction, goal, streak).
- `Turn`: stepping back onto the remembered tile pings only through that portal direction
  (replacing the champion broadcast that turn), same trick as swh17's existing unknown-portal
  probe; skipped if the portal side is opposite our facing (that ping starts at the tail).
- Added `cur_goal_` (set around the goal-driven `SafeStep` call in `Decide`, not the survivor
  flee call) so `SafeStep` knows which goal a blind refusal belongs to.
- Mirroring: swh17's `MirrorAgent` already rotates the whole observation into a canonical
  frame before calling `HuntAgent::Turn` and rotates steps/sonar back out
  (`d -> (d+2)%4`) uniformly. The new ping (`ping_dir_`, `a.sonar[blind_dir_]`) uses the same
  fields and code path as the existing unknown-portal ping, which already works correctly for
  team B under this scheme -- no special-casing needed, and the sim results below (both seats
  in every match) confirm it isn't reversed for team B.
- Namespace/macros: swh17 -> swh18, SWH17_* -> SWH18_* (mechanical, no clash with either parent).

## What I measured

`check swarm-hunt-18 maps/official`: all checks passed, 0 avoidable deaths.

vs swarm-hunt-17 (parent, both seats):
- official (11 maps, 22 matches): 12-0-10, longest +0.2, total -0.3 (even)
- worldgen batch (20 maps, 40 matches): 21-0-19, longest +1.4, total +13.5
- portal-related actions appeared and fired correctly (e.g. on official: "portal: pinged
  clear" x4, "portal: exit seen empty" x1, "portal: fallback" x1 for swh18 vs 0 for swh17;
  "detour (goal step unsafe)" fell 420 -> 375 over the same run), matching gst42's mechanism.

vs gather-split-trade-42 (parent, both seats):
- official (11 maps, 22 matches): 12-0-10, longest +4.1, total +8.4

Budget did not allow a second worldgen set against gst42; the official-map and one
worldgen-set-vs-swh17 numbers above are what I have.

## What should improve

Portal-heavy maps for swarm-hunt (same mechanism gst42 found: Default, Trauma). The lifted
counts here are much smaller than gst42's (swh17 is a different archetype with less pure
gathering), so the effect is modest but consistently non-negative in these runs.

## What I tried that didn't work / concerns

- Didn't attempt gst42's own noted follow-up (requiring 2 liberties on the exit's known
  neighbours, or a second ping) -- out of budget; carries the same known risk gst42 flagged
  (a clean echo only clears the straight line past the exit, not a head waiting beside it).
- Didn't run a second worldgen set or a trace on a specific loss; numbers above are even to
  slightly ahead of both parents within the noise these small samples carry.
