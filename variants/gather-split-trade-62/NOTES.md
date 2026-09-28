# gather-split-trade-62

## Idea

gather-split-trade-56 plus body-wall-15's "champion dead-end skip": from `champ_freeze`
(r330) the dragon that believes it is the team champion never takes a dead-end goal. A
champion that walks into a pocket later trap-splits out of it: the child walks off with the
mass under a new id, the stub keeping the champion's id dies, and the team feeds a ghost.

## What changed from the parents, and why

- Namespace `gst56` -> `gst62`, macros `GST56_*` -> `GST62_*`.
- New param `champ_no_trap = 1` (0 under `GST62_OFF`/`GST62_BASE`): in `ChooseGoal`,
  `no_trap` is also true for the frozen champion, so dead-end candidates are skipped
  (same line as body-wall-16's).
- Probe `GST62_CDBG` (compiled out; run sim with `--threads 1`): per match, how many of our
  dragons name a champion id that is alive at r380/r450, and the longest dragon's share of
  team length at r499.

Comparison of body-wall-16 and gst56 champion code: `Champion()`, `Note()`, `Decode()`, the
report params (late_report_from/memory/fade, lower_bound_reports), obituaries (including the
alternate-round obituary broadcast), the feed rules (feed_from_round 380, leader_feed_*,
feed_clear_tenths 8, feed_distance 3) and `champ_freeze` are the same in both lines. The
differences that touch consolidation are `champ_no_trap` (ported here), `cross_min_length`
(a portal length gate: not ported), threat pings on half the sonar directions, and the wall
logic, which kills enemies rather than consolidating. Their stale-id handling is the same, so
the body-wall edge in league-3 is not in Champion()/Note().

## Measured

`sim --threads 2`, both seats. practice/practice2 are two fresh batches of 20 worldgen maps.

| opponent | maps | W-D-L | longest | total |
|---|---|---|---|---|
| gather-split-trade-56 | official (22) | 13-0-9 | +1.4 | +8.3 |
| gather-split-trade-56 | worldgen A (40) | 24-0-16 | +2.6 | -15.2 |
| gather-split-trade-56 | worldgen B (40) | 19-0-21 | +3.0 | -15.2 |
| body-wall-16 | official (22) | 12-0-10 | +0.8 | +26.2 |
| body-wall-16 | worldgen A (40) | 21-0-19 | +0.2 | +14.9 |
| body-wall-16 | worldgen B (40) | 18-0-22 | +1.7 | -1.9 |

Probe vs gst56 (our dragons naming a live champion; longest share of team length at r499),
with the change (base = champ_no_trap 0):

| maps | r380 live | r450 live | r499 longest share |
|---|---|---|---|
| official | 56% (base 57%) | 48% (base 49%) | 0.60 (base 0.51) |
| worldgen A | 63% (base 56%) | 56% (base 47%) | 0.32 (base 0.33) |
| worldgen B | 67% (base 67%) | 89% (base 59%) | 0.33 (base 0.28) |

## What should improve

The endgame champion keeps its id alive more often, so more feeders at r450 walk to a live
dragon and the longest margin goes up (+1.4 to +3.0 against the parent on every map set).

## Open problems

Even with the change, a third to half of our dragons name a dead champion at r380, and on
worldgen the longest dragon ends with only about a third of team length. The ghost problem is
not only trap-splits by the champion: other causes (champion killed head-to-head, reports
fading slowly with late_report_fade 0.25, obituaries only from adjacent witnesses) are still
there. Next: log why the named champion died (trap split, head-to-head, lunge) at the
moment its id goes stale, then target the biggest cause.
