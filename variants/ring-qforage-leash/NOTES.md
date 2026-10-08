# ring-qforage-leash (g52-12, arena ring-qforage-3, line ring-qforage)

Parent: ring-qforage-1. Strategist A g52 proposal #7.

## Idea
promo-ring-qforage-1: the 3+-start forage queen died early about twice as often as its parent
(dead by r150 47 v 28), mostly to enemy lunges while foraging alone 9.6 v 5.6 tiles from home with
no ally within 2. Put her on a leash.

## Change (QueenMove, g45-2 forage branch only; LEASH_MODE, kLeashHome=6, kLeashAlly=3)
- A forage goal (a tile the beam's 12-step food flood values, TileValue>0) counts only if it lies
  within 6 (torus) of queen_home_ or within 3 of an ally head seen this turn.
- No such goal reachable: within 6 of home she parks in a ring at her spot (RingQueen, labels
  `leash: park ring/split`); further out she walks home under the parent's 0.6/tile home pull
  (`leash: no goal, home pull`). Settling (3x start or r120) unchanged. 1-2 starts unchanged.

## Local
- check maps/official: pass. Server sim v parent seed 1: 17-17, seed 2: 18-16.
- Fresh mapgen (seed 91212, 44 maps x 2 seeds): 91-85. Per family (16 games each): open 9-7,
  rich-near-home 7-9, maze 8-8, rooms 10-6, sealed-spawn 9-7, ring 8-8, off-axis 7-9, sparse 8-8,
  crowded 8-8, anti-centre 8-8, mixed 9-7. No clear family loss.
- Server-map traces (4 seeds x 2 seats), 3+ starters: queen dead by r150 4 v 7 of 80; dragons r100 30.0 v 31.2.

## Arena
- Light duels v parent (eph-85408e63): server 35-33 (3+ 21-19, <=2 14-14), official 9-11.
- Verdict ring-qforage-3 v ring-qforage-1: 48-0-70 (40.7%), loses to parent. Server 28-40,
  custom 12-22, official 6-4. v ring-field-3 56-59, v ring-sprint-21 59-55, v spar-field-2 45-73.
- Eval P(win) v parent r100/200/300: 0.462/0.459/0.478 (custom 0.455/0.423/0.436).
- CPU p99 24.7M, max 43.9M.
- Replays v parent (112 games, 3+ starters 76): queen dead by r60/100/150 9/12/19 v 8/11/15;
  queen pearls by r60 1.25 v 1.91; distance from home r60 5.5 v 9.2; dragons r100 29.2 v 32.4.
  The leash shortens her range, but the early deaths do not fall and the forage gain is lost.
- 254506 (fixed-crowded, 7 starters): she still walked 10-18 from home r7-r53 toward food near
  allies 6-12 away (the escort clause lets far allies extend the leash) and died to a lunge at r53.

## Iteration 2 (not submitted; code reverted to the ring-qforage-3 version)
Escort = an ally head within 3 of HER head, and off the home leash without an escort the home
pull returns. Duels (eph-80dfc1f8) v parent: server 33-35, custom 27-40 (dragons r100 25.8 v
30.7). Custom replays: queen dead by r150 25 v 30, but queen pearls by r60 0.91 v 2.09 and dragons
r100 26.3 v 31.2. For example, duel-8d1b6fb9 game 4 (fixed-wrap-food): she parked 1 tile from home,
length 3, and ate nothing to r100. The team went from 7 to 4 dragons while the parent's queen fed in
the far lane (20 pearls by r60) and its team reached 64. The 3+-start queen is the team's
producer, so leashing her costs the economy.

Verdict: the leash doesn't work. Not promoted.
