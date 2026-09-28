# counter-swarm-16

## Idea

Crossover of counter-swarm-15 (base) with gather-split-trade-31's step safety: every candidate
step in SafeStep is judged by a timed flood of our own body after the step (EscapeLevel, the tail
frees tiles as it moves) and, when the flood calls it doomed, by gst31's exact way-out search
(TurnOut, budget 3000). counter-swarm's strikes, guard, liberties and goal logic are unchanged.

## What changed from the parents, and why

- Namespace cs15 -> cs16 (files, macros).
- `EscapeLevel` gains gst31's `count` output (tiles reached); `TurnOut` copied from gst31.
- SafeStep: the goal step is taken only if escape level >= 1 (plus cs15's room/libs/exposed
  rules); the fallback ranking key is now exposed > escape level > libs>=2 > tiles reached >
  room > libs (cs15's key had no escape term). New params `step_escape = 1`, `turnout_budget = 3000`.
- Not ported (surgical, per the brief): gst31's liberties definition, avoid_blind, cap_safety /
  trap_small, "doomed: split". cs15's RoomAround/min_libs/guard stay.

Diff finding: this per-step body-simulated flood is the main difference between the two
SafeStep paths; cs15 judged steps only by RoomAround (static room), so it walked into pockets
it could not turn round in (its "trapped: no move" deaths).

## Measurements (build/tools/sim, both sides per map; P1, P2 = two fresh batches of 20 worldgen maps)

| a vs b | official (22) | P1 (40) | P2 (40) |
|---|---|---|---|
| cs16 vs counter-swarm-15 | 11-0-11, total +4.0 | 19-0-21, total -24.1 | 24-0-16, longest +2.1, total +32.6 |
| cs16 vs gather-split-trade-31 | 12-0-10, longest +2.4, total -60.3 | 19-0-21, total -32.8 | 23-0-17, longest +5.2, total -49.5 |
| (baseline) cs15 vs gst31 | 11-0-11 (from cs15 notes) | 18-0-22, total -16.2 | - |

Totals: vs cs15 54-48; vs gst31 54-48. check: all checks passed. Slowest turn went up to
~10-20 ms under heavy machine load (load avg ~27); gst31 with the same code shows 25 ms on
stronghold under the same load, so the cost matches gst31's.

## What should improve

Fewer walks into dead pockets, slightly better wins vs gst31. The total-length gap to gst31
(~-30..-60) is NOT closed by this: the stepping logic doesn't explain most of it.

## Tried, didn't work

Nothing else tried (short loop). The "detour" count rose (155 -> 203 on official) as expected,
but "trapped: no move" did not fall.
