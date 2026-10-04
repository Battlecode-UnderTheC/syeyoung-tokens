# ring-bigq-1

## Idea
Top teams (Vibing++ #1, Sponge #4; evals-r13) grow the queen inside their own swarm from about
r120: 8.7 at r200, 14-15 at r300, 22-26 at r400. Ours sits in its 6-cell ring at 2-3 until
Ring4Exit at r430, and we win only 37% of both-queens-alive games. Grow the queen by grazing,
but only on safe interior tiles.

## What changed from the parents, and why
Parent ring-sched-1. One change (BIGQ_MODE, strategy.cpp `GrazeOk`, `AlliesNear`, QueenMove):
- From r120 to r430, with 20+ dragons and units >= enemies seen (board-level proxy), the queen
  grazes when ALL hold: our segments fill >25% of her 7x7 view (her own counted at most 4;
  keep grazing while >=20%); no enemy head within its true reach + 2 (L-2+ceil(L/4), 1 if L<=2;
  +2 per round for heads seen 1-2 rounds ago; a partly-seen enemy has unknown length, so any
  such head in sight blocks); open room >= 20; head not on a corridor tile; enemy segments in
  view not above ours.
- While grazing: no ring, no ring split, no "shed to six" (also off for good once she has
  grazed); the beam drops the home pull and instead penalises corridor tiles (-50), endpoints
  with room < 20 (-3 per missing tile) and endpoints with few allies around (-0.5 per ally
  below 13 in the 7x7 around the endpoint).
- An enemy near (danger) ends grazing and holds her in the ring/safe habitat for 5 rounds.
- Workers don't feed her (unchanged: feeding skips a queen champion).

## Local numbers
- Sim smoke, maps/server seed 7: 18-16 (graze turns are rare on server maps: the density
  gate fails ~7x more often than it passes, because workers keep off the ring).
- Arena duels vs ring-sched-1 (random:2):
  - server: 37-31 (3+ starters 22-18, <=2 15-13); queen length r200/r300/r400 3.1/3.5/3.4 v
    2.2/2.5/2.8; queen alive r300 0.83 v 0.90, at end 0.59 v 0.63; head-on queen deaths 16 v 20.
  - official: 12-8 (<=2 10-6, 3+ 2-2); queen length r200/r300 5.4/12.3 v 2.9/2.5; longer-queen
    wins 5-0.
- vs cs-queen-split-tail-risk-v1-7, server random:2: 28-40 (queen length at end 6.3 v 10.5).
- A looser gate (start at 20%, keep at 1/6) went 33-35 on server with queen alive at end
  0.50 v 0.59 and no more growth, so it was not used.
- CPU max 31.7M.

## Arena verdict
(pending)
