# ring-field-spawngate (g50-2, arena ring-field-11, parent ring-field-3)

## Idea
eatrate-r24 / strategist-g50 #1: on rich starts the r0 "spawn field" flag (4+ beds live or due within
30 in view) keeps a whole spawn group at home, so it never takes the field opening. A field feeds only
as many dragons as it has beds.

## Change (field.cpp, r0 gate only)
A non-queen r0 dragon stays home as "spawn field" only if beds in view >= max(4, 2 x our heads in its
7x7 window, self included). 2 per head: a 2-long dragon needs 2 pearls to reach its growth split at 4
(the kHomeFood reason). Otherwise it enters field mode as before. The queen-child and home-keeper rules
are unchanged.

## Where it fires (FIELD_DBG, local, seed 1)
- wg39001: every starter has 4-5 heads and 8-9 beds. Before: all "spawn field". Now: all field mode.
- It also fires on wg39004, wg39011 (one dragon) and fixed-crowded.
- No firing on arena, the server maps or the official maps. On arena the starters don't reach this gate
  (no r0 gate line at all), so the brief's "arena starters leave by r3-5" isn't addressed here.

## Local (vs ring-field-1)
- Server maps, seed 1: 17-0-17. Every game mirrored the base.
- Fresh mapgen, seed 50207, 44 maps: seeds 1+2 88-2-86, seed 3 45-0-43.
  Per family (seeds 1+2): crowded 9-0-7, maze 7-2-7, all others 8-0-8 (identical to the base).

## Arena
- Duels v ring-field-3 (ephemeral eph-10f7d5cc): server 34-0-34, official 10-0-10, both with identical
  metrics. Rotating (ring-field-11) 60-0-60: dragons@100 37.19 v 36.87, otherwise the same.
- Verdict, ring-field-11 v parent: 56-0-56, within noise. Server 34-0-34, custom 17-0-17, official 5-0-5.
  All 336 games against the other opponents match ring-field-3's games on the same seeds and seats, so
  the change never fired in the verdict suites.
- CPU: p99 24.7M, max 42.0M.

## Next
The gate matters only on generated rich maps with a crowded spawn group. The arena losses the analysis
blamed (arena, stripes, mg39500) come from somewhere else; on arena the starters don't reach this gate.
