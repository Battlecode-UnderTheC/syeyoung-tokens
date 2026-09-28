# body-wall-3

## Idea

Crossover: body-wall-2's early and mid game (gst23 economy, wall seal/squeeze steps, cap
wallers, ally room cost) with late-surge's endgame as isolated in gather-split-trade-31.

## What changed from the parents, and why

Base body-wall-2 (namespace `bw2` -> `bw3`, macros too). Taken from gather-split-trade-31
(its whole diff against gather-split-trade-27, the endgame only):

- `split_until_round` 350 -> 400 (workers keep splitting longer).
- `feed_from_round` 340 -> 380, `leader_feed_from` 440 -> 380, `leader_feed_fresh` 10 -> 20:
  from 380 every dragon, leaders included, walks to the sonar champion (length - 0.5 x age).
- `feed_distance` 2 -> 3.
- `champ_freeze = 330`: the champion stops growth-splitting from round 330.
- `feed_clear_tenths = 8`: feed only if our length < 80% of the champion's known length,
  otherwise hold and keep gathering (reason `held: peer not clearly longer`).
- `trust_visible = 1`: an ally whose whole body is in view (last segment not on the window
  border) counts its visible length, so a trap-split stub isn't taken for the old champion.

Side effect in body-wall code: cap-waller chasing and squeeze are gated on
`round_ < feed_from_round`, so they now run until 380 instead of 340. Left as is.
gst31's cap_safety / reserve logic (from the gst27 line) is NOT taken: that's mid game.

## Measured (build/tools/sim, 2 threads, both sides of each map; worldgen = one fresh batch of 20)

| a vs b | official (22) | worldgen 20 (40) |
|---|---|---|
| bw3 vs body-wall-2 | 13-0-9, longest +2.5 | 23-0-17, longest +5.7 |
| bw3 vs gather-split-trade-31 | 11-0-11, longest -0.3 | 19-0-21, longest +0.2 |
| (reference) body-wall-2 vs gst31 | 8-0-14, longest -1.1 | 14-0-26, longest -5.5 |

check: all checks passed (turn times meaningless this run: machine load ~78; body-wall-2
shows similar numbers under the same load; no search code added).

## What should improve

The endgame closes body-wall-2's gap to gst31 (14-26 -> 19-21 on worldgen, 8-14 -> 11-11 on
official) and beats body-wall-2 on both sets, consistent with g7-7: the endgame carries most
of late-surge's edge regardless of the mid game under it.

## Tried, didn't work

Nothing else tried (short loop).
