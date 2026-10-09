# ring-shield-anchor (g53-25)

Base: ring-shield-rush2 (arena ring-shield-15). The parent's notes: variants/ring-shield-rush2/NOTES.md.

## Idea
g53-22's trace of arena 289961 (variants/ring-shield-twolines/NOTES.md): in the final feed, feeders out of
view walk to different, 3-5 round old relayed tiles of the queen while she walks about a row a turn, so no
feed line reaches her (17 long r470-485). Make the late queen wait where the lines are headed.

## Change (strategy.cpp QueenMove, ANCHOR_MODE; 0 = the parent)
From kAnchorFrom = 470, in a queen race (EnemyQueenRace), while
- an ally the conveyor would send is known: a report <= kRaceFresh/3 rounds old, length >= kFeedMin, within
  the conveyor's walking budget (500 - r)/2 + kRaceReach of her head;
- no pearl is reachable over free tiles within two turns of her free steps (2 ceil(L/4)): food she can take
  herself stays hers (the parent's harvest);
- no enemy head's reach covers her head (the beam's threat map);
- she ate within kAnchorIdle = 6 rounds of the anchor being set,
her beam routes ending beyond her loop radius of the anchor cost kAnchorCost = 6 per tile (Chebyshev).
- Anchor: her head when she last ate (a line eaten moves it with her), or at activation.
- Loop radius: she must move every turn and can't enter her own tail, so the tightest walk she can keep up is
  a square ring of >= L+1 tiles, side s = ceil((L+1)/4)+1; radius s/2 (3 at L 17).
- 6 per tile: half a pearl's beam gain (12), so an adjacent pearl is still taken, a far wander is not; all
  danger terms (QueenRisk x10000, room, escape) are far larger.
- kAnchorIdle 6: in the reproduced 289961 (anchored, iteration 1) lines reached her 1-3 rounds apart
  (r474/475/477/478); 6 rounds without one means none is coming and she forages again.

## Iterations (local = maps/server seed 1, both seats, v ring-shield-rush2; trace = 289961 reproduced:
## build/tools/match ring-shield-rush <B> slithery_fight 6669259212651375625, parent queen 17 r469-r492)
1. Off when any enemy head is in view. Trace: lines reach her (eats r474, 475, 477, 478, 490-497),
   r485 21 v 17, r499 28 v 29. Local 17-17, q485 17.7 v 17.8, q499 20.6 v 20.9, anchor on 129 turns/34 games.
   Arena duel v ring-shield-15: server 32-36 (longer queen 13-19, queen@end 13.97 v 14.81), official 9-11.
2. Off on the threat map instead (fires more). Local 17-17, q499 20.3: slithery A (anchored at length 4 she
   circled r471-477 beside a pearl 4 tiles off) and portals A lost queen length.
3. + off while any pearl is in view: inert (on 45 turns); in 289961 pearls were in view r470-490 but none
   reachable (behind bodies).
4. + only reachable pearls within 2 ceil(L/4) count (this version). Trace: r485 24 v 17, but end 27 v 29 (she
   waited r488-494 with nothing arriving, the parent surged r492-499). Local 18-16, q470/485/499 13.6/18.0/20.5
   v 13.6/17.8/20.9 in the 10 both-alive games, late queen deaths 4 v 5, anchor on 102 turns.

## Results (iteration 4)
- Arena duels v ring-shield-15: server 34-34 (longer queen 15-16, queen@end 11.82 v 11.59, alive 54.4% v 51.5%;
  3+ starters 20-20, <=2 14-14); official 10-9 (one game errored; queen@end 16.0 v 16.0).
- Fresh mapgen seed 530251 (44 maps x seeds 1,2 x seats): 85-6-85, every aggregate identical: fresh-map games
  rarely have a living, growing queen in a race at r470 (queen 2.4 at the end), so the anchor never fires.
- check maps/official passes.
