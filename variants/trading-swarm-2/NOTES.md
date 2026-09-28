# trading-swarm-2

## Idea

A clone of the stronger real server opponent (M491281, 8-2 vs gather-split-trade-27): a
memoryless trading swarm that splits whenever length allows, shares its champion by sonar,
crosses portals, and consolidates late by feeding short dragons to the longest one.

## What changed from the parents, and why

From trading-swarm-1 (namespace `ts1` -> `ts2`, file `swarm2.cpp`):

- **Split always**: `SPLIT 2` at every length 4-7 (was a random 4/5/6 threshold, max 8), rare
  splits of long dragons only before r300. Matches the real one (parent length 4 in ~95% of
  its splits). ~250-650 splits a match in sim vs ~90 for the parent.
- **Sonar every turn**: every dragon pings all 4 directions with the longest ally it knows
  (length, x, y, round stamp; 32-bit word, tag bit 31), relaying what it heard for 12 rounds.
- **Late consolidation** (from r350): the dragon that believes it's the champion stops
  splitting from r330; a dragon of length <= 5 that sees an ally head >= max(L+3, 6) within
  2 tiles resigns (pearls drop next to the champion's head), else walks next to it, else
  steers toward the sonar champion position. Real one: longest 8 -> 55 in 100 rounds.
- **Portals**: when no pearl is reachable in view, cross an adjacent portal 2/3 of the time
  (always if pearls are visible but unreachable, i.e. portal-only stores). Safety: never
  opposite the facing, never the same portal pair again within length+1 rounds (our body
  is over there), and skip when the partner edge is in view and the landing is occupied.
  Without the "same pair" rule, check found 70+ self deaths on schooltime.
- **Reverse split**: boxed in (no free neighbour with room) at length >= 5: `SPLIT L-2`.
- **Pincer**: 2-3 long dragons within 2-4 tiles of an enemy head of length >= 5 walk to the
  tile in front of it (or any tile next to it); the adjacent-head trade then takes it.

## Measurements (final code; official = maps/official 22 matches, wg = 20 worldgen maps, 40)

| opponent | official | worldgen |
|---|---|---|
| trading-swarm-1 (parent) | 16-4-2 | 37-0-3 |
| anchor-random | 19-3-0 | 40-0-0 |
| gather-split-trade-27 | 13-0-9 | 16-0-24 |
| gather-split-trade-34 | 12-0-10 | 13-0-27 |
| swarm-hunt-9 | 8-0-14 | 12-0-28 |
| counter-swarm-17 | 9-0-13 | 11-0-29 |
| late-surge-10 (league quick, official+20 wg) | 26-0-36 total | |

Parent vs gather-split-trade-27 on official: 9-0-13. Step by step vs gst-27 official:
split+sonar+portal+feeding@380 11-11, feeding@350 12-10, +pincer 11-11 (but gst-34 8-14 ->
9-13 and 9-31 -> 14-26, parent 13-4-5 -> 15-4-3), portal 1/3 -> 2/3 13-9 (wg 19-21 -> 16-24; noise).
`check`: all checks passed.

## What should improve

Long games on big maps (the champion now reaches 40-55 by r499 like the real one). Still far
from the real one's 8-2 against gst-27: loses stronghold, schooltime, queen_of_spades on
longest, and most worldgen maps.

## What didn't work / open

- Deaths are dominated by "stuck" (150-500 a match: 2-long dragons boxed in by allies), as
  in the real replays (hundreds of self/wall deaths), but that mass is wasted unless it dies
  beside the champion. Next: let boxed-in shorts resign only when a long ally is near, and
  steer shorts away from each other early.
- Portal crossings still few on worldgen (~7 a match): only adjacent portals are taken; a
  BFS that treats visible portal edges as steps would get closer to the real 110 a game.
- The sonar champion is a position that goes stale after 12 rounds; shorts far from it
  mostly wander.
