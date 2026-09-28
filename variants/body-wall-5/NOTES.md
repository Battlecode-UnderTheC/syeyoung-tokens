# body-wall-5

## Idea

body-wall-4 plus gather-split-trade-27's cap safety. A SPLIT with no free unit slot kills the
splitter, so near the 64-dragon cap a long dragon can't split out of a dead end. With fewer than
`cap_margin` = 2 free slots, a dragon of length >= 4 plans like a length-2 dragon: no dead-end
goals, no "into a dead end worth it" steps, never below its room need.

## What changed from the parents, and why

- Namespace bw4 -> bw5.
- Ported the whole gst23 -> gst27 diff (it is exactly the cap safety): the params `cap_safety`, `cap_margin`,
  `reserve_slots` (0), `reserve_length` (6); `FreeSlots`, `CanSplitNow` and `CanCountOnSplit`;
  goal selection skips dead-end candidates when `!CanCountOnSplit()`; SafeStep turns trap_ok off
  and uses `can_split_out` in place of `length_ >= 4`; trapped/doomed splits need `CanSplitNow()`;
  the growth split uses `FreeSlots() > max(split_headroom, reserve_slots)`. I left out gst27's reason
  bucketing (the "L4-5 @cap" tags) so the reasons stay the same as body-wall-4's.
- Interaction with cap wallers: their chase goal returns before the dead-end filter, and `Wall()`
  checks its own escape (EscapeLevel >= 1), so walling at the cap is unchanged. Only their ordinary
  goal and step choices get safer. I didn't change this.

## Measured (sim, both sides of every map)

| opponent | maps | W-D-L |
|---|---|---|
| body-wall-4 | official (11 maps) | 11-0-11 (only schooltime differs; each side wins) |
| body-wall-4 | worldgen batch 20 (random seed) | 21-0-19 |
| body-wall-4 | worldgen batch 20 (seed 777) | 23-0-17 |
| gather-split-trade-31 | official | 10-0-12 (body-wall-4: 10-0-12) |
| gather-split-trade-31 | worldgen A | 20-0-20 (body-wall-4: 18-0-22) |
| gather-split-trade-31 | worldgen B | 21-0-19 (body-wall-4: 19-0-21) |

`check`: all checks passed. The slowest turns are the same as body-wall-4's on a loaded machine.

## What should improve

Fewer long dragons dying whole in pockets at the cap on crowded maps. The gain is small but
consistent: +8 on 80 worldgen matches against the parent, and +4 over the parent against gst31.

## Tried / not tried

I didn't sweep `cap_margin` or `reserve_slots`. A possible next step is to require room >= need
for `Wall()` steps at the cap, since the cap wallers are the long dragons most exposed there.
