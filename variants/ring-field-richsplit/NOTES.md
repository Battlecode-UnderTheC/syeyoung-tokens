# ring-field-richsplit (g49-1)

- **Line:** ring-field (arena draft `ring-field-7`, parent ring-field-3 = variants/ring-field-1).
- **Brief:** league/briefs/g49-1.md, strategist g49 #1 (EARLY, multiply on rich starts).

## Idea
arena-r23: 73 P1 economy losses sit on tiny or rich-spawn boards where the field mode never fires
(arena, fixed-tiny, wg39013, wg39006, wg39011, wg39001). We eat but don't multiply: splits by r60
13.6 v 31.1. Our split gates beyond gst's (contest race, rush split-once, founder no-split) hold
dragons at 4.

## Change (field.cpp UpdateRichStart/RichSplit, strategy.cpp Decide step 4 and QueenMove, header)
- **Rich dragon:** its own view on its first turn holds field.cpp's spawn field (kSpawnBeds = 4 live
  or soon-due beds). First turn = r0 for starters and, with RICH_CHILD, a child's birth turn before
  r60. Without RICH_CHILD the change is nearly inert: on 1-starter boards (arena) every worker is a
  child born after r0.
- **Worker:** until r60, while 2+ pearls lie in its 7x7 view (what a 2-long child needs to reach 4),
  it splits whenever GrowthSplit >= 0: no contest-race gate, no rush split-once, no founder no-split.
- **Queen:** on a rich r0 view with no enemy head in view, and not already foraging (forage35),
  she forages; she splits at 4 under qforage-1's rules (no threat on her head, no corridor,
  OpenRoom >= 20) with 2+ pearls in view; she returns to the ring at r60 or at max(6, 3 x starters).
- Labels: `rich: split`, `rich: queen split at 4`, `rich: settle split`. `RICH_MODE 0` = parent.

## Iterations
1. Literal brief (r0 dragons only): on server maps 6 rich splits per 34 games, play almost
   identical. Gate debug on 7 target maps: the race gate held only ~18 L4 turns in 28 games; the
   founder no-split ~46 turns with pearls in view. Added the birth-view children and the founder waiver.
2. Arena v1 (eph-0be1606b): fixed-tiny 0-4, our queen dead before r100 in all 4. On 7x7 with 3
   enemy heads her forage split never passes the threat test, so she held at 4 and lost the ring's
   r1 bootstrap split. Fix: no rich forage when an enemy head is in view at r0. Fixed-tiny local 4-4.

## Local
- check maps/official: pass.
- Fresh mapgen s49117 seeds 1-2 (v1): 88-88; s49233 seeds 1-2 (v2) 95-81, seeds 3-4 (v3) 84-4-88.
  No family a clear loss (worst: ring 6-10 v1, off-axis 6-10 v3 with identical r50 numbers).
- Target maps (7, seeds 1-2): 12-2-14 v ring-field-1, mostly seat-mirrored outcomes.

## Arena duels v ring-field-3
- v1: server 36-32, official 10-10, targets 13-15 (custom 0-4, fixed-tiny).
- v3 (eph-161c6e61, = ring-field-7): server 31-37 (metrics at r20/r40 identical to the parent),
  targets 16-12 (rotating 12-8, custom 2-2, official 2-2).
- Target games, ours v parent (rich.py): splits r0-59 28.6 v 22.3, dragons r20 6.3 v 5.6, r40 10.7 v
  7.8, eaten r10-29 21.3 v 17.8, queen kids by r25 1.86 v 1.71, queen dead <r100 .25 v .39, worker
  L4-unsplit 38.7% v 45.2%.
- Film bc28064a game 21 (wg39013, us seat B): 13 dragons v 4 at r20, 19 v 5 at r24. The queen splits
  at r8 and her children multiply in the south field.

## Arena verdict
(see below)
