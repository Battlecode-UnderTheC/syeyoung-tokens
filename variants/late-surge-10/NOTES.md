# late-surge-10

## Idea

late-surge-8 plus crossing unknown portals. In the first server battle the dragons walked
around portals whose far end they hadn't seen, because the goal chooser counted those tiles as
map frontier but SafeStep never stepped through them. Now an explorer that arrives on such a
tile pings through the portal, and the next turn it steps through if the echo hit no dragon.

## What changed from the parents, and why

- Namespace `ls8` -> `ls10` (files `ls10.*`, macros `LS10_*`).
- Probe (`Turn`): when the move lands on this turn's explore goal and that tile (a seen tile)
  has an unknown side, that side must be a portal whose far end is unseen, because a seen tile
  has seen all four edges. On that turn we send only one ping, through the portal, carrying the
  champion word. ECHOES counts cover the whole turn, so with a single ping they describe that
  one ray.
- Crossing (`Decide`, after ChooseGoal): if last turn's probe hit no ally or enemy (kelp is
  fine), we're on the probed tile, it was last turn's explore goal, this turn's goal is also an
  explore goal (or there is none), the whole body is in the 7x7 view, and some other free step
  exists, we step through ("explore: through portal").
- If the probe hits a dragon, that portal side gets no explore value for 30 rounds.
- ChooseGoal gives explore value to a seen tile's unknown side only if it is a portal we would
  take (`portal_cross` on and the side isn't banned).
- There is a loop breaker (`loop_ban`): when the last 12 heads cover 6 or fewer tiles, nothing
  was eaten and the goal was an explore goal, it bans that goal. It is off (0) by default: at
  50 it scored 40-52 against gather-split-trade-31, compared with 47-45 without it, and it did
  not reduce the measured explore-goal loops (practice maps: 2959 -> 2938).
- There is a temporary `LS10_LOOPSTAT` counter (idle-loop turns, eats). It is off unless
  defined.

## Measurements (sim, --threads 2; "base" = this code with portal_cross=0, loop_ban=0, i.e. late-surge-8)

Idle loops are turns where the last 12 heads cover 6 or fewer tiles and nothing was eaten.
Measured on the official maps that have portals (Colosseum, default, queen_of_spades,
schooltime, stronghold, trauma):
- idle loops beside an unknown portal: 590 -> 269 (vs late-surge-8), 708 -> 317 (vs gst-31).
- pearls eaten per dragon-turn: 4.93% -> 5.47% (vs ls8); 5.30% -> 5.43% (vs gst-31).
- Most of the remaining idle turns have a valued goal (probably dragons waiting at beds), not
  a portal.

W-D-L (the base is the parent's exact code, so its spread shows the noise):
| opponent | maps | base | full (loop_ban 50) | final (loop_ban 0) |
|---|---|---|---|---|
| late-surge-8 | portal official (12) | 7-5 | 6-6 | 4-8 |
| late-surge-8 | worldgen A (40) | 15-25 | 22-18 | 22-18 |
| late-surge-8 | worldgen B (40) | 16-24 | 24-16 | 23-17 |
| gst-31 | portal official (12) | 8-4 | 5-7 | 6-6 |
| gst-31 | worldgen A (40) | 19-21 | 17-23 | 19-21 |
| gst-31 | worldgen B (40) | 26-14 | 18-22 | 22-18 |

About 8% of crossings were fatal turns (2.8 of 35 on the portal maps).

## What should improve

More of the map explored on portal-heavy real maps (Default, Trauma: most beds are in rooms
reached only through portals), so more pearls, and no dragons circling for hundreds of turns.

## Tried / not done

- The loop breaker at 50 rounds (see above). It is off.
- Not done: crossing when the goal is a valued target on the far side, or letting pathing plan
  through portals after a probe.
