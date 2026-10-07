# ring-field-qfeed (g48-3)

- **Line:** ring-field. **Parent:** ring-field-1 (arena ring-field-3).
- **Brief:** g48-3, strategist g48 #3: END GAME, the queen is fed in place from r300.
- The parent's notes (ring-field-1, g46-1) are in git history (variants/ring-field-1/NOTES.md).

## Idea
- arena-r22 P3: 151 losses in queen-length races.
- On the server the enemy queen is 20 at r400 and 34 at r450, while ours is 2-3 until the r430 ring exit.
- docs/analysis-2026-10-07 §8: a threat-free queen (no enemy head able to reach her within its reach + 2
  for 20 rounds) dies 3.6% per 30 rounds, against 30.7% when threatened.
- g46-3 grew her OUT of her ring from r300 and she died (62-65%). So here she grows inside her pen.

## Change
- **Queen** (strategy.cpp QueenMove, ring.cpp RingQueen, conveyor.cpp QFeedUpdate), r300-429:
  - **Pen:** tiles within kQFeedPen = 3 of the last ring she patrolled. It is redrawn whenever she
    patrols a ring.
  - **Threat clock:** QThreat(head, 2) is an enemy head in view with a free path to her within its
    strike reach + 2.
  - **qgrow** = 20 rounds threat-free, the enemy queen not known dead, and her pen reaches 2L+4 tiles
    (her body plus as many free tiles again + 4; L+1 until iteration 3).
  - **While qgrow:**
    - no ring split at 5;
    - when a pearl lies in the pen off the ring, she leaves the patrol for the harvest beam;
    - eating a pearl where QueenSpace < L+4 costs 24.
  - **Pen hold:** the beam charges 1000 per tile beyond the pen (below every risk and escape term). It
    applies whenever no enemy can reach her this turn. The food term counts pen tiles only.
  - **Paused** (not qgrow): no food term, and each pearl eaten costs 24.
  - QUEEN_SHORT shed-to-6 is off from r300.
- **Paused bit:** bit 30 of the champion word (lengths are capped at 2047). The queen's self-report
  carries it, and relays keep it.
- **Feeders** (conveyor.cpp, early branch r300-429): the g44-7 conveyor runs early for idle stubs.
  - Idle stub: length 2-3, at least 2 rounds old, no food goal within kQFeedWalk = 20 rounds last turn.
  - Walk at most 20 steps.
  - They lay even while she patrols her ring.
  - They don't feed if they see a threat to her (QThreat(target, 2)), if her report says paused, or
    if the enemy queen is known dead.
  - **Busy test:** early, every pearl within kQueueDist = 5 (Manhattan) of her head counts as pending
    (iteration 3), so about one line goes down a round.
- **Reserved ground:** the kReserve ground around the champion starts at r300 when the champion is
  our queen. Before, it started at r360.
- **r430+ is unchanged.** A grown queen is out of her ring, so Ring4Exit doesn't fire, and the beam
  harvest goes on as before.
- `QFEED_MODE 0` gives the parent's behaviour. `QFEED_DBG` prints a per-round queen trace (QF) and
  worker resigns (QL).

## Iterations (local)
1. **First build:** server seed 1 16-18, queen died 53% v 38%.
   - Portals trace: she grew 4 → 17, but feeders kept laying while she was paused.
   - She ate anyway, outgrew the pen, and the room rules pushed her out to 6 tiles; she died at r391.
2. **Added the paused bit:** feeders hold off while she is paused.
   - Portals: she held 11-14 in her pen from r330 to r430 (never more than 2 tiles out) and won both seats.
   - Submitted as ring-field-4.
3. **After ring-field-4's games** (see Arena):
   - Growth now needs the pen to reach 2L+4. With L+1 she coiled in a full pen; local stronghold,
     portals and maze traces showed boxed and escape splits of 17 -> 15 -> 9.
   - Early lays are capped by Manhattan pending.
   - Tried and dropped: pen cost 20 a tile instead of 1000, which let room pull her out. Server s1-2
     went 35-33 with queen deaths up, v 39-29 at 1000.
   - Submitted as ring-field-5.

## Local (sim vs ring-field-1)
- `check ring-field-qfeed maps/official`: passed.
- **Server seed 1:** 21-0-13. Queen-decided 12-4. Queen at the end 16.7 v 8.9; died 47% v 44%.
- **Server seed 2:** 20-0-14. Queen-decided 10-5. Queen at the end 15.3 v 12.6; died 44% v 35%.
- **Fresh mapgen** (seed 48371, 44 maps, seeds 1-2): 87-2-87 overall.
  - Per family: anti-centre 7-9, crowded 8-8, maze 7-2-7, mixed 9-7, off-axis 8-8, open 8-8,
    rich-near-home 8-8, ring 8-8, rooms 8-8, sealed-spawn 9-7, sparse 7-9.
  - It is neutral because queens rarely live to r300 on these maps.

- **Iteration 3** (the ring-field-5 code):
  - Server s1 21-13, s2 16-18. Queen deaths 41% / 53% v 41% / 38%; queen at the end 17.1 / 9.9 v 10.9 / 11.3.
  - Mapgen (same maps): 88-2-86. Per family: anti-centre 7-9, crowded 8-8, maze 7-2-7, mixed 10-6,
    off-axis 8-8, open 8-8, rich-near-home 8-8, ring 8-8, rooms 8-8, sealed-spawn 9-7, sparse 7-9.
  - These family numbers are with the pen rule only (before the Manhattan cap).

## Arena
- **Light duels vs ring-field-3 (iteration 2, eph-178ad622):**
  - server 40-28: elimination 15-12, queen 14-11, longest 11-5; 3+ starters 23-17, 2 or fewer 17-11;
  - official 8-12;
  - longest@400 14.1 v 12.3; queen length at the end 12.9 v 11.2; queen alive at the end .50 v .54.
- **ring-field-4 verdict (iteration 2):** 56-0-56 vs parent (noise).
  - Server maps 34-34. Decided by elimination 29-26, longer queen 19-17, longest 7-12.
  - vs ring-qforage-1 63-49, ring-sprint-21 59-53, spar-field-2 64-48.
  - CPU p99 24.7M, max 42.7M.
- **Targeted situation** (ring-field-4 v ring-field-3, the same 112 games, our queen alive at r300: 67 v 67):
  - queen length r350 / r400 / r430: 10.4 / 13.0 / 14.4 v 2.8 / 2.6 / 2.7;
  - queen deaths r300-429: 19 v 17; r430+: 8 v 12;
  - she strayed more than 5 tiles (Chebyshev) from her r300 head in 38 of 67 games (the parent 7).
- **Eval P(win) v parent:**
  - r100-300: .500 (the games are identical until r300);
  - r310 .54, r350 .54, r400 .57, r430 .59, r450 .57, r490 .50.
  - The edge built by r430 is gone by the end.
- **Film 228416 (Stronghold, B):**
  - She grows 7 -> 16 -> 24 -> 37 in r310-400, inside our territory with our bodies all round her.
  - At r310 a wave of stubs died around her in one round (fixed in iteration 3).
  - After r430 she reached 68 at r476, coiled across the whole region, and boxed-split down to 26.
    She lost 26-60 to the parent's queen.
  - That late collapse is in the unchanged r430+ logic (a side effect of the bigger queen).
- **Other games:**
  - 228629 (maze): 23 -> 29 -> 27 by r430, 87 at the end, won 87-48.
  - 228700 (trauma): 8 at r430, lost 26-33. The parent's queen out-grew ours after r430.
- **Duel (iteration 3, pen rule only, eph-ca4480b0):** server 34-34. Queen length at the end 12.2 v 10.1.
- **ring-field-5 verdict (iteration 3, the committed code):** 54-0-58 vs parent (noise).
  - By suite: server 31-37, custom 17-17, official 6-4. Decided by elimination 26-28, longer queen
    20-17, longest 7-12.
  - vs ring-qforage-1 62-50, ring-sprint-21 62-50, spar-field-2 63-49. CPU p99 24.7M, max 42.7M.
  - **Targeted:** queen r350 / r400 / r430 8.1 / 11.2 / 10.4 v 2.8 / 2.9 / 3.0. Deaths r300-429: 19 v 18.
    Strayed more than 5 tiles in 37 of 67.
  - **Eval:** r350 .54, r400 .56, r430 .57, r450 .62, r490 .55.
  - Iteration 3 did not fix the stray or the deaths and grew her less. ring-field-4 (commit f75ef65)
    grew her more for the same W-L.
- **Not fixed (next):**
  - She leaves her pen in more than half the games. Fleeing threats and the room rules move her, and
    the pen is redrawn whenever she re-rings.
  - Queen deaths in r300-429 are about the parent's (19 v 17-18), not lower.
  - A big queen after r430 coils and boxed-splits (228416: 68 -> 26). The r430+ harvest isn't built
    for a 30-60-long queen.
