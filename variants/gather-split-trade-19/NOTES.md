# gather-split-trade-19

Crossover (task g5-3): gather-split-trade-15 (base) + gather-split-trade-16's bed denial.

## Idea

gst15 (league leader) already discounts a target an enemy head in view reaches first to 0.3.
gst16 adds, on top of that same contest rule, bed denial: when such a lost target is a bed
still growing and we can step onto it before its tick, cross it so a segment covers the bed
when it fires (a covered bed grows no pearl). Worth `deny_value` 0.6 over
max(d, due - length + 1) + 1 rounds.

## What changed from the parents, and why

- Base: gst15 sources, namespace/macros `gst15`/`GST15` -> `gst19`/`GST19`.
- Isolated the denial by diffing gst16 against gst12 (whose contest code gst15 carries): only
  the denial block inside the contest loop, `Params::deny_value = 0.6`, `Goal::deny`, the
  `denials` list, and the "deny bed" reason relabel. Ported exactly those; nothing else of gst16.

## Measured (native sim vs gather-split-trade-15, both sides of every map)

| maps | W-D-L | longest / total margin |
|---|---|---|
| official (11) | 11-0-11 | +2.4 / -0.2 |
| worldgen, 20 fresh | 24-0-16 | +2.8 / +3.0 |

"deny bed" steps: 139 on official, 547 on worldgen (~2-3% of steps). Death causes within
noise of the parent (slightly more "trapped: no move": 43 vs 36 official, 319 vs 295 worldgen).
`check` on official: all checks passed. Slowest-turn timings were meaningless during this run
(machine load average ~135; the parent measured equally slow or slower in the same window).

## What should improve

Small edge at most: contests seen in a 7x7 window are rare. Denial costs the enemy a pearl
where we would have got nothing (contest-discounted target).

## Not tried

Tuning deny_value; late-game body-over-enemy-beds denial; measuring actual blocked ticks.
