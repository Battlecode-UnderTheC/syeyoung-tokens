# body-wall-2

## Idea

body-wall-1 (gst23's economy + "wall: seal" / "wall: squeeze" steps) with the wall made
cheaper for our own side: wallers only when splitting is impossible anyway (the unit cap),
and a squeeze pays for the room it takes from allied heads in view.

## What changed from the parents, and why

- Namespace `bw1` -> `bw2` (and macros).
- **Cap wallers** (`cap_waller_mod = 1`): when `units >= unit_limit - split_headroom`, every
  non-leader worker of length >= `squeeze_length` (before the feed round) takes the waller
  goal: walk to enemy heads seen in the last 5 rounds within 12 steps. No split is possible
  then, so growing long and chasing costs no economy. The old 1-in-4 permanent waller role
  stays off (`waller_mod = 0`).
- **Ally room cost** (`ally_room_cost = 1.0`): in `Wall`, for each allied head fully in view,
  the static room it has now (our body as it is) minus its room after our candidate step,
  floored at 0, is subtracted from that step's squeeze score. Squeezes that pen our own
  workers are dropped or redirected.

## Measured (sim, both sides of each map)

| version | opponent | maps | W-D-L |
|---|---|---|---|
| final (both) | body-wall-1 | official | 13-0-9 (longest +3.7, total +11.7) |
| final | body-wall-1 | 20 worldgen (set A) | 24-0-16 (longest +2.5, total +28.3) |
| final | body-wall-1 | 20 worldgen (set B) | 23-0-17 (longest +1.4, total +27.6) |
| no cap wallers | body-wall-1 | official / set A | 11-0-11 / 19-0-21 |
| no ally cost | body-wall-1 | official / set A | 10-0-12 / 21-0-19 |
| final | gst23 | official | 9-0-13 (body-wall-1: 11-0-11) |
| final | gst23 | set A | 21-0-19 (body-wall-1: 23-0-17) |
| final | gst23 | set B | 21-0-19 (body-wall-1: 21-0-19) |

Each change alone is about even with the parent; together they beat it (60-42 over three
map sets). Against gst23 it is even (51-51), about as body-wall-1 (55-47): no clear gain
there. Squeeze fires ~3.5x as often as in body-wall-1 (cap wallers bring bodies to enemies).

## What should improve

Late-game (at cap) pressure on enemy heads with long bodies, without the economy cost of
body-wall-1's wallers; fewer squeezes that trap allies.

## Didn't work / caveats

- Our "trapped: no move" deaths went up vs body-wall-1 on worldgen (234 vs 196 per 40
  matches): chasing wallers at the cap walk into crowded fights. Still net positive.
- Seal still ~never fires (2 in 40 matches).

## Next

- Cap wallers for only a fraction (cap_waller_mod 2) or only long ones; the ally cost also
  in the waller chase (don't go where allies are already).
- Seal with the lunge (multi-step MOVE across a corridor exit).
