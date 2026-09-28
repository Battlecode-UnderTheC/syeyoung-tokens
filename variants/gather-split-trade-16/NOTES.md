# gather-split-trade-16

Mutation (task g4-7) of gather-split-trade-5. Focus: bed denial.

## Idea

From the engine (`Game::BeginRound`): a bed's shared countdown fires on schedule whether or not
it grows a pearl; a bed with a living segment on it at that tick simply gets no pearl this
cycle and draws a new gap. A dragon of length L that steps onto a tile in round t covers it
at the ticks of rounds t+1 .. t+L (head, then each body segment in turn, tail leaves in
round t+L after the tick). So "parking" is unnecessary: crossing a bed within L rounds
before its tick blocks it. Eating needs the opposite: stepping on in the tick round itself,
which the lower id wins when both heads wait next to it (g4-3).

## What changed

- Namespace `gst5` -> `gst16`.
- Ported gather-split-trade-12's contest rule: a target an enemy head in view reaches first
  (or ties at a bed tick with a lower id) is worth `contest_factor` 0.3.
- New: when such a lost target is a bed still growing (due >= 1) that we can step onto
  before its tick (path dist d <= due), it is instead worth `deny_value` 0.6 over
  max(d, due - length + 1) + 1 rounds (aim to cross it within our length before the tick).
  Adjacent but too early (due - 1 >= length): not a denial (stepping on now uncovers it by
  the tick). Reason "deny bed" when the step toward such a goal is taken.
- Not done: late-game long-body denial of enemy-half beds (the champion's safety first; no
  time in this budget).

## Measured (native sim vs gather-split-trade-5, both sides)

| maps | W-D-L | longest margin | total margin |
|---|---|---|---|
| official (11) | 13-0-9 | +2.8 | -3.7 |
| worldgen, 20 fresh | 20-0-20 | -2.9 | +14.1 |

"deny bed" steps: ~145 on official, ~267 on worldgen (sim tallies over the set, ~2.5% of
steps). check maps/official: all checks passed. Death causes unchanged within noise.

## Expect / didn't try

About even with the parent; the edge (if any) is small because contests seen in a 7x7
window are rare. Didn't measure how many ticks were actually blocked (would need engine
instrumentation). Didn't try: body-over-enemy-beds late, tuning deny_value.
