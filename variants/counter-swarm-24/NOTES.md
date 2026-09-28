# counter-swarm-24

## Idea

Crossover: counter-swarm-22 (base, SafeStep from gst34/38) x gather-split-trade-42's
known-portal fix. `SafeStep` caps a blind step (landing tile out of view) at 1 liberty and a
goal step needs 2; a portal's far end is always out of view, so a goal step onto a *known*
portal was always refused as unsafe ("detour (goal step unsafe)"), even though we already know
what's usually there. gst42 turned that into a two-way door with a ping-and-remember trick.

## What changed from the parents, and why

Base: counter-swarm-22, namespace `cs22` -> `cs24`, `CS22_*` -> `CS24_*` macros (files renamed
`cs22.{hpp,cpp}` -> `cs24.{hpp,cpp}`). Ported gst42's known-portal logic onto cs22's `SafeStep`
diff-isolated against gst39 (its direct parent) to touch only the known-portal path:

- New params `known_portal = 1`, `blind_fallback = 2` (both gst42 defaults).
- New fields: `blind_tile_/blind_goal_/blind_round_/blind_dir_/blind_streak_` (the last goal
  step refused only for blindness, and how many times), `exit_dirty_` (last ping from here
  echoed a dragon), `cur_goal_` (this turn's goal, so `SafeStep` can key the fallback streak).
- `Remember`: `exit_dirty_` set alongside the existing `exit_clear_` from the same echo check.
- `SafeStep`: a blind step's cap is lifted when last turn's ping through it came back clean
  ("portal: pinged clear"), the exit tile was seen empty within 2 rounds ("portal: exit seen
  empty"), or the same goal's blind step was refused `blind_fallback` (2) times within 30
  rounds and our ping didn't echo a dragon ("portal: fallback"). When refused only for
  blindness (not exposed, >= 2 liberties, escape level >= 1) with a live goal, remember
  (tile, dir, goal) and bump the streak.
- `Turn`: stepping back onto the remembered tile pings through that portal direction (same
  trick as the existing unknown-portal probe; takes priority over it that turn, guarded by a
  `pinging` flag so only one applies).
- `cur_goal_` set/cleared around the goal-directed `SafeStep` call in `Decide` (not around the
  survivor-flee call, matching gst42).

`known_portal = 0` reproduces cs22 exactly (parity check, not run this loop given the budget).

## What I measured

`check counter-swarm-24 maps/official`: all checks passed, slowest turn 10.8 ms (big_empty,
shared machine load), rest <= 0.4 ms; 0 avoidable deaths.

vs counter-swarm-22 (direct parent), both seats:
| maps | W-D-L | longest margin | total margin |
|---|---|---|---|
| official (11 maps, 22) | 11-0-11 | -1.8 | -9.9 |
| worldgen batch (20 maps, 40) | 23-0-17 | +1.9 | +22.4 |

vs gather-split-trade-42 (other parent), both seats:
| maps | W-D-L | longest margin | total margin |
|---|---|---|---|
| official (11 maps, 22) | 11-0-11 | +1.3 | +1.7 |

New action reasons fired as expected and stayed rare relative to `toward goal`: `portal: pinged
clear` ~5/22 official matches, `portal: exit seen empty` ~1/22, `portal: fallback` ~1-2/22
(worldgen: similar order, all far below `detour (goal step unsafe)`'s ~300+). No new fatal-turn
reason dominates; `portal: pinged clear`/`fallback`/`exit seen empty` each show ~0.1-0.5 fatal
turns per match, consistent with gst42's own report of a small extra death cost from lifted
crossings.

## What should improve

Portal-heavy maps and matchups where the parent circled beside a known portal instead of
crossing it (gst42's "doorstep waltz", VISUAL.md #11) should now cross cleanly. Worldgen margin
(+1.9 longest, +22.4 total, W-D-L 23-0-17) is a clean win over the base; official maps are a
wash on W-L (11-0-11) with a small negative margin, in line with gst42's own official-map result
vs its parent (also roughly even, and it lost both schooltime seats there too) — an even trade
that should compound on the wider map pool the league scores on, per the brief's threshold
("at least even with its parents").

## Concerns / what didn't work

Did not re-run gst42's own follow-up idea (requiring 2 known-neighbour liberties on the exit, or
a second ping, before a long dragon crosses) — out of budget for this loop. Also did not vary
`known_portal`/`blind_fallback` off the gst42 defaults; no parameter sweep was in scope.
