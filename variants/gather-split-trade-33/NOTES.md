# gather-split-trade-33

## Idea

gather-split-trade-31 (gst27 + late-surge endgame) with the portal orbit from the first server
battle (M489641) fixed: dragons now cross unknown portals when exploring (after a clean sonar
check), stop chasing explore value they will never collect, and break any idle loop.

## What changed from the parents, and why

- Namespace/macros renamed gst31 -> gst33.
- Portal edges are recorded per tile side from each window (`portal_`), so an unknown step can be
  told apart as "unknown portal" vs "edge never seen".
- **Sonar check + crossing** (after portal-play-1): when a one-step MOVE lands next to an unknown
  portal, that turn's sonar is a single ping through the portal from the new head. Next turn, if
  ECHOES shows no ally/enemy hit, the whole body is known and in view, no enemy head is in view,
  the goal is pure exploring/patrol (TileValue 0, not a trap), and round < 480: step through
  ("portal: explore").
- **Explore value** for an unknown portal side only when we would cross it (`WillCross()`:
  round < cross_until). Before r480 this is always true, so it matters only at the very end.
- **Loop breaker**: the last 12 heads on at most 6 tiles and no length gained in 12 rounds ->
  the current (and previous) goal tile is banned for 50 rounds (ChooseGoal and patrol skip it).
  Not while waiting next to a bed whose countdown is still running (circling there is the plan),
  and never the feeding target.

## Measured (sim, vs gather-split-trade-31, 2 threads)

Idle = turns whose last 12 heads cover <= 6 tiles with nothing eaten (debug counter, compiled
out now). "Before" = this code with the three new params off (== gst31).

| maps | before idle / eaten / W-D-L | after idle / eaten / W-D-L |
|---|---|---|
| official (11, 22 games) | 2.93% / 10329 / 11-0-11 | 1.02% / 9924 / 13-0-9 (0.10% / 10682 without the bed-wait exemption) |
| worldgen set 1 (20, 40 games) | 1.69% / 54883 / 20-0-20 | 0.40% / 60459 / 22-0-18 |
| worldgen set 2 (seed 8808, 40 games) | 1.31% / 53226 / 20-0-20 | 0.34% / 54787 / 24-0-16 |

~1000 portal crossings per 40 games; deaths on the "portal: explore" step about 2 per match
(few). The idle counter also counts bed-waiting circles, which the exemption keeps on purpose.
check: all checks passed, slowest turn 0.1-0.2 ms.

## What should improve

Real maps with portal-only rooms (server Default, Trauma): the dragons now reach the beds and
pearls behind portals instead of orbiting.

## Not done / next

- "Don't cross when the portal would be the only way back" is not modelled explicitly.
- Bans are frequent (~50 per game); a softer ban (score penalty) or a shorter ban may be better.
- An unclean echo is just retried on the next approach; banning that portal side for a few
  rounds would stop re-pinging a portal a dragon sits behind.
