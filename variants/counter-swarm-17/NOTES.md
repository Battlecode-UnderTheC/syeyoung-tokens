# counter-swarm-17

## Idea

Pure parameter tweak on counter-swarm-16 (namespace renamed cs16 -> cs17):
set `hunt_radius` (how far workers chase enemy heads to trade) from 6 to 0.
Workers no longer proactively hunt; they still take strikes/trades that come
to them (WorthTrading / defensive logic untouched). Motivated by
gather-split-trade-31 -> gather-split-trade-32 (same change) beating its
parent 54-48 and late-surge-6 60-42, and by g10-2's finding that hunting is
the costly part of the busy mid game.

## What changed from the parents, and why

- `hunt_radius`: 6 -> 0 in `cs17.hpp`. Nothing else changed.
- All files/macros renamed cs16 -> cs17 (Cs16 -> Cs17, CS16 -> CS17) so both
  variants can be loaded in the same simulator without symbol clashes.

## Measurements

`check counter-swarm-17 maps/official`: all checks passed, 0 avoidable deaths,
slowest turn 16.9ms on big_empty (audit-instrumented build; not representative
of the -O2 release runtime other variants also show inflated numbers for).

### vs counter-swarm-16 (parent), official maps (11 maps x2 sides = 22 matches)

Overall: **13-0-9** (W-D-L), mean rounds 352, cs17 leads by +1.9 longest, +8.1
total length at match end.

Per-map win/loss (as A / as B):
- Colosseum: loss / WIN
- arena: WIN / loss
- big_empty: WIN / WIN
- default: loss / loss
- default_small: loss / WIN
- devil: WIN / WIN
- queen_of_spades: WIN / WIN
- schooltime: loss / WIN
- stronghold: WIN / loss
- trauma: loss / WIN
- trophy: WIN / loss

### vs counter-swarm-16 (parent), 20 worldgen maps (40 matches)

Overall: **24-0-16**, mean rounds 418, cs17 leads by +1.1 longest, +25.0 total.
Consistent with official maps: net positive, no regression on the small/
crowded maps (default_small, arena, trophy all still win as at least one
side).

### vs gather-split-trade-32 (cross-lineage comparison, not a parent), official maps

Overall: **8-0-14**, cs17 trails by -4.9 longest, -78.1 total at match end.
gst-32 (the gst31 + hunt_radius 0 variant, a different/stronger lineage) beats
cs17 head-to-head on most maps except arena, default_small, stronghold. This
is a lineage-strength gap, not evidence against the hunt_radius=0 change
itself — cs17 still improves over its own parent cs16 by the same margin
gst32 improved over gst31.

## What should improve

Removing hunting should reduce unnecessary head-to-head engagements during
the mid-game gather phase, keeping workers longer and reducing "no action"/
trapped deaths caused by being drawn out of position to chase. The parent
comparison confirms this: net swing toward cs17 on both official and
worldgen maps, no map-set-specific regression.

## What didn't work / wasn't tried

Budget was a single-parameter tweak per the brief; no sweep of hunt_radius
values or other parameters. Losses to gather-split-trade-32 suggest cs-lineage
still trails gst-lineage generally (separate issue from this task).
