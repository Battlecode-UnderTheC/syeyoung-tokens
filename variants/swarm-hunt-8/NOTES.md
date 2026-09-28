# swarm-hunt-8

## Idea

Pure parameter tweak on swarm-hunt-7: extend the hunters' (split children's) split
window from `split_until_round = 300` to `350`. Two sibling lineages found the same
knob mattered: gather-split-trade-13 got its best single-tweak win by moving
300 -> 350, and counter-swarm-12 collapsed when its split cutoff stayed at 300.
Hunters that keep splitting a bit longer should keep more small, expendable units
in play through the mid-game instead of consolidating into fewer, more vulnerable
segments too early.

## What changed from the parents, and why

- `variants/swarm-hunt-8/hunt.hpp`: `Params::split_until_round` 300 -> 350. This is
  the hunter (non-keeper) split cutoff; `keeper_split_until` (also 300) was left
  untouched since the brief's focus is specifically the hunters'/split children's
  window, not the keepers'.
- Namespace renamed `swh7` -> `swh8` throughout (`hunt.cpp`, `hunt.hpp`,
  `plugin.cpp`) so it doesn't clash with the parent when both are loaded into one
  simulator. No macro names used the `SWH7` prefix, so nothing else to rename.
- No other code changes.

## What was measured

- `build/tools/check swarm-hunt-8 maps/official`: all checks passed, 0 avoidable
  deaths on every map.
- `sim --a swarm-hunt-8 --b swarm-hunt-7 --maps maps/official --threads 2` (11 maps,
  both sides played as A and B): **13-0-9** W-D-L over 22 matches; at the end
  swarm-hunt-8 leads by **+7.2** longest-dragon margin and +56.4 total length.
- `sim --a swarm-hunt-8 --b swarm-hunt-7` on 20 fresh worldgen maps (`worldgen.py
  --batch 20`), both sides as A and B: **24-0-16** W-D-L over 40 matches; leads by
  **+5.5** longest-dragon margin and +77.6 total length. Mean match length rose
  from ~333 to ~391 rounds, consistent with hunters staying viable (splitting)
  further into the game before the 500-round cap.

Both official-map and worldgen results agree in direction and magnitude, so this
isn't a single-map artifact.

## What should improve

More hunter mass alive and splittable through rounds 300-350 should mean more
active hunting/trading pressure on the opponent in that window, which is exactly
where counter-swarm-12's collapse (stopping splits at 300) hurt it. The win rate
gain (13-0-9, 24-0-16) and positive length margins on both map sets confirm the
change helps broadly, matching the pattern already seen in the other two
lineages.

## What didn't work / wasn't tried

Only the one parameter was changed, per the brief's budget. Did not try
`keeper_split_until` (a different split window, out of focus), did not sweep to
400 (only tried since 300 -> 350 already won clearly; the brief's fallback of 400
was for the case where the window was already >= 350), and did not re-tune any
other parameter (feed timing, unit_reserve, etc.) that might interact with a
longer split window.
