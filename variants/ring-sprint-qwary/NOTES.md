# ring-sprint-qwary (g44-9; line ring-sprint, parent ring-sprint-6 = ring-sprint-conveyor)
Arena: ring-sprint-9 (iteration 1), ring-sprint-11 (iteration 2, this code).

## Idea
server-r19 S1: 26 of ring-sprint-1's 156 server losses are the queen killed while we were ahead.
The 2-3-long queen sits on her 2x2/2x3 loop; a visible enemy head hovers 1-4 tiles away; she
either keeps patrolling or, once the base's danger test fires, flees one step at a time by
QueenRisk (9 - distance), which walks her along a wall into a corner or a pocket.

## What the server replays show (decoded from bcweb fetches)
- Australia 1205181 r267-273: two 2-long hunters; she flees W along the south wall into the corner
  (17,28); at r273 every step is next to a hunter.
- QoS 1207483 r85-101: circles a 2x3 loop; a 3-long appears 3 tiles away at r98; she flees S, E,
  then S into a 1-exit pocket (13,21) and is lunged.
- QoS 1206849 r93-95: patrols S in a 3-wide lane toward a 2-long seen 4 away; pincered.
- Australia 1184985 r350-359: a 9-10-long hunter, mostly OUT of her 7x7 view; she flees from r355
  but can't outrun 3 free steps a turn.
- Of the 26 S1 deaths, the killer was 2-3 long in 15; in 5 it was never in her view (6-32 long).

## Change (evade.cpp, called first in QueenMove before RingQueen; QUEVADE_MODE 1)
For a queen of length <= 4 (one step a turn), not in the growth phase (queen_growing_):
- Threats: every enemy head in view. Its step budget is ceil(L/4) + (L-2): free steps plus paid
  steps (docs/game.md). A partial sighting counts as 8, QueenRisk's value. The queen acts first in
  the round, so her k-th tile is safe from an enemy when the path distance from its head (kelp and
  other bodies block, her own body is passable) exceeds budget + (k-1)*ceil(L/4).
- Trigger: some enemy can reach her current head within 4 of her turns. The horizon is the 2-3
  rounds the replays show a hunter taking to close from view range, plus one.
- Each legal single step is scored, in order, by:
  1. the turns it keeps her safe over a 4-turn search of her own steps (her body moves and pearls
     grow her);
  2. the worst margin at the first tile;
  3. room;
  4. a pearl.
- She leaves the loop only for a step that keeps her safe for at least this turn, and either for
  more turns than the loop's next tile, or for as many turns (short of the horizon) with a wider
  margin. Otherwise the base decides as before: ring patrol and splits, or the base flee when no
  step escapes.
- She plays safe:
  - no step onto a corridor tile (kelp on 2+ sides) unless she is already on one;
  - no step that leaves her a smaller open room than she has now (OpenRoom, capped at
    kForageRoom = 20);
  - no step that EscapeLevel calls doomed.
- No map keys and no board-size gates: it uses only what she sees this turn. CPU: arena max 41.9M
  (p99 25.8M).

## Iteration 1 (ring-sprint-9): a margin walk where nothing escapes
- Iteration 1 also left the loop when no step escaped (T = 0), walking her away by margin alone.
- In local traces (debug build, -DQUEVADE_DBG), 229 of 312 triggered turns were hopeless: long
  enemies have budgets of 6-11 steps. Example: weakhold r133-143 (vs ring-rush-1, seed
  5823891923403082162): 10 rounds walking away from a hunter she can't outrun, then lunged.
- Arena: 53-0-59 vs ring-sprint-6; NOT beats_parent.
  - Short-queen (length <= 4) head-on deaths, paired against the parent's games on the same map,
    seed, seat and opponent: 113 v 101. Those with P(win) >= .5: 44 v 32.
- Iteration 2 removed that mode.

## Iteration 2 (ring-sprint-11), arena
- Verdict:
  - The tool shows 237-0-175 "BEATS parent", but the arena's games listing returns each game up to
    11 times (448 unique ids among 1598 rows), so the verdict looks inflated.
  - Unique games: 57-0-55 vs ring-sprint-6. By suite: server 33-35, official 5-5, custom 19-15.
  - Vs the others (unique): ring-contest-4 64-48, ring-sched-17 72-40, ring-rush-1 76-36.
- Paired against the parent's games (336, same map, seed, seat and opponent):
  - W 212 v 210;
  - queen-first losses 75 v 83;
  - short-queen head-on deaths 99 v 101; those with P(win) >= .5 36 v 32;
  - dragons at r100/r200 26.4/34.9 v 26.4/34.9;
  - queen length at the end 9.1 v 9.3.
  - 259 of the 336 games are identical to the parent's (evade never moved her).
- Head to head vs the parent:
  - average game.eval (P(win)) at r100/200/300: 0.508/0.512/0.507;
  - queen died 58 v 59; queen-first losses 35 v 37.
- Light duel vs ring-sprint-6 (eph-693bfee9, server random:2): 37-0-31.
  - Queen alive at the end 61.8% v 54.4%.
  - Queen deaths 26 v 31; deaths to a visible enemy in reach for 2+ of the last 6 rounds 4 v 6.

## Replays (arena)
- QoS 199370 (draft) v 194906 (parent), seat A vs ring-rush-1. The games diverge at r33:
  - Parent: she splits on her loop at length 4 with a 2-long hunter close. She circles N, N, W
    into it as it closes from (8,28) to (8,25), and is hit at r41. The parent lost.
  - Draft: she steps S off the loop instead and splits at r35. The hunter goes away; she
    survives the game. The draft won.
- around_unsw 199719 (draft) loss: she is lunged at r369 by a 3-long that came up out of her view
  (last seen r365, 5 away). It is in view only on the fatal turn, when every step is in its reach,
  so evade defers. The trigger only sees heads that are in view this turn.

## Local
- check maps/official: all passed (both iterations).
- Iteration 1:
  - sim vs ring-sprint-conveyor, server maps seed 7: 17-0-17;
  - fresh mapgen (22 maps, all families, seed 4409, sim seed 11): 20-0-24;
  - duels vs ring-sprint-6: server 33-35, official 10-10.
- Iteration 2: vs ring-rush-1, server maps seed 21: 22-0-12, the same as the base (22-0-12).
  - Queen died 26% v 29%.

## Next
- Hunters that close from outside the 7x7 view (around_unsw 199719, server 1184985 and 5 of the 26
  S1 killers): she needs heads remembered for 1-3 rounds, with reach growing with age. This is a
  separate idea, and ring-plus-2's remembered heads lost on ring17.
