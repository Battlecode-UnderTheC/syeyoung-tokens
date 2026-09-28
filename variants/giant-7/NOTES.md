# giant-7

## Idea

giant-4 with the early-growth handicaps removed. The round-0 giants split off helpers until 100 and then
only grow, and the helpers feed the king from 340, same as before. What changes is that while they split, giants
gather as freely as helpers do, and helpers no longer treat ally giant heads as threats.

## Diagnosis (giant-4 vs gather-split-trade-31)

I played traced matches on the 11 official maps and 10 worldgen maps, both sides. The table gives
per-side averages as giant-4 / gst31. "splits" means dragons born so far. No-action deaths include
trapped resigns and feed deaths.

| round | official dragons | official length | official splits | worldgen dragons | worldgen length | worldgen splits |
|---|---|---|---|---|---|---|
| 50  | 7.9 / 10.3 | 19 / 27 | 10 / 13 | 22 / 28 | 54 / 73 | 30 / 36 |
| 100 | 13.5 / 20.2 | 34 / 54 | 25 / 35 | 32 / 43 | 79 / 117 | 88 / 107 |
| 200 | 14.5 / 27.6 | 46 / 84 | 49 / 75 | 35 / 51 | 93 / 144 | 212 / 246 |
| 340 | 16.4 / 31.3 | 49 / 97 | 82 / 115 | 36 / 52 | 96 / 153 | 380 / 445 |
| 400 | 8.5 / 30.2 | 36 / 112 | 87 / 130 | 16 / 52 | 87 / 157 | 416 / 528 |
| 499 | 4.5 / 22.5 | 30 / 162 | 90 / 140 | 9 / 26 | 84 / 218 | 449 / 574 |

- **The body deficit opens in the first 50–100 rounds.** Deaths are almost identical at every
  checkpoint (head-to-head: 3.5/3.5 at r50, 10/10 at r100, 40/45 at r340). Helper deaths aren't
  the gap. By r100 giant-4 has about 60–70% of gst31's mass and never catches up. Later its
  per-dragon split rate matches gst31's. Its absolute losses stay equal, though, so the attrition costs the smaller team
  proportionally more. Splitting later doesn't close it: fewer splits come from less mass,
  not from the split rules.
- **This is intrinsic, not a matchup effect.** In a mirror on the official maps, giant-4 is at 19.4
  at r50, while gst31's mirror is at 27.2. Toggles measured in the mirror at r50/r100: `strict_early=0`
  gave 21.2/39.7, `avoid_giant_len=0` 22.2/44.3, `evade_distance=1` 20.3/38.5, `share_factor=1`
  for helpers 20.3/41.0, and `unknown_room=0` 19.8/41.2. None of them reaches gst31's 27. I didn't
  find the rest of the early gap. The next suspects are gst31's pathing (dead-end/escape
  levels, "into a dead end worth it") and its contest rule for leaders.
- **The endgame is the second gap.** After the surge at 340, giant-4's team shrinks to 4–9
  dragons. Its king is then lunged by a swarm 3–5× bigger: 7 of 20 worldgen losses were
  wipeouts, with total 0. gst31 keeps splitting until 400, and its total grows 97 → 162 over
  340–499. Earlier toggles (split 380 / feed 360) didn't help giant-7 (6-16 official, 3-17 on those 10 worldgen maps).

## What changed from the parents, and why

- `strict_early` 1 → 0: until `giant_split_until`, a giant evades and trades like a helper. It no longer keeps
  the strict room margin and full head avoidance while it is still small.
- `avoid_giant_len` 6 → 0: helpers no longer path around the heads of long allies.

These two gave the most early mass in the mirror (+15% at r50). Everything else is giant-4.

## What I measured

Sims, both sides of each map:

| opponent | maps | giant-7 | giant-4 (same maps) |
|---|---|---|---|
| giant-4 | official (22) | 9-13 | – |
| giant-4 | worldgen set 1 (40) | 22-18 | – |
| giant-4 | worldgen set 2 (40) | 25-15 | – |
| gst31 | official | 8-14 (longest −2.2) | 7-15 (−7.5) |
| gst31 | worldgen set 1 | 8-32 | 11-29 |
| gst31 | worldgen set 2 | 13-27 | 12-28 |

Against its parent head to head, giant-7 went 56-46. Against gst31 it is about even with giant-4 (29-73 vs
30-72). This is a small step: the structural gap stays open.

## What didn't work (vs gst31, official 22 / worldgen 20)

- `giant_split_until=300`: 9-13 / 5-15.
- `helper_room_margin=0,helper_room_min=4`: 11-11 / 3-17.
- `feed_from_round=360,helper_split_until=380`: 6-16 / 3-17.

All of these are noise-level.

## What should improve / next

Next, I'd port gst31's movement layer (its `SafeStep`/escape levels and goal choice) into the giant
line, keeping the giant roles and the endgame. The early mass gap (about 30% at r50 in a mirror) is
about how dragons move and eat, not about the split window. For the endgame, I'd keep a reserve of dragons out of the
surge so a lunged king doesn't mean a wipeout.
