# counter-swarm-18

## Idea

counter-swarm-17 with gather-split-trade-33's portal-orbit fix ported: dragons cross unknown
portals when exploring (after a clean one-ping sonar check), stop valuing unknown portals they
will never cross, and break idle loops by banning the goal they circle.

## What changed from the parents, and why

- Namespace/macros/files renamed cs17 -> cs18.
- Portal sides recorded per tile from each window (`portal_`), as in gst33.
- **Sonar check + crossing**: after a one-step MOVE that lands next to an unknown portal, that
  turn's sonar is a single ping through the portal (ECHOES totals all pings). Next turn, if the
  echo is clean, the whole body is known and in view, no enemy head is in view, the goal came
  from ChooseGoal (not snipe / feeding / champion approach), is not a trap and has TileValue 0
  (pure exploring), and round < 480: step through ("portal: explore").
- **Explore value** of an unknown portal side only when `WillCross()` (round < 480).
- **Loop breaker**: last 12 heads on <= 6 tiles and nothing eaten for 12 rounds -> the current
  and previous ChooseGoal goal are banned for 50 rounds. Not while waiting at a bed whose
  countdown runs; never for snipe/feeding targets (only goals from ChooseGoal are banned).
- counter-swarm's SafeStep, strikes, guard, feeding all untouched. With the three params off
  the variant is move-for-move identical to cs17 (11-0-11 / 20-0-20 / 20-0-20 mirror results).

## Measured (sim, 2 threads)

Idle = turns whose last 12 heads cover <= 6 tiles with nothing eaten (debug counter `CS18_IDLE`,
compiled out now). Before = params off (== cs17), both vs counter-swarm-17.

| maps | before idle / eaten | after idle / eaten / crossings / bans |
|---|---|---|
| official (22 games) | 3.17% / 9680 | 0.75% / 9292 / 294 / 559 |
| worldgen seed 1515 (40) | 2.24% / 48537 | 0.63% / 50767 / 1080 / 1970 |
| worldgen seed 8808 (40) | 1.46% / 54016 | 0.42% / 53446 / 792 / 1293 |

Idle loops fall 3.5-4x (same as in gst33). Pearls eaten roughly unchanged.

W-D-L (full port):
- vs counter-swarm-17: official 10-0-12, portal maps (Colosseum, default, queen_of_spades,
  schooltime, stronghold, trauma, trophy) 5-0-9, worldgen 1515 16-0-24, 8808 19-0-21,
  4242 20-0-20. Total 65-0-77.
- vs gather-split-trade-33: portal maps 10-0-4, official 13-0-9, worldgen 1515 17-0-23,
  8808 20-0-20 (total 60-0-56). For reference cs17 vs gst33: 8-0-6, 13-0-9, 27-0-13, 16-0-24
  (64-0-52).

Ablation vs cs17 (official + 1515 + 8808): loop breaker only 50-1-51; portal crossing only
49-0-53; both 45-0-57. None of them is clearly outside noise, but none is clearly a gain in sim.

check: all checks passed, 0 avoidable deaths (audit build slowest turn ~16 ms, as for cs17).

## What should improve

Real server maps with portal-only rooms (Default, Trauma on the server): the dragons should now
reach beds and pearls behind portals instead of orbiting. The local sim maps don't reproduce the
orbit's cost the way the server games did, so the sim W-D-L may understate it.

## Not done / next

- Slightly behind the parent in sim (65-77 over 142 games, about -1 sd). A softer loop breaker
  (score penalty, or ban only the current goal / shorter ban) is the obvious next try; bans are
  ~50 per game.
- Crossing only when the portal is not the only way back is not modelled.
