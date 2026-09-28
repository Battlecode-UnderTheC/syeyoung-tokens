# portal-farmer-2

## Idea

portal-farmer-1 (namespace renamed `pf2`) with the two things its NOTES proposed for stores that are
never fully seen: (1) the lure goes only for *entry* portals, and (2) a dragon that crosses a portal
into a store latches that region as a pocket even though it is not closed.

## What changed from the parents, and why

- **Stranded tiles**: every turn, beds/pearls seen but with no known path (`paths.dist < 0`).
- **Entry portals**: a lured farmer (id % 4 == 0, `lure_mod = 4`, with >= 4 stranded tiles and not
  already in a pocket) values only unknown portals on reachable tiles within `lure_radius = 4`
  (torus Manhattan) of a stranded tile. It crosses such a portal even with pearls in reach
  (`portal: lure`); other unknown portals are crossed only when exploring, as before.
- **Store latch**: the turn after any portal crossing, if the landing tile was unreachable last turn,
  its known plain-edge region doesn't contain the tile we crossed from, it is at most 3/5 of the
  board, and it holds a stranded tile or one lies within `lure_radius` of the landing tile
  (`store_need_stranded = 1`), the region becomes a store (`store_seed_`). FindPockets marks it as
  a pocket (2x bed/pearl value, 0.3x explore) while it has beds, and drops the seed once the region
  reaches back to where we came from or grows too big.
- Debug counters under `PF2_DEBUG` (latches, lured turns with an entry portal in reach, pearls
  eaten in a latched store), compiled out.

## Measured (sim, 11 official maps x 2 seats; 20 fresh worldgen maps x 2 seats)

| config | vs pf1 official | vs pf1 worldgen | vs gst39 official | vs gst39 worldgen |
|---|---|---|---|---|
| **shipped** (lure 4, need stranded) | 9-0-13 | 21-0-19 | 10-0-12 | 26-0-14 |
| lure 4, latch any crossing | 9-0-13 | 22-0-18 | 9-0-13 | 21-0-19 |
| lure off, latch any crossing | 11-0-11 | 23-0-17 | 10-0-12 | 21-0-19 |
| lure off, need stranded | 10-0-12 | 20-0-20 | 11-0-11 | 20-0-20 |

All within noise of each other and of the parent (pf1 was 11-11 / 19-21 vs gst39). Total-length
margins for shipped: -16.7 / +12.9 vs pf1, -8.9 / +16.1 vs gst39.

Pocket pearls eaten (PF2_DEBUG, vs gst39, SUM of both seats; pf1's numbers were one match as A:
default 255, qos 172, stronghold 70, trauma 21):
- need stranded: default 437, trauma 53, qos 248, stronghold 103; latches 18/2/21/18; eaten in latched stores 32/0/46/35.
- latch any crossing: default 328, trauma 88, qos 251, stronghold 262; latches 27/57/30/61; store eaten 59/44/70/120.

## What should improve

Maps with portal-entered compartments whose far side we saw first (qos, stronghold, default):
entering farmers stay and farm. Little on trauma (see below).

## What didn't work / what we learned

- On trauma the crossings land in maze corridors (known plain region 7-17 tiles) with NO stranded
  tile in the region or within 4 tiles: the stores the dragons saw through the wall are not where
  the entry portals lead. Only 2 of ~60 crossings latched with `need stranded`. Latching on any
  crossing into an unreachable-before region (57 latches, trauma pocket eaten 53 -> 88) raises
  pocket eating everywhere but did not change W-D-L.
- The lure is still rarely consummated: lured turns with a reachable entry portal ~200-380 per map
  pair, but only 24-50 lured turns end standing on a pinged portal.
- Pocket-pearl counts go up a lot without moving match outcomes: the win is decided by the
  longest dragon / consolidation, not by gathering totals in pockets.
