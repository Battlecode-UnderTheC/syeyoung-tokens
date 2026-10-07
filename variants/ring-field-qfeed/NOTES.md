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
  - **qgrow** = 20 rounds threat-free, the enemy queen not known dead, and her pen reaches L+1 tiles.
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
  - The existing "busy" test keeps it to about one line per round.
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

## Local (sim vs ring-field-1)
- `check ring-field-qfeed maps/official`: passed.
- **Server seed 1:** 21-0-13. Queen-decided 12-4. Queen at the end 16.7 v 8.9; died 47% v 44%.
- **Server seed 2:** 20-0-14. Queen-decided 10-5. Queen at the end 15.3 v 12.6; died 44% v 35%.
- **Fresh mapgen** (seed 48371, 44 maps, seeds 1-2): 87-2-87 overall.
  - Per family: anti-centre 7-9, crowded 8-8, maze 7-2-7, mixed 9-7, off-axis 8-8, open 8-8,
    rich-near-home 8-8, ring 8-8, rooms 8-8, sealed-spawn 9-7, sparse 7-9.
  - It is neutral because queens rarely live to r300 on these maps.
