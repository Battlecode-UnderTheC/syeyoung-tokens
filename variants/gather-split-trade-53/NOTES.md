# gather-split-trade-53

## Idea

Supply-side relief at the unit cap (replay pattern #18): long dragons late in the game resign
trapped in mazes of our own bodies at 64 units. Fewer idle slots late, and short workers that
free a slot next to a long ally, instead of more caution on the long dragon's side.

## What changed from the parents, and why

Parent gather-split-trade-50 (namespace gst50 -> gst53). Three changes, each a Param:

1. `late_split_free = 3`: from `champ_freeze` (330), no growth split once <= 3 slots are free.
2. Cap-feed (`cap_feed_max 3, cap_feed_ally 10, cap_feed_dist 2`): at the cap (0 free slots), a
   worker of length <= 3 whose path reaches a tile beside the head of an in-view ally of length >= 10
   within 2 steps resigns ("cap-feed"): frees a slot, leaves pearls by the ally.
3. `goal_room_cap = 16`: ChooseGoal asks at most 16 tiles of room around a target (was length+3),
   so a long champion doesn't drop every candidate.

Debug counter `GST53_CAPLOSS` (compiled out) tallies trapped-no-move resignations of length >= 10
at <= 1 free slot.

## Measurements (vs gather-split-trade-50, both seats)

Trapped-at-cap resignations of L10+ (gst53 side only, 40 matches per set), and W-D-L:

| config                         | set A: n / length | W-D-L  | set B: n / length | W-D-L  |
|--------------------------------|-------------------|--------|-------------------|--------|
| all off (= parent)             | 426 / 5720        | 20-0-20| 203 / 2775        | 20-0-20|
| late split stop only           | 336 / 4513        | 20-0-20| 152 / 2102        | 18-0-22|
| cap-feed only                  | 408 / 5286        | 19-0-21| 140 / 2061        | 20-0-20|
| goal room cap only             | 424 / 5665        | 20-0-20| 201 / 2732        | 19-0-21|
| all on (submitted)             | 331 / 4359        | 20-0-20| 109 / 1624        | 19-0-21|

All on: length lost trapped at the cap -30% (8495 -> 5983), deaths 629 -> 440. Margins: set A
+2.2 longest / +22.7 total, set B -0.5 / -4.1. Official maps: 11-0-11, -0.5 longest, +7.0 total
(nearly all seat-decided). cap-feed fires ~6-8 times a match on worldgen.

## What should improve

Fewer long dragons smothered late; slightly longer champion on crowded maps. Wins barely move in
self-play: the per-map outcome is mostly decided by seat, so the length saved rarely flips a result.

## What didn't work / notes

The goal room cap alone does nothing measurable (the ChooseGoal drop is rare). The supply-side
change (stop late splits) does most of the work; cap-feed adds on top.
