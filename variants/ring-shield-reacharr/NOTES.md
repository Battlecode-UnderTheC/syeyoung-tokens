# ring-shield-reacharr (g53-15, arena ring-shield-9)

Base: ring-shield-bastion (arena ring-shield-3). The general half of g53-12 (ring-shield-heading) alone, without
its heading lean (FieldFanOut).

## Change (field.cpp, FieldBedsInView; FIELD_REACH=0 is the parent)
Field mode's bed/pearl counts (the arrival test, and the r0 spawn-field / home-keeper counts that use the same
function) count only tiles the walker may yet walk to: BFS from the head through every edge not known to be kelp
(an unseen plain edge counts open; a portal is followed once its far end is known), plus any tile with an edge never
seen. Cause: g53-12 trace, stripes: 4 pearls in a kelp-sealed 2x2 room ended field mode as "arrived" at r5-8 in all
four arena losses (275650, 275669, 275663, 275873). Also ported g53-12's FIELD_DBG trace lines (debug only).

## Local
- Traced stripes seeds (974.../814..., both seats v ring-contest-m1, debug build with a counter printing every turn
  where the reach filter changes the decision): the false arrival is gone in all four (parent: "arrived" r5/r8 at
  (17,3)/(14,3) as A, (6,8)/(9,8) as B). The filter flips 4-9 turns per game (beds 0-3 v raw 4-7). But the walker
  keeps its spawn heading along the band and arrives r13-14 at (9,4) / (13,7), the far room. First worker eat is
  unchanged: r25/r37 as A, r31/r29 as B (same as parent); eats by r30 on 974-A 3 v parent 6. All 4 still lost.
  So the false arrival was not what held the opening: the heading (g53-12's other half) was.
- How often it fires: 0 decision changes on 44 fresh mapgen maps (seed 531515, all 11 families, seed 1); among all
  server + official maps only stripes (3 flips, seed 1).
- Fresh mapgen seed 531515, 44 maps x seeds 1,2 x both seats v ring-shield-bastion: 86-4-86, every metric identical
  (the change never fires there), so every family is even.
- check maps/official: all passed.

## Arena
- Duels v ring-shield-3 (eph-5066143c): server random:2 34-0-34 (every map 2-2; 3+ starters 20-20, <=2 13-14 at 67/68),
  official random:2 10-10, all metrics identical to the parent's.
- Draft ring-shield-9 v ring-shield-3: 56-0-56 (server 34-34, custom 17-17, official 5-5), even. v ring-sealed-2 59-53,
  ring-field-3 59-53, ring-qforage-1 58-54. eval P(win) v parent r100 .500, r200 .497, r300 .505.
  CPU p99 23.3M, max 50.4M.
- Arena stripes v parent (280136, 280132, 280127, 280125) reproduce locally (rounds within 1): on seed 974 the A seat wins
  at r206 whichever bot holds it; on seed 814 the change alters play (A wins r312 with it, r226 without) but not the result.
