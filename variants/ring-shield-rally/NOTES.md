# ring-shield-rally (g53-18, arena ring-shield-14)

Base: ring-shield-bastion (arena ring-shield-3). Strategist D g53 #3, "rally at contact".
(Parent's notes: variants/ring-shield-bastion/NOTES.md.)

## Idea
In 2-starter opening collapses our heads are outnumbered within 4 tiles at 49% of contacts (1.77 v 2.41); theirs are outnumbered at 3%.
Every coordination rule spreads our workers out, while theirs arrive in groups.

## Change
- call.cpp: before r40, a find with enemy heads within kCallNear (= kContestReach, 4) becomes a contested call.
  - It carries a rally flag: bit 9 of the 0xA9 word, and bit 51 of the ring word. In the ring word the surplus field is now 3 bits, capped at 7.
  - Its answerer cap is max(bed surplus, their heads + 1).
- A worker answers a rally even when its goal is food, or when it is in a contest race.
  - It must not have seen the cluster since the find, so the finder is not an answerer.
  - The way there (known walk, then straight line: CallGoal's eta) must be within kRallyReach (12).
- founder.cpp: FounderGoal drops a region that an enemy head reaches first only when the enemy heads within 12 of the region outnumber ours (us included).
- Iteration 1: the finder no longer answers its own rally. Before this, Trophy 265787's lone walker was drawn to the cross held by 3 orange heads.
- Iteration 2: reach is measured by the eta, not the straight line. In duel maze game 48, a worker answered a rally 12 tiles away across the wrap.
- Debug: -DRALLY_DBG.

## Local
- maps/server, seed 1: 19-0-15.
- Fresh mapgen maps (seed 531800, 44 maps, seeds 1 and 2): 89-4-83.
  - Per family: anti-centre 14-2, crowded 7-9, maze 6-4-6, mixed 8-8, off-axis 9-7, open 6-10, rich-near-home 8-8, ring 9-7, rooms 8-8, sealed-spawn 7-9, sparse 7-9.
- rtrace (both games reproduce exactly with the parent):
  - Devil 267190: no change. The rally is found at r15 (15,8), but sonar never carries it to the four farmers.
  - Trophy 265787: id3 (r25-27) and id13 (r33) answer the founder's contested find.
  - The founder still drops the cross: 4-5 enemy heads against 1-2 of ours.

## Arena
- Duels v ring-shield-3 (server suite, 2 seeds):
  - iteration 0: 30-38;
  - iteration 1: 30-38;
  - iteration 2: 34-34.
- Official suite, iteration 0: 8-12.
- Verdict ring-shield-14 v parent: 56-0-56, even.
  - By suite: server 34-34, official 5-5, custom 17-17.
  - v ring-shield-10 60-52, v ring-sealed-2 60-52, v ring-field-3 68-44.
- eval P(win) v parent: r100 .511, r200 .525, r300 .504.
- CPU: p99 24.1M, max 50.4M.
- Contacts at r6-50 (op3), over the draft's games v the parent: outnumbered 21.4% v 22.8%; own4 3.37 v 3.23; band head-rounds 9489 v 9715.
- Decoded arena games (rtrace reproduces the draft move for move):
  - Rally answers v the parent's call answers: 286131 11 v 2, 285930 10 v 4, 285761 47 v 23, 285783 73 v 40, 286188 8 v 0.
  - 286131 (trophy, a loss): id2 walks north (11,19) to (11,13) at r32-38 to join the contested cluster, and id14 answers at r31-32. Both are among the five dragons dead by r61, so the rally fed the contact rather than winning it.
  - 285783 (default, a win): id7, id13, id16, id9 and id11 converge on calls at r14-28.
