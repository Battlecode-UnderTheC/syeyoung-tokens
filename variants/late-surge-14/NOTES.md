# late-surge-14

## Idea

late-surge-13 plus gather-split-trade-36's early "outnumbered" mode, against trading swarms on
small crowded maps. Before r200, when the distinct enemy heads seen in the last 30 rounds are
>= 4 and exceed 1.5x our dragon count (or, on a map of <= 800 tiles, an enemy was seen by r25
and they outnumber us): no even trades (only strictly longer enemies), the reactive trader
switch is off, and every dragon treats a step within 2 steps of an enemy head it won't trade
with as exposed (SafeStep's `exposed` flag only; room counting unchanged). Plus the opening
centre pull on small maps (r < 30): known tiles score `0.5 / (dist + 1 + torus dist to centre)`.

## What changed from the parents, and why

- Namespace ls13 -> ls14 (files, macros).
- Params `om_*`, `centre_*`; `UpdateOutnumbered()` after `Remember()` each turn; `ExposedNear()`
  in SafeStep; a no-even-trade line in `WorthTrading` (after ls13's own `outnumbered_factor`
  rule, which already asks 2x length when `units_ < EnemiesSeen()`).
- Consistency with late-surge's trader switch: `Trading()` is false while outnumbered, so the
  dodge mode wins over "trade evenly because they trade into us" (trade margin and hunt radius
  fall back to the quiet ones). The trader flag itself is still learned and spread by sonar and
  takes over again once the mode ends (r200 or numbers even out).
- The centre bonus is applied before the explore check (it only raises a tile's score).

## Measurements (sim, 2026-09-28)

Official maps; the small ones (trophy, devil, arena, default_small, Colosseum) counted
separately, both sides each:

- vs late-surge-13: official 11-0-11 (small maps 5-0-5).
- vs trading-swarm-4: official 15-1-6 (parent 14-0-8); small maps 6-1-3 (parent 4-0-6):
  trophy 2-0 (parent 1-1), devil 0-2 (parent 0-2), arena 1-1-0 (parent 1-1; the draw: both
  wiped out at r2), default_small 1-1 (parent 2-0), Colosseum 1-1 (parent 0-2).
- vs gather-split-trade-38: official 10-0-12 (parent 9-0-13); small maps 6-0-4 (parent 5-0-5):
  trophy 1-1 (1-1), devil 1-1 (0-2), arena 2-0 (1-1), default_small 1-1 (2-0), Colosseum 1-1 (1-1).
- 20 worldgen maps (40 matches): vs late-surge-13 22-0-18; vs trading-swarm-4 26-0-14
  (parent 25-0-15); vs gather-split-trade-38 21-0-19 (parent 19-0-21).

## What should improve

Survival in the opening on small crowded maps against trading swarms (trophy, Colosseum, arena).
Devil is still lost to trading-swarm-4; default_small lost one it used to win.

## Not tried

No parameter sweep (short loop). Next: a longer om_until on devil-like maps; the dodge for the
lunge/kamikaze decisions too.
