# ring-qkeeper-1 (g43-4): one queen-keeper mode for every late queen-growth path

Base: ring-contest-m1 (= arena ring-contest-1 / ring-contest-4). Operator: mutate.

## Idea
arena-r17: the r430 gate (ring-qgate-1) only blocked Ring4Exit. Resign waves still fed a parked
queen who then walked out and died (C2: 166532 3->29, 167603 4->52); parks were wrong when the
queen's copy of the team's sighting was stale (B: 166732); an ahead queen kept chasing (D1: 168343).
server-r17 added: both queens alive at r430 with the gate shut -> ours a median 3 at r499 v their
17->25 (7-14); queen bleeding by repeated Split(2) waits (Portals 1150391: 19 -> 3, then dies).

## Change (ring.cpp KeeperLocal/KeeperMode/KeeperBits/HearKeeper, strategy.cpp QueenMove/Decide/Turn)
One mode, from the relayed enemy-queen state, read by every growth path:
- **PARK** (enemy queen known dead, or ours >= her last EXACT length + 3 + (499 - round seen), i.e.
  more than she can still grow at ~1/round): 1-step moves, adjacent eats only (no food term), held
  within 2+sqrt(len) of where she settled (penalty 4*excess^2); full step limit only if no first
  step is safe. No ring pearl split from r430. Workers: no feed14 resigns, no qrace walk.
- **TOPUP** (from r430, not dead, no race): a pearl within 2 safe steps takes her off the ring
  patrol; beam limited to 2 steps, the 2nd only onto a pearl; food term within 1; same home hold;
  no ring split. Workers: feed14 allowed, qrace walk allowed without a fresh sighting.
- **RACE** (EnemyQueenRace(), today's gate): today's behaviour; the r430 Ring4Exit only here.
- Exact enemy-queen length: her segments in view form one unbroken chain head->tail and the last
  is off the window border (the old `partial` flag marked nearly every sighting partial).
- Relay: the ring word was full (0-25 ring, 26-35 qgate, 36-55 call, **56-63 is the 0xBA magic** -
  the brief's bits 56-57 / 58-63 are not free). The call is only sent before r48 and read only when
  its surplus (48-51) is non-zero, so from r300 with surplus 0: 36-41 enemy-queen length, 42-43
  mode, 44 dead flag, 45-47+52-55 the length's age. A dragon that doesn't know the ring sends a
  bare ring word (w=h=0, all ring readers skip it) on two directions every other round from r400,
  only with a sighting from r300 on or a death, so sightings reach the queen without the champion
  word's 30-round freshness limit. (From r300 with any sighting it diverged games from r362.)
- Queen11 wait-split capped at 3 in a row, and skipped when the dead end is deep enough to last
  to r499 (server-r17 bleed).
Debug: -DKEEPER_DBG (KEEP mode lines, EQSEE sightings, QA queen actions r430+).
Labels: "keeper: park", "keeper: park run", "keeper: topup", "keeper: topup sprint".

## Iterations (light duels v ring-contest-1, server random:2 / official random:2)
- v1 (PARK margin 3+age/4, TOPUP r460): server 36-32, official 9-11. Replay o14 (stronghold):
  queen parked at 18 when ahead, enemy fed to 42 -> margin changed to "what she can still grow".
- v2: server 28-40, official 10-10. v3 (+TOPUP r430, qrace in TOPUP, wait cap): 28-39 / 11-9.
  Reproduced arena game portals seed 4610953476561058118 locally: v3 diverged from base at r362
  (relay only; all modes RACE) -> relay restricted to r400+ with useful news.
- v4: server 36-32 (queen 17-11, elim 14-15, 3+ starters 23-17, <=2 13-15), official 10-10.
- v5 (= submitted): TOPUP second step only onto a pearl (mixed-032 seed 2 walked 2 steps on 26 of
  35 turns). Server duel-8801efa2: 34-34 (elim 13-13, queen 16-16, longest 5-5; 3+ 19-21, <=2
  15-13); queen alive at end 60.3% v 57.4%, queen length at end 10.3 v 8.4. CPU max 35.4M.

## Local
- Fresh mapgen (seed 43042006, 44 maps, sim seeds 1,2, v ring-contest-m1), v4: 88-88. Per family:
  anti-centre 8-8, crowded 8-8, maze 8-8, mixed 6-10, off-axis 10-6, open 8-8, rich-near-home 8-8,
  ring 8-8, rooms 8-8, sealed-spawn 8-8, sparse 8-8 (mostly identical mirror pairs: the keeper
  rarely fires in self-play). v1 on the same maps: 92-84.
- maps/server seed 1, v1: 18-16. check maps/official passes.

## Replays / behaviour
- In self-play the keeper rarely fires (both queens race). Mode distribution, v1 maps/server s1
  (every 10 rounds): r300-429 RACE 166 / none 48 / PARK 1; r460+ RACE 51 / TOPUP 2.
- TOPUP, mixed-s43042006-032 seed 2 (enemy queen dead unseen, last seen r150): from r430 our
  ringed queen tops up 3 -> 20 by r477 on safe 1-2 step eats and qrace feeds (base: stays 3, dies
  ~r455); she died at r478 among 27 enemy dragons. With v5 the 2-step moves drop from 26 to 2.
- Relay: exact enemy-queen lengths are rare (weakhold s1: 1 exact sighting in 21); sonar hops are
  sparse (a r439 sighting reached one neighbour at r441, the queen never) - the relay helps little.

## Arena
Submitted as **ring-contest-7** (line ring-contest, parent ring-contest-4); not promoted.
- vs ring-contest-4: **54-0-58** (48.2%, CI 39.2-57.4), beats_parent False (within noise).
  By suite: server 33-35, custom 16-18, official 5-5. Decided by: elim 30-27, queen 15-23, longest 9-8.
- vs ring-sprint-1 53-59, ring-sched-17 66-46, ring-rush-1 70-1-41. All: server 144-1-127,
  custom 78-58, official 21-19. CPU p99 26.3M, max 48.0M.
- Eval P(win) v parent at r100/200/300: 0.500 / 0.500 / 0.500 (the code is identical before r300).
- **Weak spot, 272 games paired with ring-contest-4's on the same opponent/map/seed/seat:**
  B 6.2 v 7.4 per 100, C2 2.9 v 2.9, D1 7.4 v 7.4. Both queens alive at r430 (88 games): our
  queen ends 24.2 v 21.2 long and the race is won 49 v 46. Enemy queen dead before r430 (98
  games): ours survives 71% v 72%. So PARK hardly fires: the enemy queen's death is almost never
  seen, so the resign-fed walk-out (C2) is not fixed.
- **Side effect (decoded):** with the enemy queen dead but unseen, TOPUP from r430 plus qrace
  feeding grows the queen, and she dies. 179801: enemy queen dead at r93; ours 4 -> 16 from r430,
  lunged at r481. The parent stayed at 4 in its ring and won. 179195: 2 -> 9, died at r470; the
  parent survived. This undoes the qgate's "enemy queen dead" wins.
- **Bleeding not fixed (decoded):** 179191 portals, reproduced locally with ring-sched-m3 and the
  same seed. Our queen eats her way into a pocket (r471-477) and waits 3 times. One step forward
  is into the closed branch again, so the in-a-row cap restarts: 20 -> 2, dead at r492. A
  20-round window cap was tried locally: she enters after 3 waits, but the dead end is too shallow
  and she is boxed (split to 3) by r492 anyway. That fix isn't in the submitted code.
- Win flip with the intended behaviour: 179960 stronghold v ring-sprint-1. Ours goes 13 -> 58 from
  r460 and wins 61-38; the parent's game on the same seed is lost 24-55.
