# ring-shield-rush (g53-16, arena ring-shield-10)

Base: ring-shield-bastion (arena ring-shield-3). The parent's notes are in its own directory.

## Idea
User direction 2026-10-09: a SOFT centre preference is allowed (early growth matters; real maps look like
today's). Port ring-rush-1's middle race (g39-4; ring-sched-8 was our best server climb) onto the head.
Server-r27: the opening race is our largest loss source (Devil: first head in the middle r15.4 v r10.6).

## Change (rush.cpp UpdateCentre/CentreGoal/CentreRusher/InMiddle; hooks in field.cpp UpdateField and
strategy.cpp Decide; CENTRE_MODE=0 is the parent)
- Rushers: the two lowest-id non-queen starters (q+2, q+4), when the centre is 5..26 from spawn and the
  starter sees no field at spawn (the field mode's own kSpawnBeds test). The field mode leaves them out.
- Until r30: goal = target, or the frontier tile minimising steps there + straight line on (rush-1's
  RushGoal). No growth split, second free step (RushSprint). Target refined to the densest cluster
  (ClusterScore >= 2) seen within 4 of the centre.
- Ends: arrived within 2 -> hold to r60 (goals > 8 away count half, existing ChooseGoal hook). 6 rounds
  without getting nearer -> hold if within 7 of the centre and a cluster is seen, else explore the frontier
  (the head's g40-5 explorer). Middle seen (all tiles within 2) and no cluster within 4 -> explore. r30 ->
  hold if within 7, else ordinary worker.
- InMiddle (parity rules: no exposure at head parity, 1-for-1 trades when outnumbering) also covers any
  non-queen worker within 7 of the centre before r60.
- Everyone else keeps the field / frontier opening; queen unchanged.

Tried and dropped (each made devil seed 2 lose both seats, 23v7 -> 10v18 dragons at r59):
- food within 2 steps first on the rush (as the head's explorers);
- give up when round + (best frontier steps + straight line) passes r30.
Kept from iteration: spawn-field starters don't rush (dilemma s1: a length-11 starter on 6 pearls, held
from its growth splits, walked into a dead end and lost 9 segments to trapped splits).

## Traces (sim, v CENTRE_MODE=0 = the parent; first non-queen head within 3 of the centre)
- devil s1/s2, both seats: us r10, r9, r13, r11 v parent r15, r11, r16, r11; dragons at r59 17-25 v 7-11.
- trophy: one rusher (2-dragon team), reaches the cup rim r14-17 at length 3, never eats on the way, and
  is lunged at r19; local trophy 0-4 (dragons r59 3-7 v 21-35). Arena duel trophy 2-2.
- dilemma: centre slow beds only -> rusher gives up as barren (r15/r25); spawn-field starter farms home.
- islands: us r15/r15/r24/r49 v parent r24/none/none/r25; dragons r59 ahead in 3 of 4.
- default (official): centre rooms open by portals only; rushers circle to r13-27 then explore; dragons
  r59 5-8 v 8-12. Arena duel default 0-4.

## Local
- server seeds 1-2 v the parent: 33-35 (seed 1 alone 20-14). r50 13.8/34.2 v 13.8/34.4; r100 23.7/62.0 v
  22.8/58.6.
- fresh mapgen (seed 531601, 44 maps x seeds 1,2 x seats): 90-86. anti-centre 9-7, crowded 8-8, maze 10-6,
  mixed 7-9, off-axis 5-11 (seeds 3-4 7-9, 6 more maps 12-12: 24-32 overall), open 7-9 (8-8 rerun), rich-near-home 8-8,
  ring 9-7, rooms 11-5, sealed-spawn 9-7, sparse 7-9. r50 total 47.6 v 51.1 (lower on off-axis/open/ring).

## Arena duels (eph-96cda929, first draft: no spawn-field exclusion, no cluster test on hold)
- server random:2 39-29 (3+ 24-16, <=2 15-13; elim 11-11, queen 21-11, longest 7-7). dragons@100 25.2 v
  22.5, board control@200 .51 v .45, queen alive .63 v .41. By map: australia, autarky, weakhold 4-0;
  default 0-4; queen_of_spades, trauma 1-3; rest 2-2 or 3-1. CPU max 38M.
- official random:2 10-10 (<=2 7-9). CPU max 61M (same as the parent).

## Arena verdict (ring-shield-10, 448 games): v ring-shield-3 59-0-53 (52.7%), BEATS the parent (within noise)
- By suite v the parent: server 38-30, custom 16-18, official 5-5. Decided: elim 27-23, queen 22-19, longest 10-10.
- v the others: ring-sealed-2 66-46, ring-field-3 64-48, ring-qforage-1 65-47 (all 254-194, 56.7%).
- Eval P(win) v the parent: r100 .511, r200 .521, r300 .515. CPU p99 23.3M, max 50.4M.
- By map (all opponents; the parent's own record in brackets): trophy 16-0 (62%), devil 15-1 (78%), dilemma 14-2
  (64%), stripes 13-3 (51%), islands 7-9 (63%). Losses: fixed-edges 0-8 (57%), fixed-pen 1-7 (55%),
  fixed-centre-trap 1-7 (77%), fixed-off-axis 1-7 (83%), default 3-13 (58%), queen_of_spades 4-12 (52%),
  tower_defense 5-11 (69%): the anti-centre / walled-centre cost the user accepted.
- r60 v the parent (paired games): pearls eaten / dragons. devil 100.2/22.5 v 31.2/9.8; islands 94.2/28.0 v
  78.8/23.2; dilemma 30.5/6.0 v 14.0/1.0; trophy 47.5/13.2 v 52.5/13.8; default 11.2/9.0 v 21.2/13.0;
  fixed-centre-trap 18.5/5.5 v 51.5/22.5; fixed-off-axis 41/16 v 72/30; all maps 60.1/18.5 v 60.4/18.8.
- Reproduced locally (traced): 280674 devil (us A): first head in the middle r10 v r12, dragons r59 29 v 11, win.
  280655 / 280773 trophy (both seats): rusher on the cup r15 v the parent's first head r27-28, wins.

## Next
- The walled / trap centre (default, fixed-centre-trap, fixed-edges): the rushers circle a middle reachable
  only by portals or a 1-wide neck to r13-27. A give-up test on the walk's own evidence (how far the known way
  bends from the straight line) is the next thing to try; the r30 arrival bound I tried was too strict on devil.

# g53-20: ring-shield-ghost (base ring-shield-10)
Strategist C g53 #1, ghost champion. strategy.cpp Decode()/Note():
- Note(): a full (non-lower) report under half of a known >= obit_champion length, from the same round
  or newer, replaces the stored report and starts the obituary (was: newer round only). A same-round
  report > 2x a full same-round report is not allowed to undo it (the pre-split relay).
- Second half (trace showed it needed): on a split child's first turn, a same-round full report of a
  shorter ally (the parent's mass-off announcement, heard in 30+ births in 275209) is recorded and
  relayed as an obituary by the child; the child never knew the parent's old length.
Trace (275209 repro, local match v ring-field-1): 2343 (42) trapped-split at r484, kept being named at
41@r482 to r499 by dragons out of reach of both stub and child (sonar coverage, not bookkeeping).
Local, C's 12 longest-loss maps x {seed,7} x 2 seats v ring-field-1 (batch cut short at 38/48 for fix):
base 23-25 (longest 12-9, both-queens-dead longest 24.4 v 26.3); half-1 only 21-27 (11-10, 27.3 v 26.7);
both halves 16-22 at 38 games (9-8, 28.6 v 27.7). Dead-champion share per dragon-turn r480-499 roughly
0.18 -> 0.16 -> 0.15; real-longest 0.21 -> 0.23 -> 0.24. All within noise.
Arena: not run - tokenz ephemeral timed out (no output) from the worker sandbox.
