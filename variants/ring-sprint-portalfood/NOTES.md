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

## Iteration 2

Correction to the Trophy note above: the cup is NOT walled off. Every Trophy tile is reachable on
foot (BFS over the map's kelp, portals ignored: all 625 tiles); the portal (N edge of (12,6) <->
N edge of (12,22)) is a shortcut from the bottom row into the cup's top, and every Trophy tile is a
bed (gap 1-1000). Local trace (trophy, seed 17910617060193542843, ours as B): the gate credit DID
reach the door: r13-14 worker id3 at (13,22)/(13,21) had goal (12,22)/(12,21) (credit 4-9 from the
stem-base food behind kelp), but the door tile is an unripe bed, so Decide's g40-6 bed-tick wait
("bed: wait for its tick") refused the step onto it every turn and it walked away at r15. Same in
schooltime (seed 12809119335885747224): id4 circled one step from door (21,10) (a bed) r7-r29.
The radius was not the Trophy problem; the bed wait was.

Four fixes, all inside this change:
1. A door chosen for its gate credit (`gate_goal_`) skips the bed-tick wait (goal_due -1): the
   portal is the goal, not the bed.
2. "Seen together" replaces the radius-6 Manhattan cut: food and door within one 7x7 view
   (|dx|, |dy| <= 2 x kVisionRadius), the geometry of what one head sees, no tuned constant.
   And walled = unreachable on the board (a body-free search), not merely behind a body.
3. One gated crossing per dragon until it eats again (`gate_spent_`). Schooltime, seed
   12385104951354685771, ours as B: workers id5/id11 crossed F (38,10) -> (13,18), found a pocket,
   then chased the next door, 3 gated crossings each by r51 at length 2-3, eating 0-1 pearls.
   Schooltime's pairs are far apart (F: 25 tiles), so a door near walled food is not a door into it;
   a dragon whose crossing did not feed it has learned that, and forages normally.
4. A door whose ping came back and was still not crossed (dragon past the exit, enemy head in view)
   is banned for loop_ban (50) rounds, like the loop breaker's ban. Default seed
   2100420407280668120 (duel game 2, 0-4 on default in the first v2 duel): worker id10 re-pinged
   door (6,7)/(6,8) every 3 rounds r8-r32 and never crossed; with the ban it leaves at r11
   (local r100 15 v 15 dragons, win; before 6 v 18, loss).

After: trophy seed 1791... ours as B: id3 crosses (12,22) -> (12,5) at r14 and splits in the cup
(r16, r22); id7 r18, id8 r21/24 too. Local r60 dragons (ours v base in the same seat, base-v-base
as reference), 4 seeds x 2 seats: schooltime A +5, B -9 total dragons (noise-level, single games);
trophy mixed (seed 3 as B 25 v 6 where base-v-base was 16 v 6 for A). Smoke maps/server seed 1:
21-0-13 v conveyor; gated crossings 6. check maps/official: all passed.

Duels v ring-sprint-6. Fixes 1-3 only (eph-79bd30de): server random:2 31-0-37 (3+ 16-24, <=2 15-13;
default 0-4, stripes 0-4), dragons@100 23.5 v 26.1; official random:2 10-0-10. All four
(eph-229eb805): server random:2 32-0-35 (3+ 19-20, <=2 13-15; elimination 14-11, queen 12-19,
longest 6-4), dragons@100 23.9 v 23.4, CPU max 33.9M; official random:2 11-0-8, dragons@100 37.7 v
34.3. Both within noise.
