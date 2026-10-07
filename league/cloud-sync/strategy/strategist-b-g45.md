# Strategist b (contrarian), g45 (saved by the orchestrator from the agent's final message)

Re-cut server-r19's per-game features (data/feat_*.jsonl joined to analysis/rows.json, all 370 ring-sprint-1 games) and read the head's code (variants/ring-sprint-conveyor). Numbers without a report citation come from that re-cut (script: the session scratchpad's q.py).

## What the re-cut shows
- **The opening gap starts in r0-30 and is eating, not dragon count.** 3-4-starter losses decided by r60 (25): dragon-turns ~equal (198 v 210); eaten 8.6 v 13.5 (4.4 v 6.4 per 100 dragon-turns); in 3-4-starter wins 10.4 v 10.7; from r30 their dragon-turns explode (397 v 243, then 885 v 397). 2-starter early losses: 6.2 v 9.5 eaten in r0-30.
- **The parked queen is one idle starter from r0.** Her r0-30 circling share 0.44/0.67/0.30/0.45/0.80/0.70/0.79/0.77 on 2/3/4/5/6/7/9/10-starter maps; opponents' queens 0.06-0.30. On a 3-starter map she is 30 of our 198 dragon-turns in r0-30 and eats ~nothing. Early-loss rate falls as her share of the workforce falls (29% on 2 starters, 26% on 3, 29% on 4, 11% on 5, 15% on 6, 9% on 7, 10% on 9, 0% on 10; map size is a confounder).
- **Her safety is worth something.** On 3-4-starter maps our queen is dead before r100 in 7% of games, theirs in 16%; when theirs dies before r100 we win 12 of 15. First contact is r0 there, so a 'settle at first sighting' trigger would never let her work.
- **Home food is not a safe bank.** In the arena they ate 12.4 on our side in r0-30 while we ate 11.6 at home; home-first only cedes the middle (the picket covers the contested band).
- **C2 losses are conversion, not supply.** In 12 of 21 our final total was higher: 1192234 Around UNSW 413 v 123 (longest 36-42); 1202754 Australia 252 v 63 (28-35); 1186537 Islands 226 v 66 (21-25); 1212512 Around UNSW 288 v 148 (25-32); 1184985 Australia 253 v 125 (37-48). 94% of late resigns die more than 3 tiles from our longest (server-r19 §8).

## 1. The queen works through the opening on every start, then parks (T1)
- Assumption questioned: the ring queen parks from r0 whenever another starter can split (FORAGE35 forages only when she is under 4 at r0, no ally in view is 4+, and fewer than 6 dragons), so on 3+-starter maps she parks at r0.
- Evidence: r0-30 circling 0.67 on 3 starters v their 0.28; eating 4.4 v 6.4 per 100 dragon-turns in early losses; opponents' queens circle 34% overall v ours 80%; devtest's queen eats 5.3 by r100, ours 1.9; forage was real where allowed (ring-plus-1: 2-starter pearl share before r60 0.34 → 0.56, eliminations before r200 17/58 → 2/29); the guarded queen as 1 of 4 starters stalls the opening by r20 (queen mode off: Stripes 30-30 v 1-59). Devil 9.5 v 20.5 dragons at r60; Default 8 early losses; Schooltime/Portals/Prisoners Dilemma 17. Never tried: forage on 3+ starters in the ring line.
- Mechanism (QueenMove): at r0 drop FORAGE35's `!producer` and `units_ < kForageSettleUnits` gates (every queen sets queen_forage_ = queen_growing_ = true); generalise the founder's 'queen split at 4' rule beyond small_start_ (it already requires open ground, no threat on her head, no corridor, OpenRoom ≥ kForageRoom); settle via the existing 'forage35: settle split' path at units_ ≥ 3 × units at r0 or r120, whichever first (2 starters → the current 6, unchanged). Existing threat map and corridor/pocket avoidance stay. No danger settle (g37-4 lost 35-49: she parked at length 2 beside danger).
- How to tell (3-6-starter maps): queen circling r0-60 from ~0.7 to ≤0.4; pearls r0-30 and dragons at r30/r60 up; eval at r60/r100 up. Not worse: queen death before r100 on 3-4 starters (now 7%; abort above ~12%); queen-decided record; 2-starter results (byte-identical).

## 2. The late champion goes to the material (C2, T2)
- Assumption questioned: feeders walk to the champion; 94% don't arrive, yet a 30-long champion has ceil(L/4) = 8 free steps.
- Evidence: C2 21 server losses, 12 with more material; led after r100 in 20 of 21. Devtest cull-and-harvest: champion or queen sprints through a dying cluster (3 → 62, 12 → 34). Differs from dead ends (walking feeders to stale reports g3-6, holding the champion still, regional champions, early non-queen champion feeding g38-7): starts from the existing r430 late concentration; only the champion's goal changes.
- Mechanism (ChampionHarvest, only while late_conc_: our queen known dead, r ≥ 430): ally material as food in the beam's value (each ally report ≤10 rounds old, length ≥ 2, not held (under 80% of the champion), counts its length, distance-discounted like pearls); when no reachable pearl field beats it, the champion sprints toward the densest cluster. Feeders resign within kLateFeedDistance (3) of its head as now. Lunge guard and escape-run vetoes stay ahead.
- How to tell (both queens dead at r430): our longest/total at r499 up (arena-r14 0.14 v 0.27); resigns within 3 of the champion up; C2 record better. Not worse: champion deaths after r430; total material at r499. Live-queen games byte-identical before r430.

## 3. Food we see but can't reach prices the unknown portals near it (T1: portal-gated food)
- Assumption questioned: an unknown portal is just another frontier tile. A visible pearl with no known path has paths.dist < 0 and scores zero; an unknown portal earns only explore_value / (d + 1).
- Evidence: Stripes 1195116 [img] (their dragons enter the 2x2 boxes through portals; our first box meal ~r24; r32 2/6 v 5/10); arena S4 (175461), Stripes ext 30-31; portal steps r0-60 of early losses 2.2 v 6.7 on 2 starters, r60-100 2.3 v 12.8 on 3-4 starters; 2100+ opponents 138 v 67 a game. Differs from contest-13 / ring-sprint-4 (52-60: known portal edges made ordinary; Trophy's portal ends unknown) and the poor-home push (all portals): only portals near food we see but can't reach.
- Mechanism (workers only; the queen never explores unknown portals): before r100, sum the value of each seen pearl or due bed with no known path; credit it to reachable unknown-portal frontier tiles within 6 torus steps of that food: score = max(current, sum / (d + 1 + 2)). Crossing keeps the ping / exit-clear / length gates.
- How to tell: portal steps r0-60 up; gated-room pearls eaten up (contest-13's metric); r30/r60 dragons up on portal maps. Not worse: deaths on crossing turns (contest-13 saw Islands 9.0 v 5.2); maps without portals identical.

## 4. The ring sits on beds (T2 queen race, some T1)
- Assumption questioned: the ring selector's `score -= 2` per bed tile (inherited from queen-ring17, never tested) makes the ring a place where she can't eat.
- Evidence: queen median 2 at r200/300/400, circling 80.9% v 30.3%; devtest's queen eats 5.3 by r100, ours 1.9; B 24 server losses (their queen +21-24 from r400, ours +6-8); top teams win 94%/78% with the queen 8+ at r200.
- Mechanism: in RingQueen's ring choice, bed term −2 → +2 per tile with a countdown. Everything else stays (exit and nearby-head terms, danger exit, pearl split at 5).
- How to tell: queen pearls r0-200 up; 'ring: pearl split' children up; queen length r200/r400 up; B count down. Not worse: queen deaths r0-430, especially S1 circling deaths (beds draw enemy foragers; test on top of S1's fix if it lands).

## Not proposed
- Contested-band valuation from mirrored starters (the picket covers it).
- Settling the queen on danger (g37-4 lost 35-49; queen died 42% v 24%).
- Earlier or gated champion feeding (g38-7, g16-8, g17-9 lost).
- Hunger or unseen-area exploration multipliers (hurt on 3+-starter maps, g37).
