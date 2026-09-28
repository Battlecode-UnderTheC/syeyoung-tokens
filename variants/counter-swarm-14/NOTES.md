# counter-swarm-14

## Idea

Crossover: counter-swarm-13's early and mid game (multi-step strikes, its economy, the
champion guard against lunges) with late-surge's endgame as isolated in gather-split-trade-31
(late split, everyone surges to the sonar champion at 380, feeds only a clearly longer one).

## What changed from the parents, and why

Base counter-swarm-13 (namespace `cs13` -> `cs14`). Endgame taken from gather-split-trade-31:

- `split_until_round` 350 -> 400 (workers keep splitting longer).
- `feed_from_round` 340 -> 380, `leader_feed_from` 440 -> 380, `leader_feed_fresh` 10 -> 20:
  from 380 every dragon, leaders included, walks to the champion (reports ranked by
  length - 0.5 x age, unchanged from cs13).
- `feed_distance` 2 -> 3.
- `champ_freeze = 330`: the champion stops growth-splitting from round 330.
- `feed_clear_tenths = 8`: feed only if our length < 80% of the champion's known length,
  otherwise hold and keep gathering (reason `held: peer not clearly longer`).
- `trust_visible = 1`: an ally seen with no segment on the window's border counts its visible
  length (reuses cs13's `Sighting::cut`, "any segment on the border", slightly more
  conservative than gst31's "last segment on the border").

Kept from cs13: the champion guard now has its own `guard_from = 340` (it used to key off
`feed_from_round`), so it still starts at 340. Side effect shared with gst31: hunting keys off
`feed_from_round`, so workers now hunt until 380 instead of 340.

## Measurements (build/tools/sim, 2 threads; worldgen = one fresh batch of 20 maps, 40 matches)

| a vs b | official (22) | worldgen 20 maps (40) |
|---|---|---|
| cs14 vs counter-swarm-13 | 14-1-7, longest +1.5, total +34 | 24-0-16, longest +6.5, total +62 |
| cs14 vs gather-split-trade-31 | 10-0-12, longest -4.9 | 18-0-22, longest -1.8 |
| (reference) cs13 vs gather-split-trade-31 | 12-0-10, longest -0.2 | 13-0-27, longest -11.5 |

check: all checks passed. Slowest-turn numbers were meaningless (load average ~96: cs13 itself
measured 8-48 ms in the same minute); the added work is O(1) per turn, search code unchanged.

## What should improve

The endgame lifts the counter-swarm line on worldgen the same way it lifted gst27: vs gst31
it goes from 13-27 to 18-22, and it beats its own parent on both map sets. The remaining gap
to gst31 on worldgen is its mid game (lunges, cap safety, detours) and total length (-61).

## Tried, didn't work

Nothing else tried (short loop).
