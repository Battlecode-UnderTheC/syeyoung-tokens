# ring-sprint-qdead2 (g45-7; parent ring-sprint-16 = ring-sprint-qroom)

## Idea
This iterates on g44-4 (ring-sprint-qdead). arena-r18 S1/C2: our queen dies after theirs because she
walks out at r430 and the conveyor feeds her, even though the queen race is already over. The base
learns that the enemy queen died only when she vanishes beside one of our heads, and it never relays that.
When her death is KNOWN, there is no race: no ring exit and no conveyor feeding. An old or missing
sighting is never read as a death (g44-4's parking on old sightings cost queen-tiebreak games), and
nothing parks the queen.

## Change (one behaviour: know her death, and then end the race)
- Witness (strategy.cpp Remember, as g44-4): her HEAD was in view last turn (`Sighting::head_round`),
  that tile is inside our 7x7 window now, and no segment of hers is in view -> she is dead this round.
- Trade claim (queen_attack.cpp AboutToTakeQueen / QueenTradeWord, plugin.cpp): a dragon that
  trades heads with her dies in that step and casts no sonar, so it claims the turn before.
  It claims only when she can't get away. She moves first next round, so every tile her head can
  stop on (her free steps, plus her own tile if she can split) must be inside next turn's 7x7 view and
  within the steps we may take (free steps, plus paid steps while length > 2, at most 12; TryQueenAttack's limit).
  If the claimant is still alive next round, it withdraws the claim (word "alive at X"). A claim not withdrawn by
  round X+1 counts as a death. v1's loose "within reach" claim was right 22 of 540 times in sim. Its
  withdrawals missed allies who had heard the claim: 105 false deaths in 34 games. In v2 a paid-step bug
  (length 2 can't pay) gave 7 false claims of 8. v3 (submitted): 3 claims, 1 death, 0 false.
- Relay: a known death rides in the ring word (bits 26-34, valid bit 35 clear). Dragons that don't
  know the ring send it in a new 0xBD word, on one rotating direction each turn.
- EnemyQueenDead() (a death later than every sighting) makes EnemyQueenRace() false, which gates
  Ring4Exit and the conveyor. The never-seen and old-sighting rules are unchanged.

## Local (sim)
- Self-play debug, server maps seeds 11-12 (34 games, 25 enemy-queen deaths):
  - 23 of the 25 were our queen12 direct attacks;
  - in only 10 did the attacker see her the turn before, and in none of the late (r300+) unwitnessed ones: long dragons lunge on the turn she enters view;
  - 14 witness detections, all real (+-1 round);
  - her death was known somewhere in 12 of 25 games;
  - of the 7 deaths before r430, our queen knew of 2 by r430.

  The rest have no witness at all, so no announcement can reach them.
- vs ring-sprint-qroom: server seed 11 17-0-17 (34 games, mirror results); fresh mapgen (22 maps,
  all families, seed 45707) 23-0-21, queen-decided 3-2. check maps/official passes.
- Duels vs ring-sprint-16 (eph-856dcb5a): server random:2 35-0-33 (3+ starters 21-19, <=2 14-14;
  queen 15-14, elimination 13-13, longest 7-6); official random:2 10-0-10.

## Arena verdict
(pending)
