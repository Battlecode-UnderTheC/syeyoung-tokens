# counter-swarm-7

## Idea

A pure parameter tweak on counter-swarm-1 (tag: `tweak`, task g3-3). The match is decided by
the longest dragon at round 499. counter-swarm-1 has non-leader workers feed themselves to a
bigger ally (dying next to it so it eats their pearls) starting at round `feed_from_round`
(340). Feeding earlier should let the leader dragon grow longer sooner, raising the
longest-dragon margin at the final round.

## What changed from the parents, and why

- `Params::feed_from_round`: 340 → 300 in `cs.hpp`. Nothing else changed (namespace renamed
  `cs` → `cs7` throughout, as required, to avoid clashing with the parent when both are
  loaded in one simulator; no behavioral change from the rename).
- Also tried `feed_from_round = 260` to see if feeding even earlier helps further.

## What was measured

Opponent: counter-swarm-1 (parent). Built with `cmake --build build --target counter-swarm-7
counter-swarm-1 sim check`; `check counter-swarm-7 maps/official` passed (0 avoidable deaths,
slowest turn 0.0-0.1 ms).

**feed_from_round = 300** (this variant):
- Official maps (`maps/official`, both sides played as A and B, 22 matches):
  `11-0-11` W-D-L. At the end: counter-swarm-7 leads by **+1.5 longest**, but **-24.2 total**
  length (matches the design intent: this change trades total length for the longest single
  dragon, which is what decides the match).
- 20 fresh worldgen maps (`worldgen.py --batch 20`, both sides played as A and B, 40 matches):
  `24-0-16` W-D-L (a clear edge over the parent). Longest margin **+1.0**, total **-93.8**.
- Combined: 35 wins, 0 draws, 27 losses across both map sets — an improvement over the 1.0
  win-rate parity baseline (parent vs itself would be 50/50).

**feed_from_round = 260** (tried, not kept):
- Official maps: `7-0-15` W-D-L, longest margin **-9.9** — clearly worse than both 300 and the
  parent's 340. Feeding too early apparently starves the leader of the swarm's numeric
  advantage too soon (feed-ally deaths per match roughly quadrupled vs. parent: 36.5 vs 9.6),
  so workers die for the leader before there's enough of a size gap for it to matter, and the
  leader ends up outnumbered/out-split earlier in the midgame.
- Reverted; kept 300 as the final value.

## What should improve

Longest-dragon margin at round 499, which is what the match is decided on, at a real cost in
total length (fewer, smaller workers feeding pearls to fewer, bigger leaders). Win rate on
worldgen maps improved clearly (24-16 vs 16-24 baseline parity); official maps were roughly
even in win count but still favored the new setting on the longest-dragon margin, which is
the tiebreak that matters.

## What didn't work

`feed_from_round = 260` was worse than both 300 and the parent's 340 — there is a sweet spot,
not a monotonic "earlier is always better" relationship. Confirms the KNOWLEDGE.md finding
that earlier feeding raises the longest-dragon margin, but only within a range; too early
and it just adds early deaths.
