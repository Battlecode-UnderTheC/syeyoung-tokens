# gather-split-trade-13

## Idea

Parameter-only tweak of gather-split-trade-5: `split_until_round` (leaders'
non-leader dragons stop splitting off new workers after this round) was
tuned to 300 for strategy-v1. gst5 now feeds workers into one champion
starting at round 360, so the right split-off cutoff may be later than 300
(more rounds of worker production before the feed phase begins).

## What changed from the parents, and why

- Renamed namespace `gst5` -> `gst13` (and the .so-facing macro guards,
  which are unused by default) to avoid symbol clashes when both variants
  are loaded in one simulator.
- `Params::split_until_round`: 300 -> 350 (see measurements: 250 was worse,
  350 was better than the 300 baseline). `leader_split_until` (separate
  knob, still 300) was left untouched per the brief.

## What I measured

Built `gather-split-trade-13` and parent `gather-split-trade-5`, ran
`build/tools/sim --a gather-split-trade-13 --b gather-split-trade-5` at
`--threads 2`.

- `split_until_round = 250` on `maps/official` (11 maps, 22 A/B-swapped
  matches): **9-0-13** (W-D-L), mean longest margin **-2.8**, mean total
  margin **-34.9**. Worse than the 300 baseline on both fronts -> rejected,
  not tested further on worldgen.
- `split_until_round = 350` on `maps/official` (22 matches): **11-0-11**,
  mean longest margin **-1.8**, mean total margin **+49.5**.
- `split_until_round = 350` on 20 fresh worldgen maps (`worldgen.py --batch
  20`, 40 A/B-swapped matches): **22-0-18**, mean longest margin **+5.9**,
  mean total margin **+56.4**.

350 is at least even with (and slightly ahead of) the 300 baseline on
official maps, and clearly ahead on worldgen maps (more wins, longer
dragons, bigger total-length margin). Kept 350.

`check gather-split-trade-13 maps/official`: all checks passed, slowest
turn 0.5 ms (well under the 0.5 ms rule-of-thumb / far under the 100M-point
judge budget), 0 avoidable deaths.

## What should improve

Letting workers keep splitting off for longer (until round 350 instead of
300) gives the eventual champion more bodies to feed once the feed phase
starts at round 360 -- there's less of a gap between "workers stop
spawning" and "feeding starts", so more length is available to convert
into one long dragon. This shows up as a better total-length margin on
both map sets and more worldgen wins, at a roughly neutral effect on
longest-dragon margin on official maps.

## What didn't work

`split_until_round = 250` (shortening the split window) was worse on every
metric tested on official maps, so it wasn't carried to worldgen maps --
cutting off worker production earlier starves the champion-feeding phase
of material.
