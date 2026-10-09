# ring-shield-hold (g53-23)

Base: ring-shield-rush (arena ring-shield-10). The parent's notes: variants/ring-shield-rush/NOTES.md.

## Idea
g53-19: on fixed-edges (0-8), fixed-pen (1-7), fixed-off-axis (1-7) ring-shield-10's rushers reach the middle,
then hold it to r60 (goals > 8 from the target count half). End the hold when the middle the holder sees is poor.

## Trace (step 1; build/tools/match with HOLD_DBG, maps taken from arena replays 280512/280479/280771, seed 1592614637)
- Arena games reproduce exactly (280512 fixed-edges: B wins by elimination r155 locally v r156 in the arena).
- Yield of the held middle (live beds = pearl or bed due within 30, within 8 of the target, in the holder's memory):
  - fixed-edges A: id2 arrives r9 (arrival is within 2, before the middle is all in view, target = the bare
    centre), holds r10-36 with 0 live beds near (58-93 tiles seen) while 1-9 lie in ground seen elsewhere; length 3
    throughout, dies ~r37. Seat B: 0 near / 2-3 far (r24-56); seed 2: 6 near r22, 0 from r38, 4-5 far; length 2-3.
  - fixed-pen: both rushers hold from r6-8, 0 near and 0 far for 50 rounds, length 2 (wandering dc 1-13).
  - fixed-off-axis s2 B: 0-4 near (132-136 tiles) v 2-17 far, length 2-4. (Seed 1592614637: no hold; the loss
    there is the walk, not the hold.)
  - devil (where the hold wins): 20-32 live beds in 116-145 middle tiles (.17/tile) v 3-9 in 137-196 tiles
    elsewhere (.02-.05).
- So the held middle yields 0 pearls per round on these maps; the holder eats nothing to r60.
- But it is one dragon of 10-15: r60 team (non-queen) dragons/length on fixed-edges seed 1592614637 was 13/30 (A)
  and 15/38 (B) v 21/51, 20/47 for the same code with CENTRE_MODE=0. Most of the deficit is the rush itself
  (two starters walking 9-23 rounds unsplit), not the hold.

## Change (rush.cpp HoldPoor + one test in UpdateCentre; HOLD_MODE=0 is the parent)
- A centre holder (rush_state_ 2, centre_) re-tests each round, from its own memory: the middle (within
  kHoldRadius 8 of the target, the radius ChooseGoal's hold already uses) has no live bed, or fewer live beds per
  tile seen than the ground seen beyond it -> the hold ends: before r30 it explores the frontier (as the barren
  rule does), else it is an ordinary worker. No new tuned constants (8 = the existing hold radius, BedWeight =
  the existing cluster test).

## Local
- Paired traces (HOLD_MODE 0 v 1 v ring-shield-bastion): fires on fixed-edges (r10, r24), fixed-pen (r7, r9),
  dilemma seat A (r24), queen_of_spades (r17); never on devil, trophy, stripes, islands, default, tower_defense
  (identical games). fixed-edges A: the released id2 explores, eats to length 4, splits at r50 and lives (v died r37).
  queen_of_spades s1 A turned L -> W; outcomes elsewhere unchanged.
- maps/server seeds 1,2 v ring-shield-rush: 38-0-30; r50/r100 totals 33.6/61.9 v 33.5/60.5.
- fresh mapgen seed 723023 (44 maps x 2 seeds x 2 seats) v ring-shield-rush: 91-2-83, r50/r100 identical. Per family:
  anti-centre, crowded, off-axis, open, rich-near-home, ring, rooms 8-8; maze 7-2-7; mixed 7-9; sealed-spawn 9-7; sparse 12-4.
- build/tools/check ring-shield-hold maps/official: passes.

## Arena duels (eph-7214661a v ring-shield-10)
- server random:2: 34-0-34 (3+ 19-21, <=2 15-13; elim 12-11, queen 16-17, longest 6-6); metrics equal
  (dragons@100 22.9 v 22.8, queen alive .62 v .62). CPU max 33.8M.
- official random:2: 10-0-10, every metric identical (the rule does not fire there).

## Arena verdict (ring-shield-17, 448 games): v ring-shield-10 55-2-55 (50.0%), even (within noise)
- By suite v the parent: server 34-0-34, custom 17-0-17, official 4-2-4. Decided: elim 22-24, queen 22-20, longest 10-10.
- v the others: ring-shield-15 51-3-58, ring-sealed-2 67-45, ring-field-3 64-48 (parent: 66-46, 64-48).
- Eval P(win) v the parent: r100 .490, r200 .510, r300 .511. CPU p99 26.5M, max 50.4M.
- Target maps v the parent: fixed-edges 2-0 (288400, 288313); fixed-pen 1-1 and fixed-off-axis 1-1 are seat
  mirrors (identical games: the rule never fired there on the arena seed). v ring-sealed-2 / ring-field-3 on those
  maps the results equal the parent's (edges LL/LL, pen LL/LL, off-axis LL/WL): the change does not fix those maps.
- 288400 (fixed-edges, us A) reproduces locally (A wins r351 v r352): id2 dies head-on at the middle r9-10; id4
  enters the hold at r23 (stall rule), is released at r24 (2 live beds in 72 middle tiles v 5 in 145 elsewhere),
  walks out (d4 -> d20 by r45) and eats 5 by r60; the parent's id5 in the same game dies at r15-20. Team r60:
  25 dragons / 59 length v 22 / 49.

## Next
- The hold is a small part of these maps' deficit: on fixed-edges the rush itself costs ~8 dragons by r60 v
  CENTRE_MODE=0 (two starters walking 9-23 rounds unsplit, and the length-5 starter dying head-on at the middle
  on arrival in mirror games). The next change is on the rush, not the hold.
