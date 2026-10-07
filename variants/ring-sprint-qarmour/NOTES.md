# ring-sprint-qarmour (g46-4) = arena ring-sprint-24

- **Line:** ring-sprint. **Parent:** ring-sprint-guard2 (arena ring-sprint-21).
- **The one change:** queen armour (strategist g46 C #3), from kRaceFrom (r430), in armour.cpp:
  - QueenRisk uses the rules' reach ceil(Le/4)+Le-2 (floor 3, no cap of 8) for each enemy head in
    view; unbounded within view for a head whose body leaves view and for a body seen without its
    head (from its frontmost segment, +1). Her whole move is judged by its endpoint (she acts first).
  - conveyor feeders lay a line only if none of its corpse-pearl tiles is in that reach (else wait);
  - she stops valuing pearls once growth can't change the result (enemy queen seen dying, or longer
    than every enemy seen + 2/round left; 2 = the fastest fed-queen growth in our replays).
- **Gate fix:** plugin.cpp takes the late lookahead step only if it is a free step (check caught a
  worker stepping into kelp at schooltime r497 once the armour changed the trajectory).

## Local
- check maps/official passed. Server smoke v guard2 (seed 1): 17-17, queen-decided 10-6, end queen
  9.9 v 8.2, queen died 44% v 50%. Fresh mapgen (seed 46407, 44 maps x 2 seeds): 85-91; per family
  8-8 except maze 7-9, off-axis 6-10, sealed-spawn 7-9, rich-near-home 9-7 (16 games each: noise).
- Duels v ring-sprint-21: server 35-33 (queen alive at end 57% v 50%), official 7-13.

## Arena (ring-sprint-24)
- vs parent 54-0-58 (48.2%, noise): server 33-35, custom 15-19, official 6-4. Elimination 28-24,
  longer queen 19-23, longest 7-11. CPU p99 25.8M, max 56.8M. Eval r100-r400 0.50 (play identical before r430).
- Late (r430+) queen deaths in h2h games: 8 v 8. End queen length 6.4 v 7.4.
- Behaviour works as specified: in 19 decoded late deaths of 5+ queens across all its games, 18
  killers had NO segment in her view (Chebyshev 4-5 before her move, 2-3 after); before, in
  ring-sprint-21's deaths, 4 of 8 killers were visible (head or body). Near a visible 5+ enemy head
  her moves are away 34 / toward 26 (parent 29 / 21; 66 h2h replays).
- So the remaining mechanism is unseen killers just outside the 7x7 view.

## Iteration 2 (not submitted)
- Penalty 2 x (tiles out of her view within Chebyshev 2 of the endpoint) late. Duel v ring-sprint-21
  server 32-36; queen alive 60% v 57% but end queen 8.3 v 10.2: the sprint-gating trade the
  KNOWLEDGE note warns about. Reverted.
