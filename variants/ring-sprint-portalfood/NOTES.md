# ring-sprint-portalfood

## Idea

Strategist b g45 #3 (T1: portal-gated food). Food we see but have no known path to scores zero
today, and an unknown portal earns only the flat explore credit (explore_value / (d + 1)), so our
workers walk past the 2x2 pearl boxes and the cup that opponents enter through portals
(server-r19: 2100+ opponents 138 portal steps a game v our 67; early losses r0-60 2.2 v 6.7 on 2
starters, r60-100 2.3 v 12.8 on 3-4 starters; Stripes 1195116, Trophy).

## What changed from the parents, and why

Base: ring-sprint-6 (variants/ring-sprint-conveyor). One change, in ChooseGoal + one gate in Decide:

- Workers only (not the queen), before `kGateUntil` (r100), while crossing is on: every seen food
  tile (`base > 0`) with no known path (`paths.dist < 0`) is credited to each reachable tile beside
  an unknown portal within `kGateRadius` (6) torus steps of it. Such a tile scores
  `max(current, sum / (d + 1 + kGateDelay))` (kGateDelay 2: a ping turn and a cross turn), with the
  usual share discount when an ally in view is nearer to it (one crosses, not all).
- On reaching that tile (distance 0, no longer a candidate), ChooseGoal sets `gate_cross_` when the
  credit `sum / 3` beats the goal it chose; Decide's existing "portal: explore" crossing then also
  fires on `gate_cross_` (labelled "portal: gated food"). The ping / exit-clear / length >= 2 /
  body-in-view / no-enemy-head / not-a-trap gates are unchanged.
- Maps without portals: no unknown-portal tile, no change.

Constants: kGateRadius 6 = 2 x the 3-tile view radius (a portal and food one head sees from the
same spot); kGateUntil 100: the observed portal-step gap is in r0-60 / r60-100, and later the
poor-home rule covers locked-off dragons; kGateDelay 2: the strategist's, ping + cross turns.

## Local

Smoke, maps/server seed 1 v ring-sprint-conveyor: 18-0-16; "portal: gated food" 7 crossings per
34-game run on top of 24 "portal: explore" (base 29); 0.2 fatal turns on gated crossings.
`check maps/official`: all passed.

## Arena

(see below)
