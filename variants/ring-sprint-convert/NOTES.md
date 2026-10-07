# ring-sprint-convert (g46-3)

- **Line:** ring-sprint. **Parent:** ring-sprint-guard2 (arena ring-sprint-21).
- **The one change:** late conversion from r300 into ONE target at full throughput (strategist g46 C #1).

## Idea
s21 server losses: 53 queen races lost with material level at r400 (92 v 95) but queens 2 v 22;
18 Schooltime losses with both queens sealed at 3, lost on longest (22 v 54) while holding 314 v 162.
Our queen stays in her ring until r430 and the conveyor lays one line at a time; no champion is fed
while our queen is alive.

## Change (strategy.hpp "g46-3" block, conveyor.cpp, UpdateLateChampion, QueenMove, UpdateConvertQueen)
- From kConvertFrom = 300 the queen leaves her ring once she has seen no threatening enemy head
  (one that reaches a tile beside her head within ceil(L/4)+2, +1 if seen in part) for 20 rounds.
- She is the target while out of her ring, not sealed (kelp-closed region in memory with no ally head
  seen in it for 40 rounds) and the enemy queen is not known dead. She relays this in bit 30 of her
  report word (length field cut to 11 bits). Otherwise the target is the late champion:
  `UpdateLateChampion` no longer returns early while our queen is alive, and late concentration
  (champion harvest, no growth splits, reserved ground) starts at r300.
- The conveyor feeds the target from r300 with kConvertLines = 3 lines at once (3 walkers in line,
  "busy" at 3 sprint-turns of pearls), with no enemy-queen sighting gate.
- Guards: a worker lays itself only while units-1 >= max(3, their count as it sees it, half our count
  when conversion began).
- Small fixes it needs: the queen's opening "roaming" ends at r300 (its settle rule re-parked her),
  no "shed to six" split while she is the target, reserved ground from r300.

## Local (sim, vs ring-sprint-guard2)
- Server maps seed 1: 12-0-22; seed 2: 13-0-21. Queen alive at end 9.7/6.2 v 10.6/12.7; queen died
  62-65% v 35-38%. Games ending before r300 are identical (mirror pairs).
- Fresh mapgen (all families, seed 46301, 44 maps, seed 1): 36-0-52; died 65% v 57%.
- check maps/official: passed.
- Iterations: (1) any enemy head in view as threat + champion whenever not calm: 9-25, the flag
  flickered and the material went to champions; (2) reach-based threat, sticky once out: 14-20;
  (3) + guard cap (their count / half of ours): 12-22.

## What the traces show
- The conversion works mechanically: queens reach 24-77 by r350-380 (autarky 3->24 r300-355; maze
  4->77 by r380 with enemy queen at 3; portals/trauma won 33-42 v 19-31).
- But every queen death after r300 (autarky r337, maze r409, unsw r383, islands r309, slithery r469,
  australia r307; seed 1) is "queen12: direct attack" by the parent's workers: any enemy head that
  sees her and has a <= 12-step path kills her head-on, whatever her length. 130-200 rounds out of
  the ring instead of 70 is too long without queen armour (strategist C #3).
- Without the guard cap 46 dragons became 8 in 20 rounds (corpses feed half their length).
- A giant queen (77 on maze) boxed herself and shed 2 per turn by "queen: escape split" from r421.

## Arena
Not submitted: `tokenz ephemeral` was blocked by the permission system, and the local result is a
clear loss vs the parent anyway.
