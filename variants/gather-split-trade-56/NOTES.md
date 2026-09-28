# gather-split-trade-56

## Idea

Measure where the losing side's midgame eating goes (trade corpses, pearls refused beside enemy
heads), then fix the biggest leak: an enemy head in view only contests its 2 nearest
pearls/beds (ties included), not every tile it reaches first, so a corpse line or pearl field
beside an enemy is not written off at 0.3x.

## What changed from the parents, and why

gather-split-trade-51 plus `enemy_claims = 2` in ChooseGoal's contest discount
(`GST56_OFF` restores gst51: every tile an enemy head reaches first is discounted). An enemy
eats one pearl a step, so it cannot take a whole field before we arrive.
Debug-only (`GST56_DEBUG`) exposure probe in SafeStep/Turn: a goal step refused as Exposed,
and whether an adjacent enemy head stepped onto that tile next round.

## Measurements (diagnosis; gst51 unless noted)

Corpse pearls, with a scratch harness (pearl diff around every Apply: new pearls on a dead
dragon's tiles are corpse pearls, the actor that removes one ate it), r60-379, h2h = a
head-to-head happened in that turn:

| gst51 vs | maps | winner's h2h corpses: winner / loser / left | loser's h2h corpses: winner / loser / left |
|---|---|---|---|
| late-surge-21 | 5 small official, both seats | 313 / 129 / 12 | 203 / 211 / 13 |
| late-surge-21 | 20 worldgen, both seats | 7218 / 3536 / 28 | 3972 / 6404 / 29 |

- Trade corpses are split ~64/36 in favour of the dragon's own team; almost none are left
  (<1%). The winner takes ~53% of all trade corpses (~30 pearls a match on worldgen): a real
  but modest channel, and it follows the local numbers (the side with more dragons around the
  trade eats more), not a missing behaviour.
- vs trading-swarm-5 (worldgen, both seats) we eat 28% of their trade corpses, they eat 38% of
  ours: the swarm cleans up better.
- Non-h2h deaths (trapped resignations, body hits) in r60-379: winner's corpses eaten by its
  own side ~2x more than the loser's (15953 vs 7653): more dragons dying trapped among allies
  and recycled. Almost none cross teams.

Exposure probe (goal step refused because an enemy head we won't trade with is adjacent):

| vs | maps | refused (pearl tile) | enemy stepped on it (pearl) | stayed off (pearl) | unseen |
|---|---|---|---|---|---|
| late-surge-21 | 5 small official | 303 (45) | 78 (14) | 173 (23) | 11 |
| trading-swarm-5 | 5 small official | 83 (26) | 31 (15) | 31 (4) | 4 |
| late-surge-21 | 20 worldgen | 2212 (515) | 598 (190) | 1010 (178) | 124 |

~1% of turns; ~13 pearl refusals a match, a third of which the enemy did step onto (a trade
we'd have given). Loosening Exposed would win ~4 pearls a match at the cost of trades: not the leak.

## Measured (sim, gst56 vs gst51, both seats)

| maps | W-D-L | longest / total margin |
|---|---|---|
| official (11) | 16-0-6 | +7.9 / +15.5 |
| worldgen set 1 (20) | 19-0-21 | -0.3 / -41.8 |
| worldgen set 2 (20) | 20-0-20 | +0.1 / +64.5 |

vs late-surge-21, 20 worldgen both seats: gst56 23-17 (gst51 21-19); own trade corpses kept
67% (gst51 64%), theirs taken 37.5% (35.6%). vs trading-swarm-5: 25-15 both (unchanged).
`check`: all passed; slowest turn 54.4 ms on stronghold (parent 54.5 ms, inherited).

## What should improve

Slightly more pearls from fields/corpses next to enemy heads; even on worldgen, ahead on the
official maps (small sample).

## Next

- The losing side's flat length is mostly numbers: it has fewer dragons around each trade
  and fewer allies to recycle trapped deaths into. Levers are upstream (more dragons alive at
  contact), not in goal scoring.
- enemy_claims 1 or 3 not tried.
