# ring-field-qgrowgate (g52-19)

- **Line:** ring-field. **Parent:** ring-field-qfeed (arena ring-field-9).
- **Brief:** g52-19, strategist B g52 #7. The parent's notes (g48-3 iterations 1-4) are in git
  history (variants/ring-field-qfeed/NOTES.md).

## Idea
- server-r26: 47 longer-queen losses with both queens alive (Trauma 13, Slithery 7, Portals 7, PD 6).
  The enemy queen was 7 at r300, 17.6 at r400, 24 at r430 (Trauma).
- ring-field-9's pen growth answers that (10 v 3 at r430), but was even in the arena: relatives keep
  their queens at 2-4 until r420 and the growth cost survival there.
- So growth needs evidence that the opponent races its queen early.

## Change (one)
- **Trigger** (conveyor.cpp UpdateQTrigger): from r150 (kQTrigFrom), any of our dragons saw the enemy
  queen (id 0/1, other team) at length >= max(8, ours + 5).
  - 8: twice the 2-4 that ring relatives hold their queens at; +5 a lead over ours that a ring split
    at 5 can't close. "Ours" is the queen's own length, or her reported length for a worker.
  - The max sighted length (capped at 31) rides in ring-word bits 36-40, next to the sighting round
    (bits 26-35). It is read only when the word carries no cluster call (bits 48-51 zero; calls end by
    r48). Every ring-word sender relays its max. The trigger latches (her growth must not switch it off).
- **Without the trigger (r300-429)** she plays ring-field-3's ring: QFeedWindow() is false, so no pen hold,
  no pen food, no pearl costs, no qgrow (the ring split at 5 stays on). Her report says "paused", so feeders
  hold off; the early conveyor branch and the r300 reserved ground also need the trigger; queen shed-to-6
  stays on until r430 as in ring-field-3.
- **With the trigger:** ring-field-9 exactly. **r430+:** ring-field-9 exactly.
- `QGROW_GATE 0` gives ring-field-9. `QTRIG_DBG` prints the first latch per dragon and the queen's state at r429.

## Local
- `check ring-field-qgrowgate maps/official`: passed.
- **v ring-field-1 (= arena ring-field-3), server s1:** 18-16; queen-decided 9-7; queen at the end 11.5 v 8.9,
  died 38% v 44%. Trigger at r429: 0 of 18 surviving queens (enemy queen max seen 0-4). The 20 latches
  were all r445+, after the parent's queen left its ring (no effect after r430). So it is rf3 v rf3 to r430.
- **v ring-field-qfeed (base), server:** s1 14-20 (queen 5-9, elimination 7-7); s2 15-19 (queen 6-7,
  longest 2-5). Total 29-39, about 1.2 sigma.
  - s2: trigger fired in 6 of 17 surviving queens, latched about r314-317, queen 9.8 at r429.
  - The base's queen grows in her own pen, often out of our sight, so many races go untriggered.
- **Fresh mapgen** (seed 52193, 44 maps, seeds 1-2) v base: 92-1-83. Trigger at r429 in 1 of 46.
  - Per family: anti-centre 10-6, crowded 8-8, maze 7-9, mixed 8-8, off-axis 8-8, open 8-8,
    rich-near-home 9-7, ring 8-8, rooms 8-8, sealed-spawn 7-9, sparse 11-1-4.
- **Not run:** v codex-queen-value-1 (not local; `tokenz get` timed out in the sandbox).

## Arena
- Not submitted by the worker: `tokenz ephemeral` timed out (no output, 120s and 300s) in the sandbox.
  The bot is ready for the orchestrator to submit.
