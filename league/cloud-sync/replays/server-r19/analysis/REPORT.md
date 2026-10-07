# Where ring-sprint-1 loses on the server (server-r19, 2026-10-06)

The analysis agent couldn't save this file, so the orchestrator saved it from the agent's final message. Data, scripts, tools and img/ (14 filmstrips) are in this directory.

## Summary
- **Sample:** all 370 games of ring-sprint-1 (rs1, submission 17933), 214-156, 10-05 18:08 to 10-06 05:43, against all 166 games of ring-contest-1 (rc1, 88-78), from the arena's server bridge (`/api/v1/server/battles`). Every game decoded; 13 filmstrips viewed.
- **Performance (opponents' ratings at battle time):** rs1 **2046±20** v a mean opponent of 1984; rc1 **2035±30** v 2015.
  - Ranked games only: rs1 2096, rc1 2069.
  - On the 12 common opponents: rs1 2019 v rc1 2009; paired win-rate difference **−0.9 points**.
  - **The free sprints moved nothing measurable on the server.** The server rating of 2136 sits about 90 above measured performance: it counts ranked games only, and 165 of 370 games were unranked scrims (50 against kraken).
- **Loss causes, ranked:** 1. opening snowball, decided by r60: 80 of 156 (51%); 2. queen blunders: 36 (23%); 3. late queen race and longest tiebreak: 52; 4. mid-game eliminations: 11.
- Devil and Trophy are pure opening losses: 27 of 30 losses there are decided by r60.

## 1. Rating-adjusted performance

| tier | rs1 W-L | rs1 perf | rc1 W-L | rc1 perf |
|---|---|---|---|---|
| 2250+ | 8-19 | 2199 | 7-13 | 2232 |
| 2150-2249 | 15-22 | 2096 | 3-12 | 1936 |
| 2050-2149 | 31-35 | 2079 | 23-17 | 2142 |
| 1950-2049 | 30-27 | 2044 | 15-9 | 2074 |
| <1950 | 130-53 | 2000 | 40-27 | 1918 |

Half the rs1 sample is opponents under 1950. Common opponents: rs1 lost ground to Delusion Tax (1-4 v 4-1) and Computers (0-5 v 1-4), gained on test4, Weneedmoretokens, fandagong; neither significant.

**Per map (rs1 perf; rc1 in brackets):** weakest Devil 1717 (1733) 4-14 · weakhold 1844 (2021) 10-15 · Trophy 1872 (rc1 0-9) 10-16 · Australia 1955 (2140) · Trauma 2000 · Schooltime 2000 (1862) · QoS 2003. Strongest Maze 2468 (2149) 21-2 · Prisoners Dilemma 2305 · Around UNSW 2235. (The server's own weakhold rating, 2167, reflects that its losses were mostly unranked games against kraken.)

**Map classes (rs1; no 1-starter maps in the pool):**

| class | maps | W-L | perf |
|---|---|---|---|
| **2-starter cramped** | Stripes, Trophy | 21-24 | **1953** |
| 2-starter open | QoS | 10-12 | 2003 |
| 2-starter corridor | Maze, Tower Defense, Trauma, weakhold | 48-32 | 2047 |
| 3+-starter cramped | Devil, Portals, Prisoners Dilemma | 37-24 | 2059 |
| 3+-starter open | 7 maps | 98-64 | 2074 |

2 starters 79-68 (2011); 3+ starters 135-88 (2070).

## 2. Loss causes and where P(win) turns
Turning point = the round from which the arena eval's P(win) stays below 0.5. Losses turned ≤r60: 80; r61-150: 14; r151-300: 17; r301-430: 30; r431+: 15. Cramped maps turn early (37 of 48 by r60); open maps late (32 of 76 after r300). Against 2250+, 13 of 19 losses decided by r60; against <1950, 23 early and 23 after r300.

| cluster | n | per 100 (rs1 / rc1) | where |
|---|---|---|---|
| A. Early collapse (our r100 total length < 60% of theirs) | 76 | 20.5 / 22.9 | Trophy 15, Devil 12, Stripes 8, Default 8, QoS 6, Trauma 6. Median turn r20 |
| B. Queen race lost, both queens alive | 24 | 6.5 / 9.0 | Slithery Fight 8, Trauma 4. Median turn r230 |
| C2. Longest dragon, both queens dead | 21 | 5.7 / 4.2 | Islands 5, Australia 5. Led after r100 in 20 of 21 |
| D1. Our queen died first | 17 | 4.6 / 6.0 | weakhold 7 |
| M. Mid/late elimination | 11 | 3.0 / 2.4 | QoS 4 |
| C1. Longest, queens tied | 7 | 1.9 / 2.4 | all Schooltime |

## 3. STUPID losses (blunders): 36 of 156 (rc1: 14 of 78)
Our queen is length 2-3 most of the game (median 2 at r200/300/400). 97 of 164 queen deaths are at length 2-3, killed by 2-3-long enemies 1-3 tiles away.
- **S1. Queen killed while we were ahead (P ≥ 0.5 ten rounds before): 26 losses.** She keeps circling her 2×2 ring beside a visible hunter (Australia alone 5).
  - [img] Australia 1184985 r351-359 (P 0.90): queen (2) alone in open ground circles the same 2×2; a 10-long enemy hovers 2-4 tiles away for 7 rounds; she doesn't move off; head-on at r359.
  - [img] QoS 1207483 r93-101 (P 0.60): circles while a 3-long enemy is 1-2 tiles away for 4 rounds; lunged.
  - [img] weakhold 1186540 r482-490 (our queen 9 v their 4, a won game): leaves to eat, walks to 3 tiles of a freshly merged 27-long enemy; lunged r490.
  - Also Around UNSW 1212512 r460 (P 0.98); Default 1202855 r405 (P 0.82); Devil 1187188 r412 (P 0.80).
- **S2. The queen splits herself to death or resigns boxed: 10 losses** (3 while ahead).
  - [img] weakhold 1204754 r472-480: in a 1-wide corridor 7→5→3, eats, 4→2→3→4→2; resigns r480, boxed by her own split children; our queen had been longer.
  - [img] weakhold 1202866 r477-485: same corridor and pattern (8→6→4→2…); our 13-long champion against her flank.
  - [img] Islands 1212978 r121-129: at 57-60 dragons near the 64 cap, among our own bodies, 6→4→2, resigns r129 (P 0.54).
  - Others: Portals 1191779 (r205), Islands 1199410, Australia 1198397.

## 4. STRATEGIC losses
- **T1. Opening snowball (cluster A, 76).** In losses our total length is 21 v 35.5 at r60, 28 v 56.5 at r100. They split more in r30-100 (18.5 + 33.8 v 12.2 + 22.0) and eat more per dragon-turn in r0-60 (8.7-9.7 v 7.0-8.3 per 100). We circle twice as much early (13-17% of moves v 7-10%).
  - [img] Stripes 1195116 r4-36: their dragons enter the 2×2 pearl boxes through the portals; ours walk the lanes past them; our first box meal ~r24; r32: 2 dragons/total 6 v 5/10.
  - Games: Devil 1212800 r18-54, Trophy 1212799 r0-40, Stripes 1195116 (all [img]).
- **T2. Late queen race and tiebreak (B + C1 + C2, 52).**
  - [img] Schooltime 1205304 r420-490: we own two thirds of the board with ~50 dragons (total 248 → 346); they keep 7-14 dragons and resign stubs next to one champion in a walled bed room (31 → 66); our longest 30, queens tied at 3; lost on longest.
  - [img] Slithery Fight 1212977 r402-498: they already have twice our total at r400 (294 v 159); our r400 resign wave dies in the contested middle and they eat it; queen race lost 33-48.
  - Late resigns cost us 61 a game (152 length); 77% die more than 3 tiles from our queen (median 20), so she can't eat them. In losses the opponent eats more of our r300+ remains than we do (150 v 117).

## 5. Devil and Trophy
- **Devil (4-14; 12 of 14 losses early collapses).** Before r60 we eat 33 of 34 pearls at home, 0.9 in the middle; they eat 24 home, 42 middle, **16 in our zone**. Our first head in the middle: never (r100+); theirs r16; their heads in the middle at r20/40/60: 2/6/5, ours 0. Dragons 9.5 v 20.5 at r60, 8.5 v 28 at r100. They split twice as often before r60 (34 v 16); we lose 11.5 stubs a game to resigns before r100. [img] 1212800 r18-54: their dragons cluster in the central column by r18-24; ours stand single-file in the vertical lanes of our third and don't eat; by r54 their mass spills into our half. Our wins look the same (no middle) and came only against weaker teams.
- **Trophy (10-16; 15 of 16 losses early collapses).** In losses they have 10-12 heads in the cup at r40-60, we have 1-2; pearls by r60 in the middle 46 v 6; dragons 6 v 14 at r40, 9 v 22 at r60. In our 10 wins we eat 21 in the middle by r60 and they have 3 heads there at r40. [img] 1212799 r0-40: our two starters leave their alcoves the other way, over the board edge onto the outer ring; one enemy starter takes the bottom portal into the cup ~r10, the other enters from below; by r20 they have 7 in the cup; r40 17 v 5, total 47 v 13; our queen (2) circles alone top left. The cup is reached through its portal and a bottom mouth; we don't take the portal.

## 6. The free sprints on the server
- Use: 32.9% of non-queen moves by 5+-long dragons (rc1 2.5%, almost all after r430). By phase: 1% before r60, 16% r60-100, 25-30% r100-400, 43% after r430. Strong opponents (2100+) sprint on 47%, and 22-26% already in r0-60 (they have 5+-long dragons early). The v2 gate: 0.8% of our sprints with an enemy head in view.
- Deaths: during the sprint 270 of 215,934 (0.13%). Death to an enemy within 5 rounds: 1.7% after a sprint v 3.5% after a single step (selection: the gate keeps sprints away from enemies).
- Net: our 5+-long non-queen dragons killed by enemies **11.0 a game v rc1's 8.8 (+25%)**; longest at r400/r490 unchanged (median 9/15); longest-decided games 38-36 v rc1's 12-16. The sprints trade a few more long dragons for faster walking; net about zero.

## 7. What strong opponents do that we don't (2100+, 109 games)
- Opening: portal steps 138 a game v our 67; more splits r30-100; dragons 14/23 at r60/r100 v 11/17; eat 8.4 v 7.4 per 100 dragon-turns in r0-30; grow 5+-long dragons early and sprint with them.
- Their queens move: 34% of her moves circling v 80% for ours.
- Late: fewer stub resigns (29 a game v 52), nearer the champion (median 16 tiles); they feed one champion inside a walled room ([img] Schooltime).
- They accept sloppy deaths (walls, self-hits, failed splits: ~110 a game) for faster expansion.

## 8. Movement inefficiencies (all games; opponent after the slash)
- Circling (head revisits one of its last 8 tiles without eating): r0-30 13.3/6.8%, r30-60 16.7/10.1%, r60-100 15.6/10.8%, and after r300; the queen 80.9/30.3%. Worst runs: 2-long stubs waiting at beds (Prisoners Dilemma 1203633 id 37 r61-155, 94 turns; Around UNSW 1178680 id 9 r6-85, 79 turns).
- Feeders that never arrive: the r360+ resign wave, 61 a game and 152 length; 77% die more than 3 tiles from our queen, 94% more than 3 from our longest ally ([img] Slithery Fight 1212977 r402, weakhold 1204754 r472).
- Dead ends: 55 entries a game v 43, only 20 deaths after one in 370 games. Not a cause.
- No pearl in view: 42.7% v 41.1% of dragon-turns. Not a cause.
- Target switching: not measurable (server replays carry no logs); circling is the proxy.

## Images
14 filmstrips in img/, all viewed except 1207482_open. From images: Trophy starters avoid the cup and portal; Devil lane-parked dragons v the opponent's central column; Stripes portal-gated boxes; the queen circling beside visible hunters (Australia ×2, QoS, weakhold); split-to-death in the weakhold corridor (×2) and the Islands self-box; Schooltime champion-in-a-room; Slithery Fight resign wave in contested ground. All counts from decoded replays (tools/feat.py, tools/early.py) and arena metrics.

## Files
/home/user/syeyoung-tokens.league/replays/server-r19/ (raw replays deleted): data/ (games.json, met/, feat_*.jsonl, b/); analysis/ (rows.json, perf.txt, phase_*.txt, agg_*.txt, early.jsonl; perf.py, phase.py, agg.py, sprcmp.py, blunder.py, early_agg.py); tools/ (feat.py, early.py, fl.py, dl.sh); img/.
