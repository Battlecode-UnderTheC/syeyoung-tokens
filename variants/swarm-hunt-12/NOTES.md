# swarm-hunt-12

## Idea

swarm-hunt-9 with gather-split-trade-33's portal fix ported: dragons cross unknown portals when
only exploring (after a clean single sonar ping through them), stop chasing explore value they
will never collect, and a loop breaker bans goals they circle without eating.

## What changed from the parents, and why

Base swarm-hunt-9 (namespace `swh9` -> `swh12`). From gst33 (= gst31 -> gst33 diff), adapted to
hunt.cpp:

- `portal_`: per tile, the sides that are portal edges, recorded from each window's hedge/vedge.
- Sonar check: when a one-step MOVE lands next to an unknown portal (not the side we came in
  by), that turn's sonar is a single ping through it (ECHOES totals all pings). MirrorAgent
  already rotates the sonar array for team B, so the ping direction is correct in both frames.
- Crossing ("portal: explore"): next turn, if ECHOES shows no ally/enemy hit, the whole body is
  known and in view, no enemy head is in view, round < 480, the goal is not a trap, and the goal
  is pure exploring (TileValue 0 AND not an occupied tile, i.e. not a hunted enemy head) and we
  are not walking to the champion (feeding).
- Explore value for an unknown portal side only when we would cross it (round < 480).
- Loop breaker: last 12 heads on <= 6 tiles with nothing eaten in 12 rounds -> current and
  previous goal banned from ChooseGoal's candidates for 50 rounds; not while waiting at a bed
  whose countdown runs, never the feeding target. (Hunting targets are not filtered by bans.)

## Measured (sim, 2 threads)

Idle = turns whose last 12 heads cover <= 6 tiles with nothing eaten (temporary debug counter,
compiled out). "Before" = this code with the three features off (== swh9). vs swarm-hunt-9:

| maps | before idle / eaten / W-D-L | after idle / eaten / W-D-L |
|---|---|---|
| official (22 games) | 4.06% / 9538 / 9-4-9 | 1.17% / 12169 / 10-0-12 |
| official portal maps only (14) | 5-4-5 | 7-0-7 |
| worldgen set 1 (20 maps, 40) | 2.55% / 57919 / 19-2-19 | 1.06% / 60815 / 22-2-16 |
| worldgen set 2 (seed 8808, 40) | 1.69% / 57422 / 20-0-20 | 0.59% / 58516 / 22-0-18 |

vs gather-split-trade-33: official 10-0-12 (longest -2.9; portal maps 7-0-7), worldgen set 1
20-2-18 (longest +1.5), set 2 22-0-18 (longest +2.4).

~440 crossings per 22 official games, ~1600 / 470 per 40 worldgen games; ~2 deaths per match on
the crossing step. check: all checks passed (slowest-turn numbers were noisy under load avg ~160;
swarm-hunt-9 showed the same spikes, 0.0-0.5 ms on a quiet run).

## What should improve

Real maps with portal-only rooms (server default, trauma): dragons reach the pearls and beds
behind portals instead of orbiting. Pearls eaten +28% on the official maps.

## Seen, not fixed

- trauma: we now eat far more (total 67-22) but lose 20-22 on longest: 7 mid-sized dragons at the
  end, many "trapped: no move" deaths; consolidation into one champion fails there. "trapped:
  no move" deaths rose 32 -> 55 per official match (more dragons, more cap pressure). The
  endgame consolidation, not portals, is the next lever for this line.
- default: lost both seats vs swh9 (17-20, 16-22); before the port it split 1-1.
