# ring-qforage (g45-2; new line, parent ring-sprint-6 = variants/ring-sprint-conveyor)

## Idea
Strategist b g45 #1: the queen works through the opening on every start, then parks.
FORAGE35 let her forage only when she was under 4 at r0, no ally in view was 4+, and the team had
fewer than 6 dragons, so on 3+-starter maps she parked at r0. server-r19: on 3-starter maps her
r0-30 circling share is 0.67 v 0.28 for the opponents' queens; in 3-4-starter losses decided by
r60 we eat 4.4 v 6.4 pearls per 100 dragon-turns; devtest's queen eats 5.3 by r100, ours 1.9.

## Change (strategy.hpp QFORAGE_MODE, strategy.cpp QueenMove)
- At r0, a start of 3+ dragons (the team count she sees on her first turn): she forages
  (queen_forage_ = queen_growing_ = true) whatever her length or her allies' lengths.
- She splits at 4 on the existing queen17 forage path, plus the founder rule's open-ground test
  (OpenRoom >= kForageRoom) for these starts; no threat on her head and no corridor as before.
- She settles on the existing "forage35: settle split" path at max(6, 3 x the starting count)
  dragons or r120 (kForageSettleRound), whichever first. 3 = the existing 2-starter settle (6 = 3x2).
- Threat map, corridor/endpoint-room penalties unchanged; no danger settle. 1-2-dragon starts
  untouched (units >= 3 gate).

## Local
- check maps/official: pass.
- Smoke, maps/server seed 7 vs ring-sprint-conveyor: 14-0-20 (one seed, noisy).
- Traces (seed 7): default: she splits at r1, r25, r61, foraging between; devil: split r0, settles
  r27 at 9 dragons; unsw: split r0, settles r31 at 30; schooltime: split r0 (boxed legal), forages
  to r48 then "safe habitat".

- Light arena duels v ring-sprint-6 (eph-1aa2690b): server 37-31 (3+ 23-17, <=2 14-14, metrics
  identical on 2 starters); official 11-9.
- Fresh mapgen (seed 4502, 22 maps, 1 sim seed): 19-25; 3+ 15-21, <=2 4-4. Single seed, noisy.

## Arena: ring-qforage-1 (this code), BEATS parent (within noise)
- vs ring-sprint-6 61-0-51 (54.5%); server 35-33, official 5-5, custom 21-13. Decided by:
  elimination 26-22, longer queen 25-16, longest 10-12. vs ring-contest-4 66-46 (parent 99-72),
  ring-sched-17 69-43 (parent 109-63), ring-rush-1 77-35 (parent 109-63).
- CPU worst p99 25.8M, max 41.9M.
- Games v parent, by start (arena metrics, ours/parent's side in the same games):
  - 1-2 starters (36): every metric identical, 18-18 (byte-identical play, as intended).
  - 3+ (76): 43-33; pearls eaten r0-30 25.1 v 23.2; dragons r30 13.9 v 13.5, r60 23.0 v 22.0;
    eval P(win) r60/100/200/300 .557/.573/.552/.553. 3-4 starters (44): eval r100 .595.
  - Queen circling share r0-60 (feat.py's definition, all 112 replays): 3+ 0.36 v 0.69; queen
    pearls by r100 2.36 v 1.11; queen splits r0-60 1.80 v 1.33.
  - Queen dead before r100: 3-4 starters 14% v 11% (6 v 5 of 44); 3+ 11 v 9 of 76. Two of ours
    are 'no valid move' (199077 around_unsw r9, 198834 fixed-wrap-food r57): foraging among her
    own swarm, an ally head (allies move after her) took her last exit. The rest are lunges.
- Replays: 198882 autarky: she eats r16/20/24/36 and splits r17/25 (parent's queen: nothing by
  r60); 199012 islands: eats r2/7/11/15/20, splits r3/12/21 (parent: splits r3/22); 198713 default:
  splits r2, eats r9/12, splits r16, then settles (parent: split r0 and parked). On 4-long-queen
  starts (australia, around_unsw, devil, schooltime) she splits at r0 like the parent's ring
  bootstrap but then eats little: the home food goes to the workers.

## Iteration 2 (tried, not kept)
Penalised forage endpoints with fewer than two free exits that no other head touches (20 each, the
corridor cost). Fixed the 199077 trap locally (she lives), but duels went the wrong way: v
ring-sprint-6 server 28-40 (3+ 14-26), head-to-head v iteration 1 on 3+ server maps 27-33 (longer
queen 8-19). Reverted; the committed code is ring-qforage-1.

## Next
- The ally-boxing deaths: better fixed worker-side (workers don't take the forage queen's last
  free exit) than by making her flee crowds.
- Her length-2 roaming when no food is in view is aimless; a home or ally-centroid pull when she
  sees no food could cut wandering without restoring the parked queen.
