# ring-field-goals (g50-4, arena ring-field-13, parent ring-field-3)

## Idea (strategist-g50 #3, evidence eatrate-r24)
On rich losses our workers head for an unripe bed on 29% of turns v 20% (pearl in view 71%), and
for a live pearl on 34% v 44%. Goal scoring should value food the dragon can actually get.

## Change (strategy.cpp ChooseGoal; `GOALS_MODE 0` = parent)
- (i) When the arrival schedule ran, the share (0.1x) and crowd discounts ignore allies the schedule
  placed: a placed ally already holds its one tile, so it doesn't claim an unmatched pearl too.
- (ii) Cluster bonus: a neighbouring bed counts only if it is ripe when we would reach it (`due0[n] <= dn`).
- (iii) Schedule participants: self always, then the nearest allies by torus distance (ties by id),
  instead of the lowest ids (the id sort + truncate to 12 could drop self).

## Local
- check maps/official: passes.
- Duels v ring-field-3: server 34-0-34 (3+ starters 20-20, <=2 14-14); official 11-0-9.
- Fresh mapgen (seed 50407, 44 maps x 2 seeds x 2 seats) v ring-field-1 (identical to rf3): 84-1-91.
  Per family (16 games each): anti-centre 6-10, crowded 7-9, maze 6-10, mixed 9-7, off-axis 7-9
  (rest not run: machine load ~150).
- Goal mix on the 88 duel games (local exact repro, traced build, workers r10-59), ours v rf3:
  unripe bed 21.6% v 20.5%, live pearl 32.0% v 31.3%; newborn first eat 6.5 v 5.9 rounds.
  Against rf3 both sides are already under the 23% target; the mix did not move.
- Why (instrumented, same games): turns choosing an unripe bed with a reachable pearl within 6:
  parent 2713 (6.4% of worker turns), draft 3058 (7.2%). In the parent the pearl had been cut by
  share 16% / crowd 9% of those times; in the draft 2% / 0% (the change does what it says). The rest
  is the schedule giving that pearl to an ally (56-59%) and beds whose ripe-by-arrival neighbours
  earn a cluster bonus (field beds), which is intended.
- Targeted situation (eatrate-r24's 84 rich rf3 v gst-search-12/queen-3 arena games, local exact
  repro, the draft in rf3's seat; rf3's own repro matches the analysis: 34% pearl / 29% unripe on
  the 32 rich losses). Rich losses, draft v rf3: pearl 31.6% v 33.9%, unripe bed 29.8% v 29.3%,
  ripe bed 6.5% v 4.4%, eats per worker-turn 0.138 v 0.132, newborn first eat 4.83 v 5.25 rounds
  (my birth->first-eat convention, same for both). Targets (<=23% / >=40% / <=4.3) NOT met.
- Why the mix can't move much: of the turns heading to an unripe bed with a reachable pearl within 6,
  ~72% are beds that ripen by the time we arrive (due - dist <= 0; the "unripe" label is at
  decision time, not arrival), and 61% have the pearl matched by the schedule to an ally that gets
  there first. Share/crowd cuts on that pearl fell from 17%/12% to 4%/2%. The remaining lever is
  the schedule itself, which is not this change.
