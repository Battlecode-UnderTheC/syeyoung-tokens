# ring-sprint-nobleed (g44-5; parent ring-sprint-1; arena ring-sprint-5)

## Idea
arena-r18 S5: the queen splits herself to death while boxed in (178824 13 -> 3, 175726 10 -> 2).
Stop the bleed: keep room before she walks into a pocket, and don't split-wait when waiting
can't open a way out.

## What changed from the parent, and why (strategy.cpp QueenAction, strategy.hpp)
- Among queen routes with no escape (EscapeLevel 0), +kTrappedRoomWeight (50) per tile of room,
  so the room she keeps outweighs a pearl (12). Observed cause: 178824 took a pearl into a 2-tile
  pocket over a 5-tile one, then split 13 -> 3.
- "queen11: wait at closed corridor" (Split(2) at r430+) only when another dragon is beside her
  head (waiting can open a way). Walled by kelp and her own body she walks the branch instead.
  Observed cause: 176382 split 9 -> 3 at a kelp corridor mouth.

## Local (from the first run)
178824 repro keeps 12 at r374 (turns into the 5-tile room); 176382 repro walks the corridor
9 -> 12. Duels vs ring-sprint-1: server random:2 34-34, official random:2 10-10. Fresh mapgen
44 maps x 2 seeds 88-88; maps/server 34-34. CPU max 31.6M/39.1M.

## Arena verdict (ring-sprint-5)
- vs parent ring-sprint-1: 55-0-57 (49.1%, loses within noise). Server 33-35, custom 17-17,
  official 5-5. Decided by elimination 27-27, longer queen 18-19, longest 10-11.
- vs ring-contest-4 62-50, ring-sched-17 66-46, ring-rush-1 69-1-42.
- CPU: p99 24.2M, max 41.9M.
- eval P(win) vs parent r100/200/300: 0.500/0.500/0.500. vs the others: 0.515/0.529/0.528,
  the same as ring-sprint-1 on the same seeds (0.515/0.529/0.527).

## Step 6: did it fix S5 in the arena? No: the change almost never fires.
- 336 draft games vs contest-4/sched-17/rush-1, paired with ring-sprint-1's games on the same
  map, seed and seat: 312 are identical (same rounds, reason and result). Results flip 2 W and 1 L.
- Queen "no valid move" deaths: 17 for the draft v 18 for the parent (vs others), and 4 v 4
  inside the head-to-head games. S5-type deaths (last action a split, then resign) fall on the
  same seeds with the same lengths: 190320 = 175726 (weakhold, 10 -> 2 at r482),
  190550 = 175818 (islands, 10 -> 3 at r203), 190546 = 175819, 190463 ~ 182133 (portals 17 -> 3
  v 13 -> 3 at r490), 190266 ~ 175760 (weakhold 23 -> 3 v 20 -> 3 at r494).
- Local debug repro of 190320 (weakhold, seed 3981088957100987201, v ring-rush-1; local
  boxes at r472 rather than r478): every bleeding split is "queen10: boxed legal split"
  (no route at all), not queen11. Route debug: at r462-463 the region ahead was 38-43 free tiles;
  at r464-467 enemy (rush-1) bodies closed it to 6 -> 4 tiles; from r468 every route had escape 0
  and the choice among them was set by QueenRisk (an enemy head nearby), so she still ate 3 pearls
  (8 -> 11) in the sealed pocket, used up its last free tiles and split 11 -> 3, then resigned.
  The islands repro (190550) dies the same way ("queen10: boxed legal split" -> "queen: trapped").
- Tried a v2 (not submitted, reverted): in a sealed pocket (every free tile ahead seen, none past
  depth 12) keep 3 spare tiles, -50 per missing tile. It changed neither repro: the -50 is tiny
  next to the 10000 x QueenRisk and 1e6 escape terms, and by then she is already boxed.

## What to try next
The bleed is the end of a trap that closes 5-10 rounds earlier, mostly from enemy bodies
coiling around her, not kelp. A fix has to act before escape hits 0: e.g. treat a shrinking
free region around her (free tiles ahead falling turn on turn, below ~2x her length) as a reason
to leave, and in a sealed pocket weigh "certain death when the spare tiles run out" against
QueenRisk on the same scale.
