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

Duels v ring-sprint-6 (eph-0b8fb81d): server random:2 39-0-29 (3+ starters 25-15, <=2 14-14;
elimination 13-10, queen 16-16, longest 10-3); official random:2 9-0-10. Paired by seat/seed in the
server duel, r0-100: portal crossings 896 v 754, deaths on crossing turns 73 v 83, pearls eaten
within 10 rounds of a crossing 633 v 617. No-portal maps (devil, tower_defense, weakhold) identical
through r60. Portal steps r0-60 per game: default 7.2 v 3.2, islands 9.8 v 5.0, schooltime 8.0 v 3.2,
stripes 11.2 v 8.0; portals map 23.2 v 29.5; trophy 0.2 v 0.2 (unchanged).

Submitted ring-sprint-13 (parent ring-sprint-6): 55-0-57 v parent (loses, within noise); by suite
server 34-34, official 6-4, custom 15-19. v ring-contest-4 63-49, ring-sched-17 61-51, ring-rush-1
72-40. CPU p99 25.8M, max 41.9M. Eval P(win) v parent r100/200/300: 0.468 / 0.481 / 0.463.

Replays: Stripes seat A (arena 201014, 200696; duel 4, 36): our first crossing r10 (base in seat A:
r18-r32 or never; arena 200691/201030 base 1 and 8 crossings r0-60). Seat B crosses at r3 either way
(existing explore). Trophy (200759/784/794/797): 0 crossings r0-60 for both: the cup's entrance
(12,21) is 7+ steps from the cup's food (its exit (12,6) is near it), so the radius-6 credit never
reaches it. The change does not fix Trophy.
