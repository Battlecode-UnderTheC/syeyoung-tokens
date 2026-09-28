# gather-split-trade-90

## Idea

gst84's biggest non-cap length sink (league-6 replay study, 276 length/game) is trapped
deaths of short stubs, not at the unit cap: 10-14 short dragons pile into one small
bed-filled room and wall each other in (VISUAL.md pattern #35; against trading-swarm-5,
520/520 trapped deaths were walled in by our own or ally bodies, never an enemy). This
variant makes both the goal picker and the growth-split rule crowd-aware for the whole
game (not just the opening fan-out window), so dragons spread out over beds/pearls
instead of scrumming, and don't split new children directly into an existing crowd.

## What changed from the parents, and why

- Renamed namespace `gst84` -> `gst90` and every `GST84_*` macro -> `GST90_*` (required
  so this variant can be loaded in the same simulator as its parent without clashing).
- `ChooseGoal`: added a crowd discount on bed/pearl candidates (not bare explore tiles).
  Reused the ally-head-in-view list that fan-out already builds for the opening
  (`fan_out_until`), but built it unconditionally (`crowd_allies`) so it keeps working
  after round 80. Any bed/pearl tile within `crowd_discount_radius` (3, torus distance)
  of an ally head in view has its score multiplied by `crowd_discount_factor` (0.55) per
  ally head nearby, so a tile already claimed by 2-3 allies scores a fraction of what an
  uncontested one does and a dragon looks elsewhere.
- `Decide`'s growth split (step 4): now also requires fewer than `split_crowd_limit` (3)
  ally heads within `crowd_discount_radius` (Manhattan distance, matching the style of
  the nearby give-up-slot check) of our own head before splitting, so a new child isn't
  born straight into the same crowd its parent is trying to avoid.
- New params in `Params`: `crowd_discount_radius = 3`, `crowd_discount_factor = 0.55`,
  `split_crowd_limit = 3`. No sweep done given the 25-minute budget; these are the
  brief's literal numbers (3 tiles, 3+ heads).

## What I measured

- `check gather-split-trade-90 maps/official`: all checks passed, slowest turn 0.5 ms,
  0 avoidable deaths.
- `sim --a gather-split-trade-90 --b gather-split-trade-84 --maps maps/official
  --threads 2`: 14-0-8 (W-D-L) over 22 matches; at the end gst90 leads by 1.1 longest,
  but trails by 5.6 total length. Non-cap `trapped: no move L2-3` actions/fatal-turns
  went up slightly on this small (11-map, 22-match) sample (56.0 vs 42.5) — noisy given
  the sample size, and offset by a clear W-D-L lead.
- `sim --a gather-split-trade-90 --b gather-split-trade-84 --maps <20 worldgen maps>
  --threads 2`: 29-0-11 (W-D-L) over 40 matches; at the end gst90 leads by 6.3 longest
  and +47.3 total length. This is the larger, more trustworthy sample and shows a clear
  overall improvement, though the specific `trapped: no move L2-3` (non-cap) bucket was
  also marginally up here (228 vs 210) rather than down.
- `sim --a gather-split-trade-90 --b trading-swarm-5 --maps maps/official --threads 2`:
  14-1-7 (W-D-L), leads by 0.4 longest, trails by 23.6 total length.

## What should improve, and what didn't land as expected

Overall W-D-L and length-margin against the parent both improved clearly on the larger
worldgen sample (and were still a clear net win on official maps), so the crowd
discount + split gating are a net positive even though I did not see the drop in the
specific non-cap short-stub trapped-death count the brief's diagnosis targeted — that
count was flat-to-slightly-up on both map sets. Two candidate reasons, worth checking
next: (1) the discount is Manhattan-distance-cheap-lookup based on ally heads *seen this
turn*, which misses allies just out of sonar view that are still physically in the same
room; a room-membership check (shared dead-end pocket, from `pathing`'s dead-end-room
helper) might target the actual pattern-#35 rooms better than a flat radius. (2) gating
the split on heads near our *own* head doesn't stop us walking a goal step into an
already-crowded room even after discounting it — the discount lowers a crowded tile's
score, but if every candidate in view is inside the same crowded room it can still win
by default. Next task could try scoring "no reachable tile at all above a floor" as a
trigger to widen the search radius instead of taking the best of a bad set.

## Ready to submit

Yes: `check` passes on `maps/official`, and gst90 is ahead of gather-split-trade-84 in
both W-D-L and length margin on the larger (worldgen) sample, and ahead of
trading-swarm-5 on official maps.
