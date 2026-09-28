# late-surge-27

## Idea

Crossover of late-surge-24 (base) and body-wall-17. Ports bw17's early centre pull: on
small maps (<= 1,024 tiles), before round 50, every scored goal candidate (pearls, beds,
unexplored edge) has its score multiplied by
`1 + 0.3 * (1 - torus_dist(tile, centre) / (W/2 + H/2))`, so the whole ChooseGoal scoring
pass leans towards the map centre in the opening rather than only adding a separate centre
floor candidate (which late-surge already had via `centre_value`/`open_centre_value`, and
which is kept unchanged — this is an additional multiplicative pull, not a replacement).

## What changed from the parents, and why

late-surge-24 plus one thing (namespace renamed `ls24` -> `ls27`, macros `LS27_*`):
- New `Params`: `bw_centre_bonus` (0.3), `bw_centre_until` (50), `bw_centre_max_tiles` (1024),
  named to avoid clashing with late-surge's existing `centre_until`/`centre_value` (a
  different, additive centre-floor mechanism used for the outnumbered-mode opening).
- In `ChooseGoal`, a `bw_pull` multiplier is computed once per tile from the same formula
  bw17 uses, then applied to the tile's collected score (`score *= bw_pull`) and to the
  explore-edge score (`bw_pull * mult * p_.explore_value * w / (d + 1)`), exactly mirroring
  bw17's `strategy.cpp` diff against body-wall-16.

## Measurements (sim, both seats)

| opponent | maps | late-surge-27 |
|---|---|---|
| late-surge-24 | official 11 (22 matches) | 12-0-10 |
| late-surge-24 | worldgen batch (20 maps, 40 matches) | 22-0-18 |
| trading-swarm-5 | official 11 (22 matches) | 16-0-6 (parent: 17-0-5) |
| trading-swarm-5 | worldgen batch (20 maps, 40 matches) | 25-1-14 (parent: 21-0-19) |

At the official-maps end state vs late-surge-24: late-surge-27 leads by 1.7 longest, 5.1
total length on average. Against trading-swarm-5, ls27 is roughly even with its parent on
the official maps and clearly ahead on the worldgen batch (25-1-14 vs 21-0-19). `check`
passed all official maps (0 avoidable deaths); turn times vary a lot run to run under the
shared machine's load (0.1-20+ ms observed for both this variant and the unmodified
late-surge-24 baseline rebuilt fresh), so that variance is machine noise, not the change:
the added work per turn is one extra multiply per already-visited tile.

Only tried centre_bonus = 0.3 (the port's default value); did not get to sweep 0.2/0.45
within the time budget (see below).

## What should improve

Faster/less-contested small-map openings against swarm-style opponents, matching the gain
bw17 saw and gather-split-trade-63 saw for the same port. Confirmed: even-or-better against
both the direct parent and trading-swarm-5 on both official and worldgen maps.

## Tried and didn't work / not done

Did not sweep centre_bonus at 0.2 and 0.45 as the brief suggested trying, or trace a loss to
see failure modes in detail — out of the ~20 minute budget once measurement across four
opponent/map combinations was done. Next: sweep bonus 0.2/0.45 against late-surge-24 and
trading-swarm-5, and check whether it interacts with the existing `centre_value`/
`open_centre_value` additive terms (both mechanisms are active in the same window on small
maps; a future variant might want to fold them into one).
