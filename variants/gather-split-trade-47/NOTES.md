# gather-split-trade-47

## Idea

Feeders cut off from the champion walk toward its sonar-reported tile anyway. They use an
optimistic path where unseen edges are passable (cost 2), known portals are crossed, and portals
with an unknown far end are closed. If even that finds no path (a pocket closed by unknown
portals), the feeder goes to the nearest tile beside an unknown portal and crosses after a
clean ping.

## What changed from the parents, and why

Mutation of gather-split-trade-42 (namespace gst47, GST47_* macros).
- `SeekDir`: Dial's shortest path over known + optimistic-unknown edges to the champion's tile.
  The result (or the nearest unknown-portal tile) becomes the goal step when the normal BFS
  finds no path (`seek_cutoff`, `seek_unknown_cost` 2, `seek_from_round` 380).
- Only feeders of length >= 6 seek (`seek_min_length`). With every feeder seeking, the team
  lost 33-47 on worldgen (total margin about -55): ~1,200 turns a match went to walking instead
  of gathering, for ~4 more feeds a match. That matches KNOWLEDGE's "a walking feeder stops gathering".
- While seeking, the ping and crossing of unknown portals also works after round 480
  (`WillCross() || seeking_`). Reasons: `seek: unknown ground`, `seek: to portal`, `seek: portal`.

## Measurements (vs gather-split-trade-42, both seats)

Debug counters (compiled out) at r400/r470 count non-held feeders with no known path to the
champion's tile. Feeds counts feeder resignations next to the champion from r400 on, with their
summed length.

| maps | variant | r400 no-path | r470 no-path | feeds / length | W-D-L | longest / total margin |
|---|---|---|---|---|---|---|
| official | off | 242/333 | 94/106 | 148 / 736 | 11-0-11 | 0 / 0 |
| official | min 6 | 236/329 | 78/105 | 172 / 920 | 12-0-10 | +1.1 / -22.0 |
| worldgen A | all seek (min 1) | 1071/1381 | 299/388 | 706 / 3232 | 16-0-24 | -2.5 / -55.6 |
| worldgen A | min 6 | | | 567 / 2869 | 25-0-15 | +3.4 / -2.8 |
| worldgen B | off | 1285/1602 | 473/566 | 647 / 3229 | 20-0-20 | 0 / 0 |
| worldgen B | all seek | 1109/1493 | 348/477 | 692 / 3349 | 17-0-23 | |
| worldgen B | min 6 | | | 711 / 3628 | 20-0-20 | -4.3 / -26.3 |
| worldgen C (fresh) | off | 983/1315 | 302/386 | 523 / 2692 | 20-0-20 | 0 / 0 |
| worldgen C (fresh) | min 6 | 978/1310 | 373/462 | 532 / 2828 | 21-0-19 | -1.8 / -5.4 |

Other tries on A/B: from r440 with min 4: 19-21 and 22-18. Unknown-edge cost 4: 18-22 and 18-22.
Total for min 6: 78-64. Sets A and B were used to choose it; on unseen maps (official + C) it
went 33-29.

Of the no-path feeders, ~80-90% have an optimistic path through unknown ground. Only ~1% are
closed in by unknown portals only. So "cut off" mostly means "never explored the way", not
"behind a portal". Feeding mass at the champion rose (+25% official, +5-7% worldgen), but wins
barely moved: about even with the parent, within noise.

## What should improve

Long cut-off dragons now reach the champion more often, instead of gathering in a
far region until r499.

## Next

Pick seekers by what they can deliver: length vs. optimistic distance vs. rounds left. Start
far dragons before r380 only if they are long. Short walkers cost more gathering than they
deliver.
