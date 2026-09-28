# swarm-hunt-15

## Idea

swarm-hunt-14 (gather-split-trade-38's movement/goal layer + swarm-hunt roles) with its
remaining differences from gst38 switched off one at a time. The one that mattered is the
keeper rule: every round-0 dragon is now a keeper (gst38's leader rule), not only those with
no echo on their first turn. Keeper guard 2, the cap feed and strikes/lunges stay.

## What changed from the parents, and why

- Namespace `swh15` / `SWH15_*`.
- `keeper_echo` 0 (compile-time `SWH15_ECHO`): keepers = all round-0 dragons.
- `SWH15_GUARD` (keeper_guard) and `SWH15_CAPFEED` (cap_feed_max_length) are compile-time
  constants too, kept at swh14's 2 and 3.
- The lunge rule (item 4 of the task) is byte-identical to gst38's Lunge and call site
  (only the kamikaze reason is named "strike"), so there was nothing to switch.

## Measured (sim, both seats; official 22, wg1 = worldgen seed 1212, wg2 = seed 8808, 20 maps each)

| config | vs gst38 (off / wg1 / wg2 = total) | vs swarm-hunt-14 |
|---|---|---|
| swh14 as is | 10-12 / 19-21 / 19-21 = 48-54 | |
| guard off (1) | 10-12 / 18-22 / 21-19 = 49-53 | |
| cap feed off | 12-10 / 18-22 / 18-22 = 48-54 | |
| **echo rule off (committed)** | 12-10 / 19-21 / 19-21 = **50-52** | 13-9 / 23-17 / 18-22 = **54-48** |
| guard off + echo off | 12-10 / 19-21 / 19-21 = 50-52 | 13-9 / 19-21 / 17-23 = 49-53 |
| all three off | 12-10 / 15-25 / 21-19 = 48-54 | |

Mean team total length vs gst38 on official (traced, A seat, games still running at that round):
swh14 r100 48-48, r340 85-105 (7 games); swh15 r100 54-49, r340 101-94. The r100-r340 gap is
closed by the keeper rule: with the echo rule, a round-0 dragon that saw an echo became a
hunter (feeds, trades evenly, lunges) instead of a keeper that keeps growing.

`check`: all checks passed, 0 avoidable deaths.

## What should improve

Total length by r340 and results vs gst38 (about even) and swarm-hunt-14 (54-48).

## Didn't work

Guard off and cap feed off are noise-level alone, and stacking them with the keeper change
is worse on worldgen (49-53 vs swh14; all three off 15-25 on wg1).
