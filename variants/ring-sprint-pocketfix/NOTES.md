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
  dragons' tiles count as open, so only kelp and her own body can prove a trap. Off at r499.
- The split credit (iteration 2): a split at 4+ counts as a way out only if her free slots
  exceed the largest one-round rise in team units over her last 10 turns. She acts first in the
  round (id 0/1), so the slots she sees can be filled before her next turn. Iteration 1
  (ring-sprint-29) trusted the slots she saw, and the queen still died at r300 in
  duel-b26b7005 game 1.
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
- Iteration 1, ring-sprint-29: even with the parent, 56-0-56 (server 34-34), identical play. It
  never fired: none of its 16 Schooltime games had the trap (no queen deaths on either side).
  Targeted duels on the bug seed (12385104951354685771) against the 8 bug opponents: our queen
  as B still died at r300 in all 8 (slots seen free, then filled).
- Iteration 2 (split credit from the unit tempo): the same 8 targeted duels. As B the queen was
  alive at the end 8/8 (parent 0/8). She dodged at r298 with a 2-step 'NW' onto the bed, 3 -> 2
  (film of duel-2e5d223a game 1). Record as B 1-7 (parent 0-8): crossover-1 is now a win, and
  the other 7 are losses on the longer queen, 2 v 3 instead of 0 v 3. Seat A 8-0, unchanged.
  Duel against rs21 on server, random:2: 34-0-34, identical. Fresh mapgen seed 48419:
  88-0-88, mirror.
- Iteration 2 submitted as ring-sprint-32: even with the parent, 56-0-56 (server 34-34,
  identical). Over all 448 games, paired against ring-sprint-29, the results differ only in
  227800 (spar-field-2, Schooltime, seat A): 'W longest 30-20' became 'L longer queen 3-2'. Both
  of the draft's own Schooltime dodges (227800 and 227674, r298) were unneeded, since a slot was
  still free at r300. Team units at her r298 turn: 58 (dodged, unneeded), 63 (dodged, unneeded)
  and 62 (the parent's death case). The slot count doesn't separate them; the cap fills or not in
  the two rounds after she decides. CPU max 40.9M.
- Trade-off on Schooltime: the opponent's queen also sits at 3 in a mirror pocket, so a dodge
  (2 v 3) loses the queen tiebreak about as surely as a death (0 v 3). It only pays when the
  opponent's queen dies later.
- Next: the queen can't keep 3 in a 4-tile pocket at the cap without a slot. A team-level slot
  reservation would give eat + split, 3 again later (a separate idea).
