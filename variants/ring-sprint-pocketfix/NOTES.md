# ring-sprint-pocketfix (g48-4)

- **Line:** ring-sprint. **Parent:** ring-sprint-guard2 (arena ring-sprint-21).
- **The one change:** a blunder fix in `QueenMove` (strategy.cpp): the queen doesn't take a
  route that leaves her with no legal move next turn once she grows by one.

## Evidence
arena-r22 REPORT blunder #7: ring-sprint-21 lost 8 Schooltime games at r300 the same way
(213334, 213623, 213318, 213351, 213149, 213601, 213343, 213310). Decoded 213334: the queen
(id 1) sat sealed in the kelp 2x2 at (56..57, 1..2) from r0, 3 long, cycling N/W/S/E. The bed
at (56,1) came due at r299. At r298 she stepped N and left (56,1) as her only free tile; the
pearl spawned there, her only move at r299 was to eat it, at 4 she filled the pocket and at
r300 she had no move. The team was at 64 units, so she couldn't split either.

## Why the base did it
Before r430 the queen's beam only makes free steps (max_steps = ceil(L/4) = 1 at length 3), so
at r298 'N' was her only candidate. No check looked one turn past the move, and the
pearl countdown was ignored.

## Change
- `trapped_after(body, eaten)`: from where the route leaves her, is there a next step n after
  which she still has a legal move? n's growth counts if n has a pearl now or its observed bed
  countdown is <= 1, which means it spawns before her next turn. Unseen tiles and other
  dragons' tiles count as open, so only kelp and her own body can prove a trap. Length 4+ with
  a free slot is never trapped, because she can split. Off at r499.
- A trapped route costs 2e6, more than the 1e6 space and escape penalties.
- If the best route is still trapped after the free-step depths, the beam runs up to 2 more,
  paid, steps (once). In the Schooltime pocket that means N+W at r298: she stands on the due
  bed at length 2, so the pearl can't spawn, and she cycles on. The label is
  `queen: pocket dodge (paid)`.

## Local
- `check ring-sprint-pocketfix maps/official`: passed.
- It never fires in normal local play. Without the unit cap, a 4-long queen splits; with the
  split case handled, no trigger on 34 server games against queen-ring17.
- Test hack (slots forced to 0 in the check only, since reverted), Schooltime against
  queen-ring17, 6 seeds: the dodge fired at r130, r290 and r314, every time as a 2-step onto the
  bed. Queen deaths 0%, queen at the end 2.7.
- Fresh mapgen (seed 48417, all families, 44 maps x 2 seeds) against ring-sprint-guard2:
  84-8-84, identical mirror play.

## Arena
- Duels against ring-sprint-21: server 34-0-34 and official 10-0-10, with identical metrics.
  The change doesn't fire in these games.
