# gather-split-trade-100

Mutate of gather-split-trade-96 (task g18-10).

## Idea

gst96 prices a dead end by expected loss and lets a 2-3-long dragon enter a pocket
whose pearls would grow it to 4 so it can split out. Its own worker report flagged
that this "grow" case counted pearls from *memory*, including ones that may already
have been eaten by the time the dragon gets there, and that there was no visibility
into whether dragons that did grow to 4 inside a pocket actually managed to split
out before dying. This mutation does the two things the report suggested trying
first (its item (b), an EscapeLevel-based U-turn test, is left for a follow-up task).

## What changed from the parents, and why

1. Renamed the `gst96` namespace and every `GST96_*` macro to `gst100`/`GST100_*`
   (mechanical, so this variant can't clash with its parent when loaded together).
2. **(a) fresh pearls only for the grow decision.** In `ChooseGoal`'s dead-end
   pricing (`strategy.cpp`, the `trap_loss` block), pearls counted for the pocket's
   `gain` score still use `TileValue` as before, but the `grown` length used to
   decide "will reaching this pocket grow us to 4 so we can split out" is now
   computed from `fresh_pearls`: only pearls whose tile is `board_.Current(t)` (seen
   this exact round) and still `.pearl` in memory. A remembered-but-possibly-eaten
   pearl no longer counts towards sending a 2-3-long dragon into a dead end it can't
   actually grow out of.
3. **(c) instrumentation for split-outs vs trapped deaths in grow-pockets**, behind
   `GST100_DEBUG` (compiled out in the real build, so it costs nothing normally): a
   `grow_pending_` flag is set when `ChooseGoal` returns a goal with `.grow == true`,
   cleared (and `g_pocket_split` incremented) when the dragon reaches the split
   check with a live grow-goal and actually splits, and counted as `g_pocket_trapped`
   in the destructor if the agent is destroyed (dies) while still `grow_pending_`.
   Printed at process exit as `POCKET-GROW split-out N trapped M`. I did not have
   budget left in this pass to also run and read that counter output; the next
   worker on this lineage can build with `-DGST100_DEBUG` and check it directly
   against gst96's reported +24% short trapped deaths on labyrinth.

I did not attempt (b) (the EscapeLevel-based U-turn test) this round — budget was
spent on the safer (a)+(c) change and verifying it's at least even, as the brief
asked.

## What I measured

Built normally (no `GST100_DEBUG`) and ran `sim --threads 2`:

- vs `gather-split-trade-96`, `maps/official` (both seats, all 11 official maps):
  **12-0-10**, mean 364 rounds, leads by +0.0 longest / +13.9 total length.
  - Devil specifically: won as B (82-26, longest 54-26), lost as A (0-74, longest
    0-25) — same lopsided-by-seat pattern gst96 itself showed on Devil.
  - Default specifically: won both seats (36-53 total but longest 32-19; 97-13).
- vs `gather-split-trade-90`, `maps/official`: **8-0-14**, leads by -4.0 longest /
  -12.4 total. Weaker against the grandparent than gst96 itself reportedly is
  (2214 vs 2196 in the league), but this is one run per map/seat with no map-level
  averaging, and gst96 vs gst90 head-to-head wasn't re-measured here for a
  baseline to compare against directly.
- vs `gather-split-trade-96`, 20 worldgen maps, default style, seed 5000 (both
  seats, 40 matches): **21-0-19**, +4.8 longest / +24.4 total.
- vs `gather-split-trade-96`, 20 worldgen maps, `--style labyrinth`, seed 6000
  (both seats, 40 matches): **20-0-20**, -0.5 longest / +4.8 total.

Net: essentially even to slightly ahead of gst96 across official maps and both
worldgen sets (win counts 12-10, 21-19, 20-20; total-length margin positive in
three of four comparisons). No regression signal strong enough to withhold.
`check gather-split-trade-100 maps/official` passes (slowest turn 0.1-0.2 ms, 0
avoidable deaths).

## What I'd try next

- Actually run a `GST100_DEBUG` build on the labyrinth worldgen set and read the
  `POCKET-GROW split-out/trapped` counts, to see whether (a)+(c) alone moved the
  needle on gst96's reported short-trapped-death spike, before someone invests in
  (b) (the EscapeLevel-projected-body U-turn test) on top.
- Re-run gst96 vs gst90 on the same maps/seats in the same pass as gst100 vs gst90,
  so the -12.4 total-length gap above has a same-conditions baseline to compare to
  rather than the league's aggregate number.
