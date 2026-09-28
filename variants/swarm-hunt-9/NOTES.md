# swarm-hunt-9

## Idea

Crossover: swarm-hunt-8's early and mid game (keepers out of lunge range, hunters that
strike/lunge, MirrorAgent frame for team B) with late-surge's endgame as isolated in
gather-split-trade-31: split late, then everyone surges to the sonar champion and feeds it
only when it is clearly longer.

## What changed from the parents, and why

Base swarm-hunt-8 (namespace `swh8` -> `swh9`; no macros used the prefix). Endgame only, from
gather-split-trade-31 (= gst27 -> gst31 diff):

- `split_until_round` 350 -> 400 (hunters keep splitting; `keeper_split_until` stays 300).
- `feed_from_round` 360 -> 380, `keeper_feed_from` 440 -> 380, `keeper_feed_fresh` 10 -> 20:
  from 380 every dragon, keepers included, walks to the champion.
- `feed_distance` 2 -> 3.
- `champ_freeze = 330`: the champion stops growth-splitting from round 330.
- `feed_clear_tenths = 8`: an in-view champion is fed only if our length < 80% of its known
  length; otherwise hold and keep gathering (reason `held: peer not clearly longer`).
- `trust_visible = 1`: an ally whose last visible segment is not on the window border counts
  its visible length (a trap-split stub isn't mistaken for the old champion).

Champion selection (length - 0.5 x report age, sonar-relayed) was already the same in both lines.
Everything else (hunting, trade rules, keeper guard, cap feeding, unit reserve) is swarm-hunt-8's.

## Measurements (build/tools/sim, 2 threads; worldgen = one fresh batch of 20 maps)

| a vs b | official (22) | worldgen 20 (40) |
|---|---|---|
| swh9 vs swarm-hunt-8 | 12-0-10, longest -2.9, total +37 | 22-0-18, longest +4.3, total +57 |
| swh9 vs gather-split-trade-31 | 10-0-12, longest -1.1 | 18-0-22, longest -7.2 |
| (ref) swh8 vs gather-split-trade-31 | 8-0-14, longest -3.2 | 16-0-24, longest -8.1 |

check: all checks passed, slowest turn 0.5 ms (trophy).

## What should improve

Longer longest dragon at round 499 on open/worldgen maps; the gap to gst31 narrows but does not
close, so the endgame is not the whole of gst31's edge over this line (its mid game, likely
fewer head-to-head deaths, matters too). Official-map longest margin vs swh8 was slightly
negative (stronghold 44-82 both sides, big_empty as A), so the gain is modest and noisy.

## Tried, didn't work

Nothing else tried (short loop).
