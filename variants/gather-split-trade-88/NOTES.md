# gather-split-trade-88

## Idea

Mutate of gather-split-trade-84. league-6 pattern #34: all three losses were eliminations
during the feeding phase (r380+) -- once 32 of our dragons fed themselves into a single
champion (29 -> 1 dragons), leaving that sole survivor to lose a lunge trade 28 rounds before
it would otherwise have won on length; twice 12-13 dragons fed a champion only 4-11 long while
outnumbered 13 v 17-21, and the team was wiped at r417/r418. Feeding was unconditional past
`feed_from_round`/`leader_feed_from` regardless of how many dragons it cost the team or how
short the champion being fed was.

## What changed from the parents, and why

Namespace renamed `gst84` -> `gst88`, macros `GST84_*` -> `GST88_*` (required: avoids clashing
with the parent when both are loaded into one simulator).

In `Decide()`, gated the feeding branch with a new `feed_afford` check before a dragon is
allowed to resign into the champion:

- `units_ > max(feed_keep_units, EnemiesSeen())` -- new params `feed_keep_units = 6`: never feed
  down to fewer than 6 dragons; and reuse the existing `EnemiesSeen()` (enemies seen/heard within
  `enemy_memory` rounds) so that when the team is outnumbered late it keeps gathering/trading
  instead of feeding itself away.
- `champ.length >= feed_min_champ` -- new param `feed_min_champ = 12`: don't feed a champion that
  isn't even worth 12 pearls' worth of length; small champions aren't worth the sacrifice.

Both conditions are `&&`ed into the existing `feeding` bool, so every other feeding rule
(distance, clearance margin, merge-round override, leader freshness) is unchanged -- this only
adds a global "can we afford it at all" gate in front of them.

Did not implement the optional sole-survivor lunge-avoidance (from ~r460, dodge enemy heads
within their lunge reach): ran out of budget to add and separately validate it; left for a
follow-up task. The `feed_keep_units`/`feed_min_champ` values were not swept beyond the
defaults suggested in the brief -- also left for a follow-up if the numbers need tuning.

## What I measured

`build/tools/check gather-split-trade-88 maps/official`: all checks passed, 0 avoidable
deaths, slowest turn 1.7ms (stronghold; pathing-search heavy map, same order of magnitude as
other gst variants, well under the 100M CPU-point budget).

`sim`, 2 threads, gather-split-trade-88 vs each opponent, both seats:

- vs gather-split-trade-84 (parent), official maps: 10-0-12 (22 matches), mean margin -8.4
  longest / +25.9 total.
- vs gather-split-trade-84 (parent), 40 worldgen maps (2 batches of 20): 22-0-18 each batch
  (44.0% -> combined 44-0-36 across both, since the two `--seed` batches produced the same
  20 maps -- worldgen's `--seed` did not vary map content run to run here, worth noting for
  future workers), mean margin roughly even (-0.1 to -0.6 longest, +57 to +60 total).
- vs trading-swarm-1, official maps: 20-1-1 (dominant, unchanged behaviour vs this opponent).
- vs trading-swarm-5, official maps: 11-1-10, roughly even (+2.8 total margin).
- vs body-wall-24, official maps: 9-0-13, mean margin -2.0 longest / +48.7 total.

Combined vs the parent across official + worldgen: 32-0-30 -- essentially even, with a
positive total-length margin, satisfying "at least even with the parent."

`feed champion` fatal-turn counts (a proxy for the pattern this task targets) roughly halved
for gst88 relative to gst84 in the same matches (worldgen: 28.3 -> 11.8 average fatal feeds;
official vs the parent: 13.9 -> 2.7), consistent with the intended fix: the team feeds less
often, and specifically less often in a way that turns out to be its last action.

## What I'd try next

- Sweep `feed_keep_units`/`feed_min_champ` (the brief suggested trying a few values); I only
  used the suggested defaults given the time budget.
- Add the optional sole-survivor lunge-avoidance from ~r460 (avoid enemy heads within their
  lunge reach when we are the last dragon) -- the r472 loss in the study was exactly this
  case and this change doesn't address it directly, only the earlier bulk-feeding cases.
- Look at whether worldgen's `--seed` should vary per batch (both my batches produced
  identical maps), which limited how much fresh-map coverage this measurement actually got.
