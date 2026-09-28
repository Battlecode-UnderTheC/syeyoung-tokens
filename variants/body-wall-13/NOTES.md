# body-wall-13

Crossover: body-wall-12 (base) x gather-split-trade-46, task g14-1.

## Idea

body-wall's movement layer is the same SafeStep as gst38/39: a step whose landing tile is out
of view is capped at 1 liberty (`avoid_blind`), and a goal step needs 2. A portal's far end is
always out of view, so every goal step onto a **known** portal (one we've already mapped both
sides of) was refused as a "detour", even with nothing dangerous there — dragons circled
portals beside full stores. Ported gst42's fix (remember a goal step refused only for
blindness; ping through the portal next time we stand beside it; lift the cap if the ping came
back clean, or the exit was seen empty within 2 rounds, or the same goal was refused twice
within 30 rounds) plus gst46's veto (don't lift if the exit's neighbours look risky: an enemy
head beside it within 4 rounds, or more than `exit_risk_max` (2) of its sides are
unknown/occupied/stale) onto body-wall-12. Left out gst46's `GST46_DEBUG`-only lift/death
tally (debug-only, not applicable).

## What changed from the parents, and why

- Namespace/macros renamed `bw12`/`BW12_*` -> `bw13`/`BW13_*` (crossover requirement, no
  clash with either parent when loaded in one simulator).
- `strategy.hpp`: added `known_portal` (1), `blind_fallback` (2), `exit_risk_max` (2),
  `exit_risk_ping` (0) params (same defaults gst42/gst46 shipped with); added state
  `cur_goal_`, `blind_goal_`, `blind_streak_`, `blind_round_`, `blind_tile_`, `blind_dir_`,
  `exit_dirty_` — body-wall-12 already carried the `portal_`/`ping_round_`/`ping_tile_`/
  `ping_dir_`/`exit_clear_` fields from the same gst38/39 lineage, so only the new fields were
  needed.
- `strategy.cpp`:
  - `SafeStep`: the blind check now first asks whether the portal is *known* and, if so,
    whether it can be lifted (clean ping / exit seen empty / fallback-after-two-refusals),
    then applies gst46's exit-risk veto before actually lifting the cap. A step whose lift is
    vetoed still counts as a "blind refusal" for the fallback counter, same as gst42.
  - `Turn`: the sonar-ping block for exploring an unknown portal now checks first whether
    we're stepping back next to a tile whose blind goal step we remembered (`blind_tile_`) and
    pings that direction instead — same priority order as gst42/46 (checked before the
    explore-ping, guarded with `pinging`), so it doesn't fight body-wall-12's own
    champion/threat sonar priority (magic 0x5C encoding untouched).
  - `Decide`: set `cur_goal_ = goal` around the `SafeStep(preferred, ...)` call so SafeStep can
    tell whether the refused step is actually this turn's goal step.

## What I measured

- `check body-wall-13 maps/official`: all checks passed, slowest turn 59.6 ms (schooltime,
  well under the 0.5 ms *per-turn CPU budget* is measured differently by the judge — this is
  wall time of the whole match under the debug harness, not the judge's per-turn CPU metric;
  body-wall-12 has the same profile, unchanged by this port).
- `sim --a body-wall-13 --b body-wall-12 --maps maps/official --threads 2`: **14-0-8**, leads
  by 5.5 longest / 29.1 total at match end. New actions fire: `portal: pinged clear` (7),
  `portal: fallback` (1). Clear win.
- `sim --a body-wall-13 --b body-wall-12` on 20 fresh worldgen maps (`worldgen.py --batch 20`):
  **19-0-21**, -0.7 longest / -8.4 total — essentially even; few portal crossings fired on
  this batch (worldgen maps have fewer/shorter portal chains near goals than the official
  set). Combined with the official-map win, at least even with the parent overall.
- `sim --a body-wall-13 --b gather-split-trade-48 --maps maps/official --threads 2`:
  **11-0-11**, -1.2 longest / +0.9 total — even, as expected against a strong non-parent
  lineage (gst48 already carries its own crossing logic).

## What I'd try next

- The worldgen batch showed almost no `portal:` actions firing at all (for either side),
  suggesting the fix mostly matters on maps with portal-dense goal routes (like the official
  set, per the brief's "68 -> 231 crossings a match" figure from gst42's own report) — worth
  running a larger worldgen batch or one built with more portal pairs to see the effect more
  clearly.
- Didn't get to tune `exit_risk_max`/`blind_fallback` for body-wall's specific play (bigger,
  fewer dragons than gst46's swarm) — inherited gst46's defaults as-is per the brief's budget.
