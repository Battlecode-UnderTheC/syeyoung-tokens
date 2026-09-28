# gather-split-trade-55

## Idea

Crossover: gather-split-trade-51 (base, league #1) + gather-split-trade-53's unit-cap relief.
Replay pattern #18 (`late-surge-18`, 4,196 length lost over 16 games after r150) is length
trapped at the unit cap. gst53 showed two of its three fixes do the work on gst50; port the
same two onto gst51.

## What changed from the parents, and why

Parent gather-split-trade-51 (namespace gst51 -> gst55). Ported from gst53 (itself gst50 ->
gst53), as two new Params:

1. `late_split_free = 3`: from `champ_freeze` (330), stop growth splits once <= 3 slots are
   free. This is most of gst53's effect (supply-side: fewer new dragons competing for slots
   just as the cap starts to bind).
2. Cap-feed (`cap_feed_max 3, cap_feed_ally 10, cap_feed_dist 2`): at the cap (0 free slots), a
   worker of length <= 3 within 2 steps of an in-view ally of length >= 10 resigns beside it,
   freeing a slot and leaving pearls. Inserted as step 1b, right after the existing "feed
   champion" resign check, using the `paths`/`seen_` already computed there.

Dropped gst53's third change (`goal_room_cap = 16`): gst53's own report found it did nothing
measurable alone, and the brief said to drop it.

Not attempted (budget): the worker's suggested wider parameters (late-split stop at 5-6 free
slots or from r300; cap-feed at 1 free slot or workers up to length 5). Left as follow-up.

## Measurements

vs gather-split-trade-51, both seats, `--threads 2`:

- Official maps (`maps/official`, 11 maps x 2 seats = 22 matches): 11-0-11 W-D-L, -0.1 longest,
  -19.7 total. As expected, official maps are almost entirely seat-decided (win/loss flips
  identically when seats swap on every map), so this set doesn't discriminate the change.
- Worldgen (20 fresh maps x 2 seats = 40 matches): **22-0-18 W-D-L**, **+3.6 longest, +2.1
  total** margin at gst55's favor. Cap-related trapped deaths: `trapped: no move L10+ @cap` 6
  (gst55) vs 8 (gst51); `cap-feed` fired 7 times across the 40 matches. Directionally the same
  win-rate-barely-moves, length-slightly-better pattern gst53 found on gst50.

I did not run a second independent worldgen batch given the time budget; the one batch shows a
small positive margin and no regression, which meets the "at least even" bar.

## What should improve

A few more long dragons surviving to the end on crowded worldgen maps late in the match (less
smothering by our own bodies at the cap), and a small total/longest-length edge. Win rate is
expected to move little in self-play since per-map outcome is still mostly seat-decided.

## What didn't work / notes

Nothing new tried beyond the direct port; see "Not attempted" above for what's left for a
follow-up task (tuning the two thresholds, and checking whether the saved length specifically
reaches the round-499 champion rather than just avoiding waste elsewhere).
