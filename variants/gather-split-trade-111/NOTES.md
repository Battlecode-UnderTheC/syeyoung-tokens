# gather-split-trade-111

## Idea

g21-1: strategist-e's P1 (reactive consolidation). Parent gst107 starts feeding the
champion and freezing its splits on a fixed clock (`feed_from_round`=380,
`champ_freeze`=330). strategist-e-r2.md's diagnosis: the opponent consolidates on a
*signal* (we're outnumbering it, or its longest stopped growing) while we consolidate
on a *timer* regardless of state, which loses both openings (we're still splitting
small while behind on count) and endgames (Schooltime: we out-eat on total but lose on
longest because we're still splitting at r380-400).

## What changed from the parents, and why

Namespace/macros renamed `gst107`/`GST107_*` -> `gst111`/`GST111_*` (required so it
doesn't clash when linked alongside the parent).

Added `Params::reactive_min_round` (300), `reactive_count_margin` (4),
`reactive_longest_margin` (6), and a new `StrategyAgent::Consolidate()` that returns
true once `round_ >= feed_from_round` (unchanged backstop) **or**, from
`reactive_min_round` on, either of:
- `EnemiesSeen() >= units_ + reactive_count_margin` (recent-sightings estimate of the
  opponent's count has pulled ahead of ours), or
- the longest enemy sighted within `enemy_memory` rounds is `>= reactive_longest_margin`
  longer than our own `Champion()` (their longest has already overtaken ours).

Every call site that gated on `round_ >= p_.feed_from_round` (feeding, the champion's
"stop splitting to gather for itself" flag, the lunge/hunt exemptions for the
champion) and both call sites that gated on `round_ >= p_.champ_freeze` (dead-end
avoidance, the growth-split guard) now call `Consolidate()` instead, so feeding and
champion-freeze fire together, from whichever signal trips first, on all the same
decisions the old fixed rounds gated. `feed_from_round` and `champ_freeze` stay as
hard backstops (always fire by their round) since P1 argues for firing *earlier*, not
for removing the ceiling.

Kept the correction from the brief in mind (opponent's minimum split length is 4, not
2-3, per trading-swarm-8's NOTES.md) — didn't touch `split_min_length` (already 4 in
gst107) or otherwise assume a lower opponent split threshold.

I did not wire the two new params into the `GST111_TUNE`-only `GST111_SET` env-var
table (a debug-build-only tuning knob); it's not needed for check/sim/submission and
out of scope for the one-change budget.

## What I measured

`sim`, 2 threads, W-D-L / longest+total margin at the end, dragons/longest/total at
r50 and r100 (both seats each map/opponent). Worldgen: fresh seed 90000, batch 20
(not one of the excluded ranges).

**vs gather-split-trade-107 (parent)**
- official (11 maps, 22 matches): 11-0-11, margin at end +0.6 longest / -6.0 total.
  r50: 10.2 vs 10.2 dragons, 3.4 vs 3.4 longest, 25.1 vs 25.1 total (identical opening,
  as expected — the opening is untouched). r100: 21.2 vs 21.2 dragons, 3.8 vs 3.8
  longest, 54.4 vs 54.4 total.
- worldgen seed 90000 (20 maps, 40 matches): 17-0-23, margin -3.2 longest / -5.2 total.

**vs gather-split-trade-96 (live v7)**
- official: 11-0-11, +0.6 longest / +13.4 total (an improvement over gst107's own
  11-0-11 / -0.1 / -6.0 baseline against gst96 on the same maps).
- worldgen seed 90000: 17-0-23, -3.2 longest, comparable to gst107's own 19-0-21 vs
  gst96 baseline on the identical 40 matches (so this isn't a regression introduced by
  my change so much as an existing gst107-vs-gst96 worldgen gap gst111 inherits).

**vs trading-swarm-8 (real-opponent clone)**
- official: 10-1-11, +0.4 longest / +5.0 total.
- worldgen seed 90000: 29-0-11, +8.2 longest / +64.7 total — the strongest result of
  the three, on the opponent most like what we actually face.

## What should improve

Games like M578651/M547324 (strategist-e's #2/#3): still splitting near r380-400,
losing the longest race despite a total-length lead. `Consolidate()` should trip
earlier there once the opponent's longest overtakes ours (or once its sighted count
outnumbers ours) rather than waiting for r380 — the `feed champion` action count rose
(gst107: 9-11, gst111: 16 on official; 28 vs 34-35 on worldgen), consistent with the
trigger firing before the old timer on some fraction of games.

## What didn't work / surprises

- An earlier, more aggressive setting (`reactive_min_round=250`,
  `count_margin=2`, `longest_margin=3`) fired too readily: worse both vs parent
  (official 10-0-12) and on worldgen, and vs gst96 worldgen (14-0-26, clearly below
  gst107's own 19-0-21 baseline there). Backed off to 300/4/6, which is even vs parent
  on official and matches (not beats) the pre-existing gst107-vs-gst96 worldgen gap.
- Dropping the longest-margin leg entirely (count-only trigger) didn't fix the
  worldgen deficit vs parent/gst96 either (still 17-23 / 17-23) — the regression there
  isn't obviously attributable to one leg over the other; I didn't have budget to
  trace it via `--trace` on a specific losing worldgen map.
- The opening (r50/r100 dragon/longest/total) is bit-identical to the parent in every
  measurement above, confirming the change only affects consolidation-phase decisions
  as intended — no accidental interaction with growth/exploration.

## Next

- `--trace` a losing worldgen match against gst107 to see whether `Consolidate()`
  fires at a bad moment (e.g. a transient sighting spike triggers it, then the signal
  goes away and we've already frozen/fed prematurely) — a hysteresis (require the
  signal to hold for a few consecutive rounds) might fix that without touching the
  thresholds again.
- Try gating the *count* leg and the *longest* leg on different `reactive_min_round`s:
  the longest-overtook-us signal (diagnosis for M547324/M578651) is probably safe
  earlier than the count-outnumbered signal (diagnosis for M561837), which needs more
  settled sightings to trust.
