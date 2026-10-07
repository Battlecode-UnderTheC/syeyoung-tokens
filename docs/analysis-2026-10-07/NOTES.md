# Deep analysis: working notes (all claims from replays and metrics, not code)

Data: all 676 real server games of ring-sprint-1 (server v17933; 388-288, rated 2138). Arena DB source=competition.
Files: real_ring-sprint-1.json, met/, rep/ (gz), feat_s1.jsonl (tools/feat.py), metrows.json (tools/metsum.py),
qd_s1.jsonl (tools/qd2.py), beds.json, dt/ (dev test 1, 138 games vs top teams, same extractor).

## 1. Games are decided early, or by the queen
- **The early turn:** in losses, mean P(win) is .41 at r30, .33 at r60, .29 at r100. In 143 of 287 losses the eval stays below .5 from r≤50.
- **Wins** are flat at r30 (.50), then pull away.
- **How games end:**
  - Losses: elimination 126, longer queen 86, longest 73.
  - Wins: longer queen 192, elimination 122, longest 72.

## 2. Who loses their queen decides the game
| Queen outcome | W-L |
|---|---|
| Only theirs died | 252-0 |
| Only ours died | 0-104 |
| Theirs died first, then ours | 44-63 |
| Ours died first, then theirs | 43-39 |
| Neither died | 49-80 |

- **Their queen dies in 82% of games (364/446); ours in 29%.**
- **Queens on both sides are 2-3 long.** Theirs are mostly killed by our 3-long dragons lunging from 3 tiles.
- **Our queen:** killed by a 2-3-long enemy head-on or lunging from 1 tile; 0 allies within 3 in 54 of 129 deaths. Examples: 198108 (r61, wandered 21 from spawn), 191755 (sits in open ground while stubs circle 2-4 away).

## 3. The early economy: we are the most home-bound bot
- **Losses, r30** (us/them): dragons 9.3/10.4, eaten 16.8/22.0, middle eats 2.8/5.1, spread 8.2/10.6, farthest head 15.1/18.2, portal steps 1.6/3.6.
- **Losses, r60:** middle eats 7.8/17.1, far eats 2.6/7.4 (they eat on our side), dragons 13.7/18.5.
- **Even in wins** our spread is lower (7.9 v 9.2 at r30) and portal use half.
- **Spread at r30 across bots:** dev test 1 (#1) 11.2, its opponents 9.5, our opponents 9.8, us 8.0.
- **Films:**
  - Devil 202566 (L v bread first search): all six enemy starters drive to the central bed column by r10-20; ours stay in the left lanes; 15 v 7 dragons at r30, 33 v 5 at r80.
  - Devil 203530 (W): our starters go east by r10-20, and the centre is split.
  - Trophy 189460 (L): our second starter leaves the cup via the torus wrap; the enemy fills the cup by r30 (9 v 5).
  - Trophy 190569 (W): our QUEEN walked into the cup at r20 and split there, 36 v 5 dragons at r80.
  - QoS 196748 (L): the enemy takes the top bed room at r10-20 and later the bottom one; two of ours die at the bottom room entrance at r40.

## 4. The late game: we never concentrate length
- **Our queen is length 2 from r0 to about r450 in EVERY game** (Trauma table), then sprints to 15-30.
- **Queen-decided losses:** their queen grows from r200: 4.1, 6.6 (r300), 12.9 (r400), 18.7 (r430), 33 at the end; ours is 2 until r430, then 13.6.
- **Longest-decided losses:** we end with MORE total length (185 v 151) and dragons (24 v 11.5), but their longest is 48.6 v our 24. Their longest grows from r300.
- **Teams that beat us** have more dragons at r100 and a longer longest dragon from r200 (bread first search 8.6/14.7/20.8 at r200/300/400 v our 5.5/6/8.4; Vibing++ 10.5/16/21.8). Our longest stays at 5-6 until r300 against everyone.
- **Films:**
  - Schooltime 191752 (L v bread first search, longest 113 v 24): the enemy champion sits coiled in the walled bottom bed room, 76 → 126, fed by enemy self-culls inside the room. Our r380 death wave is wasted far away.
  - Slithery 190639: the enemy queen is 31 by r438 by a bed cluster at the edge, ending at 57; ours is 4-8 until r470.
  - Portals 196688: after the r384 cull the enemy converts EVERYTHING into its queen (n1, queen 41 = total). Ours is 3 at r454, 31 at the end, while we still hold 79 total.
  - Islands 190620 (L v Vibing++, 68 v 20): their longest is 8 at r33, 16 at r100, 27 at r200, 35 at r399, 47 at r432 → growing all game. Ours is 4 until r300. Our total collapses from 136 at r333 to 75 at r399.

## 5. Death causes: churn
- **We die almost only by "resign":** in r150-300 of wins, 92 per game (214 length), with ZERO free neighbour tiles, i.e. boxed in (in lanes or by our own bodies; also newborns spawned into blocked lanes).
- **Opponents die to walls, self, own bodies and failed splits** in similar total numbers. Dev test 1: 115 failed splits per game. Everyone churns: about 300 splits and 300 deaths per game.

## 6. Opponents
- **bread first search 13-39:**
  - QoS 0-8 (early eliminations);
  - Slithery 1-6 and Portals 1-5 (late queen races: their 40-64 v our 27-35, often P .55-.65 at r400 then collapse);
  - Schooltime 1-6 (longest 92-113 v 17-24).
- **Teams we farm** (WeLikCoding, tozoman, doofenshmirtz) have few dragons and short ones.

## 7. More findings
- **The arena confirms the queen-outcome structure** (1,832 games v non-ring bots, 1318-514): only theirs died 970-0, only ours 0-188, theirs first 103-197, ours first 91-42, neither 154-85.
- **Server, theirs died first and we still lost (63):** our queen died a median 122 rounds later (median r273). 34 were lost by elimination: the queenless enemy out-grew and eliminated us. An economy failure, not a queen-safety failure.
- **Neither queen died, losses (80):** their queen grows 3, 4 (r200), 6 (r300), 13 (r400), 22 (r450), 28 at the end, sitting ~4-5 from spawn with about 1 escort. Our heads got within 1 tile of her in 50/80 games and still didn't kill her. Our queen is 2.5 at d≈2 until r450.
- **Thrown games:** 71 losses had P>0.6 after r200 (41 lost on longest, 19 on queen).
  - **Conversion failure:** 66 tiebreak losses (39 longest, 27 queen) where we had MORE total length (37 with ≥1.5x).
  - **Schooltime:** both queens are sealed in 2-tile starting cells all game at length 3, so it's always 3-3 → longest dragon. We hold 2-5x the material but never build a champion. The walled bottom bed room stays full and uneaten (202411: ~25 pearls untouched r420-490; our longest 20-22 v their 30; we had 46-50 dragons v 7-21).
- **Our strong maps are the queen war:**
  - Maze 34-2 (our queen died in 2/36; theirs in 32/34 wins, median r354);
  - PD 39-8 (their queen dies ~r189, then elimination);
  - Around UNSW 33-10.
- **Stripes 190622 (L):** our two starters eat NOTHING in r0-40. All food is in portal-gated 2x2 boxes; we never enter. The enemy enters by r8.
- **Dev test 1 v bread first search, Devil (1151248):** the same rush-the-centre opening. All 6 dev test starters are in the central lanes by r10; 22 v 7 dragons at r80.
- **Spread at r30:** dev test 11.2 / its opponents 9.5 / our opponents 9.8 / us 8.0. Portal steps at r30: 5.5 / 4.3 / 3.6 / 1.6.

## 8. Queen-kill opportunities (tools/qopp.py, 637 games)
"Opportunity" = at turn start a non-queen dragon can reach the enemy queen's head through free tiles within its move reach (ceil(L/4) + paid steps).
- **Ours:** 1,265 opportunity dragon-turns, 251 converted (20%). Only 27 games had chances without a kill (14 of them losses, queen ~25 long, ~11 rounds of chances).
- **Theirs v our queen:** 3,059 opportunity dragon-turns, only 7% converted.
- **In 384 of 675 games our dragons NEVER had their queen in reach.** Those games: win 44%, our heads in their zone at r100 2.4. Games with a chance: win 75%, heads in their zone 4.8.
- **The bottleneck is reaching her,** not converting. Their queen sits 2-5 tiles from her spawn.

## 9. Growing a queen is a gamble
- **Enemy queen ≥10 at r300:** 54 games; 34 died (we killed them); their side won 24.
- **Enemy queen ≥10 at r400:** 72 games; 22 died; their side won 47 (65%).
- **Our queen** never reached ≥10 before r400 (once in 676 games).

## 10. ring-sprint-21 (current live, 113 server games, 50-63): same profile
| | sprint-21 | opponents |
|---|---|---|
| Spread at r30 | 7.9 | 9.7 |
| Portal steps at r30 | 1.4 | 3.8 |
| Queen at r450 | 3.5 | 10.1 |
| Longest at r400 | 9.7 | 15.6 |

Our deaths are again mostly resigns (203/game).

## 11. Where starters go (r20 BFS distance from own spawn, us v them)
Devil 7.1 v 10.7, Trophy 7.1 v 10.7, weakhold 7.0 v 10.1, Default 8.7 v 10.7. Opponents also end closer to OUR spawn: they advance toward the enemy.

## 12. Rules facts used (docs/game.md)
- Head-on kills BOTH regardless of length.
- Ids are visible in the 7x7 window (queens = ids 0/1).
- Free steps ceil(L/4): a 43-long queen gets 11 free steps.
- A pearl drops on every 2nd segment of a dead dragon.
- A bed and its mirror share one countdown.
- A pearl doesn't spawn under a living body.
- Every turn must MOVE or SPLIT.

## Caveats
- My multi-step counter reads ~0 for everyone (likely a path-reconstruction issue), so no claims about sprint usage.
- My 'hot region' arrival metric didn't discriminate (it includes home beds); dropped.

## 13. Second pass (deeper)
- **Opponent fingerprints** (tools/fprint.py) [portal-step figures here include sprints; see §29]:
  - bread first search (25% for us): n 12/30/40 at r30/100/300, eat@60 88, longest 21 at r400.
  - Vibing++/Cultery (20%): spread 12.1/16.9, portal steps 39 by r100, longest 10.6/15.9/22.5.
  - We are lowest of all on spread (8.0 at r30), portal use (11 by r100) and middle eats (9.5 by r60).
- **Opening films v top opponents:**
  - Autarky v bread first search 208778: they cross into our half's bed column by r20-30; 15 v 11 at r30, 19 v 9 at r50.
  - **Tower Defense v bread first search 206942:** we sit in our spiral tower with total length 4-5 for 60 rounds; they leave theirs at ~r55 into the big middle field. Exploration/exit failure.
  - Devil v Vibing 191242: their queen and all starters rush the centre (same as bread first search and dev test).
  - Slithery v Preyas 200526: we lead r10 (24 v 16 dragons), then stall r15-30, eating 14-17 per 5 rounds v 20-25, with 28 boxed resigns in r0-30. 29 v 46 dragons at r30.
  - **Trauma v bread first search 208818:** all food is inside the central maze; they enter by r20-30; we stay in our open base until ~r50-60 (total 13-16 for r30-80).
- **Corridor use (tools/corr.py, r0-100):** our head share on corridor tiles and our corridor eating are lower on maze maps:
  | Map | Head share (us/them) | Corridor eats (us/them) |
  |---|---|---|
  | Trauma | .21/.30 | 14/35 |
  | Islands | .08/.12 | 42/78 |
  | Portals | .16/.21 | 85/117 |
  | Devil | — | 58/92 |
  | Weakhold | .22/.28 | — |
- **Early self-inflicted deaths are not the differentiator:** opponents lose as much to walls, self and failed splits. Our resigns are equivalent churn.
- **Early timing:**
  - First pearl eaten by the team: us r12.7 (W) / r14.9 (L) v them r11.8 / r11.6.
  - First split after r0: us r30.7 / r37.1 v them r26.5 / r32.0.
- **Logistic model of the result on r30/60/100 diffs** (acc .64/.70/.74):
  - eat_own +.45/.47/.34 (home eating speed matters MOST early);
  - dlen (length lost to deaths) −.32/−.42/−.32;
  - eat_mid +.22/+.32/+.21; spread +.17/+.30/+.33;
  - n +.07/+.36/+.65.
  - **Correction:** "skip home food" is wrong. Eat home fast AND push.
- **Map loss types** (per-map table):
  | Type | Maps | Losses by | Signature |
  |---|---|---|---|
  | ELIMINATION | Trophy, Devil, QoS, Stripes | elimination | eat@60 Trophy 30 v 60, Devil 38 v 81; queen dies 67-87% |
  | LONGEST | Schooltime, Islands, Australia, Around UNSW | longest | their lon@400 19-27 v our 12-15 |
  | QUEEN | Trauma, Slithery, Portals, Weakhold | queen | their q@450 14-18 v our 2-4 |
- **Longest-decided losses (73, tools/feedprox.py):**
  - their champion at r380 is 22.7 v ours 10.9;
  - 24% of their r380+ dead length lands within 3 of their champion v our 13.7%;
  - they finish at 48.6 v 24.
- **Queen-death taxonomy (294 of our queen deaths):** far-from-home (≥8 from spawn) 97, lunged near home 90, stepped-into near home 77, boxed 30.
  - Far-from-home deaths have no ally within 3 in 64/97.
  - **QoS: 24 of 34 queen deaths far from home.** She walks to the torus edge next to the enemy spawn (190705, 185498 v dev test 1).
  - Queen death rate by map: Trophy 87%, Islands 79%, QoS 77%, Default 74%, Devil 69%, Stripes 67% v Maze 6%, Trauma 10%, Slithery 13%, Portals 14%.
- **Trades:** in r150-300 of losses, the opponent initiates 16.9 trades/game v our 11.8; we're hit 15.5 v 10.4.
- **Sonar:** nearly every team pings 2-4 times per dragon-turn (us 3.9). Not a differentiator.
- **Head-to-head v dev test 1** (2-3; W Maze, PD; L Stripes, Autarky, QoS):
  - Stripes 185532: we enter a box by r13 but lose a trade and drop to 1 dragon by r22.
  - QoS 185498: our queen crosses to the enemy spawn side by r25.
  - Autarky 185527: dev test holds the central band from r30 and expands to 24 dragons by r100 v our 10.
- **Top-v-top sample** (Cultery, aAah, forgot to mention; 68 games):
  - 60% end by elimination;
  - the winner often holds 50-64 dragons (the unit cap) v the loser's 0-2;
  - on queen maps the winner sometimes ends with ONLY its queen (Portals 1301698: n1 v 34).
  - Top teams v us: spread@30 10.0 v 8.0; portal steps@60 17.4 v 4.3; eat@60 66.5 v 53.7.
  - Their queen and longest are NOT bigger than ours mid-game (q@300 3.5, lon@400 13.1).
  - Their queens roam WITH the swarm, 9-11 tiles from spawn (r30-200), while ours stay 1-2. Escort counts are similar (~1-1.6).
  - Films: top_trophy_1301706 (the queen inside the cup amid 9 own dragons by r26); top_devil_1301695 (the queen in the central lanes by r14; a 16-long champion by r38).
- **Current ladder (2026-10-07):** Cultery (team 306, ex-Vibing++) 2528, aAah 2523, larp stock 2450, forgot to mention 2443, dev test 2436, 𓎼… 2424, chad gdp 2413, Sponge 2370, horse 2307, WeHaveQuizzes 2267 (#10). **Us #23, 2147.**

## 14. Territory confirmed at the top; the aAah blueprint
- **My replay board-control computation (tools/bc.py)** matches the arena metric within 0.03.
- **Top-v-top (102 games):** the board-control leader wins 64% (r30), 72% (r60), 78% (r100). A lead ≥0.2 at r60 → 30-4.
- **Per-team, top sample:**
  | Team | W% | bc30 / bc60 | eat@60 | portal@60 | queen died | queen / longest |
  |---|---|---|---|---|---|---|
  | aAah (#2) | 62% | 0.54 / 0.55 | 75 | 26.5 | 51% | 1.8 at r300-450 |
  | Cultery (#1) | 49% | 0.45 | 62 | 13 | 74% | |
  | forgot to mention | 39% | | | | 80% | grows its queen (6.5 at r300) |
  | larp stock | 41% | | | | 94% | lon@400 20 |
- **Spacing at r30** (nearest ally head / share of heads with an ally within 2):
  | Bot | Nearest ally | Within 2 |
  |---|---|---|
  | US | 3.76 | **0.43 (most crowded)** |
  | our opponents | 4.27 | 0.35 |
  | aAah | 4.27 | 0.29 |
  | Cultery | 3.88 | 0.41 |
  | forgot to mention | 4.13 | 0.38 |
  | larp stock | 4.47 | 0.32 |
- **aAah's queen:** dist from spawn 11.4 (r30), 13.7 (r100), 16.8 (r200); escort own3 0.8 → 1.9 → 2.0 → 2.4; length 2.4-4. It roams inside its own carpet.
- **Films:**
  - aaah_islands_1301696: an even carpet, n 5/8/15/24/33/45/58 by r86, single dragons evenly spaced over the whole map.
  - aaah_qos_1263292: claims the top bed room by r14; its queen comes near the room by r26.
- **Takeaway:** aAah is our identity (tiny safe queen, no growth) done with a territorial, evenly spaced, portal-fluent economy. It is the blueprint for "Swarm Territory".

## 15. Top teams on queen-race maps (Maze, Portals, Slithery, Trauma, weakhold, Schooltime)
- **Both queens often die by r200-300**, and then the longest dragon decides.
- **Cultery and forgot to mention grow their queen from ~r150-300** and win the queen race when she survives:
  - Cultery Portals 1299651: 41 at r300 → 68;
  - Cultery Maze 1299650: 35 → 64;
  - forgot Slithery 1308232: 37 (r300) → 95;
  - forgot Trauma 1308235: 23 (r300) → 57;
  - forgot Portals 1263290: 15 → 41.
- **aAah keeps its queen at 2-3** and loses Portals 1-3 and Trauma 2-2 to those queens. It wins Schooltime and Slithery via longest when queens die or are sealed.
- **Film `cultery_portals_qgrow`** (r180-300): Cultery holds the right third of Portals as a solid mass of its own bodies. Its queen grows 10 → 15 → 21 → 30 → 37 → 41 inside that fortress, while its dragon count drops 24 → 9 (feeding). There's a stable front in the middle.
- **So there are two top styles:**
  1. **carpet** (aAah): territory, tiny queen;
  2. **fortress-queen** (Cultery and forgot, on queen maps).
  - The best answer is map-adaptive WITHOUT map keys: grow the queen when she sits inside a sealed own-body / own-territory region; otherwise keep her tiny and win on territory.

## 16. Head-on trades (not a lever)
Per side per game, (initiated, hit) head-on collisions by phase:
| phase | top teams | us | our opponents |
|---|---|---|---|
| r0-60 | 2.3/2.2 | 1.4/1.4 | 1.5/1.2 |
| r60-150 | 8.2/8.0 | 6.6/6.7 | 7.5/5.7 |
| r150-300 | 14.4/13.6 | 12.6/13.7 | 15.1/10.9 |
| r300-500 | 9.5/9.1 | 8.6/10.1 | 10.9/7.7 |
Length lost head-on, ours/theirs: wins 1.01/1.05/1.12/1.08, losses 0.99/0.96/1.03/1.09 (same phases).
Exchange is ~1:1 in both wins and losses, so trade efficiency does not separate them. Top teams and our opponents
*initiate* more (they move toward contact; we get hit more than we hit after r150), but the bill is even.

## 17. Bed proximity in the opening (fv_ids: 59 W, 60 L that reach r40)
Mean number of beds within Chebyshev 3 of a head (us/them):
| | r10 | r20 | r30 | r40 |
|---|---|---|---|---|
| W | 3.82/3.37 | 4.12/3.57 | 4.30/3.90 | 4.80/4.01 |
| L | 3.61/4.13 | 4.05/5.20 | 4.35/5.31 | 4.68/5.53 |
Our own number is the same in W and L (~4.0-4.8). What changes is the opponent: in losses their heads sit among
~30% more beds from r20. We lose openings to opponents who go to the bed *clusters* (dense regions), not to the
nearest bed. This is the macro-positioning gap from section 13 in another form: head placement by bed density.
Top-v-top (101 decided games, winner/loser): r10 4.42/4.05, r20 5.19/4.69, r30 5.39/4.85, r40 5.57/4.79; the winner is
higher in 53/59/60/70 of 101. Same signal at the top: by r40, sitting among more beds goes with winning in 70% of games.
**Per map, elimination maps (all our games reaching r40), beds within 3 of a head, us/them:**
| map | W | L |
|---|---|---|
| Devil | r10 2.0/5.3, r40 5.7/8.6 (n9) | r10 2.1/4.5, r40 5.4/8.5 (n27) |
| Trophy | r10 5.7/7.6, r40 8.4/9.3 (n13) | r10 6.5/8.8, r40 8.2/11.2 (n25) |
| QoS | r10 4.6/2.6, r40 6.2/3.2 (n18) | r10 3.1/4.2, r40 5.7/6.4 (n26) |
| Stripes | r40 7.8/6.7 (n21) | r40 6.6/7.3 (n18) |
- On Devil and Trophy we are behind on bed access in WINS too: we never reach the clusters there; those wins come from elsewhere (queen kills).
- QoS is decided by it: ahead in W, behind in L.
- Top teams at r40: Devil 2.0-11.1 (median ~7), Trophy 6.1-13.6 (median ~10). Our opponents on Devil/Trophy sit at the top-team level; we sit below it.
- Server per-map ratings (ring-sprint-1): Trophy 1814, Devil 1857 are ~200 below every other map. These are exactly the two maps where our heads never reach the bed clusters.

## 18. Opening movement (tools/startdir.py, tools/straight.py)
- Mean starter distance from own spawn at r20, us/them: Devil 7.1/10.7, Trophy 7.1/10.7, weakhold 7.0/10.1,
  Default 8.7/10.7, Australia 5.7/7.3, Around UNSW 7.7/9.0 (QoS, Stripes, Islands about equal).
- Net head displacement per 10 moves, r0-60 (248 games): us 5.42 v them 6.09; distinct tiles per 10 moves 9.39 v 9.72.
  Lower on 16 of 17 maps (Devil equal 5.43/5.30: there we move as much but along our side lanes, not into the field).
- Film devil_202566_L: at r10 their 6 starters are all at the central 2x2 bed clusters; ours are still within ~5 of
  spawn, then work the vertical lanes on our side (r20-60) and never enter the field.
- Bots see only the 7x7 window (+ map size, own spawn), so their rush is a heading choice (leave spawn, cross toward
  the board interior) not map knowledge.

## 19. Big queens: enclosure decides survival (tools/fortress.py, seal2.py, qquiet.py, qlife.py, qfeed.py)
"Enclosed" = no enemy head can reach the queen's head through free tiles (bodies block).
- **Big enemy queens (≥10) in our 676 games:**
  | at | enclosed: died / their side won | reachable: died / won |
  |---|---|---|
  | r250 | 33% / 67% (n21) | 95% / 24% (n21) |
  | r300 | 35% / 65% (n26) | 93% / 22% (n27) |
  | r350 | 30% / 67% (n43) | 81% / 24% (n21) |
  Top-v-top: big queens are mostly enclosed (r250 14 v 6), and enclosed ones win 12/14.
- Death hazard per 30 rounds for queens ≥8 (all datasets): enclosed 2-4%; enemy head ≤8 free steps away 26-50%.
- **Our queen is enclosed most of the game** (160 random games): 36% of samples at r0-60, 43% r60-150, 52% r150-250,
  59% r250-350, 69% r350-450. Hazard while enclosed: 0.0-1.5% per 30 rounds. **She stays 2.5-2.8 long the whole time.**
  Enemy queens at the same enclosure rates are 3.9 → 5.7 → 10.7 → 19.1.
- Our enclosures: median 364 free tiles (quartiles 128-748), 23 beds inside, 7-12 uncollected pearls inside
  (nearest ~14 steps away). The border is mostly our own bodies (58 own v 22 enemy tiles; 46% have NO enemy border).
- Fortress films (fort_trauma_1308235, fort_slith_1308232): forgot's queen grows 14 → 49 in its own open base on
  Trauma while its swarm holds the maze between; on Slithery each side carpets its half at the cap and forgot's queen
  grows 17 → 90 behind its own front. The pen is not special geometry; it is the back of a held territory.
- **Queen economy per 100 queen-turns** [undercounted by the path bug; corrected table in §29, same pattern]:
  | phase | us: bed / drop eats, length given away by splitting | them |
  |---|---|---|
  | r150-250 | 0.6 / 0.5, gave 1.0 | 0.9 / 1.0, gave 0.4 |
  | r250-350 | 0.6 / 0.6, gave 1.2 | 0.7 / 2.0, gave 1.5 |
  | r350-450 | 0.6 / 0.9, gave 0.8 | 1.0 / 6.2, gave 3.1 |
  | r450-500 | 2.9 / 8.7 | 1.7 / 7.4 |
  Our queen splits away what she eats; they start feeding (drop eats) at r250 and hard at r350; we start at r450.
- **Drops eaten by queens after r200 are own sacrifices on both sides:** us 95% own resigns; them 41% own resigns,
  38% own self-collisions, 15% own failed splits. Feeding is deliberate everywhere; the difference is timing.
- **A window-visible safety signal (qquiet.py, 3454 samples, both queens, r60+):** rounds since any enemy segment was
  inside the queen's own 7x7 window.
  | | enemy seen <30 rounds ago | quiet ≥30 rounds |
  |---|---|---|
  | enclosed | 2.6% die within 30 | 1.8% |
  | reachable | **14.0%** | 1.9% |
  Being quiet for 30 rounds is as safe as being enclosed, and the queen can see it herself. Our queen is quiet in 45%
  of samples (enemy queens 37%). Caveat: these hazards are measured mostly on small queens; a head-on kills both
  whatever the length, and body collisions kill the mover, so length itself adds no exposure except being boxed.

## 20. Round-500 results (reason strings of 676 games)
- Decided by queen length: 192 W / 86 L. With BOTH queens alive at r500: **28 W / 56 L**; with their queen dead: 164 W;
  ours dead: 30 L.
- In the 56 both-alive losses their queen beat ours 38.5 v 20.5 (median); margins ≤10 in 25 of 56.
- In those 56 losses (qquiet, qlife): our queen was enclosed 69% and quiet ≥30 rounds 45% of r250-450, yet was 2.4
  long at r250, r300, r350 and r400, then 3.4 at r450 and 17.7 at the end. Theirs went 5.6 → 7.5 → 12.0 → 16.7 → 29.5 → 39.1.
  Drop eats per phase (whole set): ours 0/0/3/1/41/380, theirs 26/34/68/223/532/269. In the 28 wins theirs was
  slower (4.0 → 6.5 → 15.1).
- 21 of the 56 had our queen enclosed >90% of r250-450 the whole time; 11 of those 21 are Portals.
  Film `qrace_portals_210934`: both queens are 3 until ~r400; theirs starts feeding first and finishes 38 v 30.

## 21. Portals rooms
- 96 of 104 Portals beds are in 24 2x2 rooms that can only be entered by portal. 71-84 pearls sit in them at any time
  (us and top-v-top alike). That is a fast-respawning steady state, not neglect: on Portals we eat 135-210 room pearls
  per phase (r0-150, r150-350) and 30-100 dragons per phase die inside rooms (trapped), on both sides.
- In our Portals losses the opponent out-harvests the rooms: early 179 v 143, mid 350 v 211 (wins: 125 v 135, 294 v 205).

## 22. Elimination losses (125 of our losses)
- They end at median r243 (quartiles 160-370). Our queen died in all 125, a median of 31 rounds before the wipe.
  At her death we had a median of 3 dragons v their 27.
- By map: Trophy 25, Devil 25, QoS 23, Stripes 18, Default 9, Autarky 8, Tower Defense 6, weakhold 6, PD 4.
- **Dragon-count trajectory (median):** r10 3 v 3, r20 4 v 4, r30 5 v 7, r40 6 v 9, r60 7 v 13, r100 6 v 22, r160 6 v 29,
  r250 6 v 32. **We stop growing at r40-60 and plateau at 6-7 dragons for 100-200 rounds** while they triple.
  The plateau is a starved periphery: in `trophy_189460_L` our dragons hold the map corners (single beds) while theirs
  fill the cup (dense beds, portal entry) from r20-30. It is the opening failure (§17-18) carried to its end.

## 23. Queen outcome table and "thrown" wins (qd_s1, 675 games)
| our queen / their queen at the end | W | L |
|---|---|---|
| both alive | 49 | 80 |
| ours alive, theirs dead | 252 | 0 |
| ours dead, theirs alive | 0 | 104 |
| both dead, ours died first | 43 | 39 |
| both dead, **theirs died first** | 44 | **64** |
- **107 games reached the won state** (their queen dead, ours alive; 252-0 when it holds) **and then lost our queen:
  44-63.** That's 22% of all our losses thrown from a won position.
- In the 63 losses: their queen died at median r100, ours at r273 (gap median 122, quartiles 64-217). She died head-on
  to an enemy in 57/63 (40 lunges), 5 from home, ~4 enemy heads within 5, <1 ally within 3, length 3.
  6 were our queen's own resigns (boxed).
- **Material at the moment we killed their queen:** L 12 v 23 dragons, W 24.5 v 21.5. After it, in L the enemy grows
  23 → 28 → 33 → 40 (+0/50/100/200) while we hold 12-15. In W we grow 24 → 36 → 49.
- After their queen died, our queen was enclosed 17% (L) / 15% (W) of samples (52-69% in general) and quiet 27% / 49%.
- Films `thrown_devil_202570`, `thrown_devil_206390`: their queen died r44 / r82. By r260-330 our queen is the
  last dragon (n1 v 45-62) and still survives 60-70 rounds alone at the map edge before she is caught.
- The kill itself is a head-on, which kills our hunter too, so no one of ours necessarily sees it happen. Unless a
  witness reports it, our team can't know it has the won position (the same is true for them).

## 24. Quiet-queen replication (ring-sprint-21, 113 server games, 5902 samples) and length check
| | quiet ≥30 | enemy seen <30 ago |
|---|---|---|
| enclosed | 1.1% die within 30 | 2.0% |
| reachable | 3.5% | **16.4%** |
sprint-21's queen: enclosed 51-56%, quiet 53-56% of r150-450, length 2.7-2.9. Enemy queens 4.3 → 6.6 → 15.6.
**By length** (all qquiet samples pooled, both sides; safe = enclosed or quiet ≥30):
| length | safe | exposed |
|---|---|---|
| ≤4 | 2.0% (n12202) | 15.0% (n3954) |
| 5-14 | 4.0% (n1807) | 13.4% (n700) |
| ≥15 | 2.7% (n1343) | 9.3% (n301) |
Length does not raise the hazard; the state does.

## 25. How queens die, up close (tools/qlast.py, qlast2.py; all 676 games)
- **Our kills that hold** (their queen dead, ours alive, 252): spread evenly over the game (quartiles r105/r210/r348).
  - 141 are our lunges and 38 our plain head-ons.
  - 57 (23%) are the enemy's own errors: self 20, into an ally 19, wall 18.
  - Our killer is typically 3 long, 41 rounds old, and starts 1-3 tiles from her. Their queen is 3 long, 8 from her spawn, with 1.2 allies within 3.
- **Killer start distance** (Chebyshev, at the start of the killer's turn): our queen's killers 1:134, 2:73, 3:55. Ours
  on theirs 1:95, 2:90, 3:137. Killers are almost always inside the queen's window; we strike from 3 more often.
- **The queen's last move before a head-on death:**
  | | moved closer | same distance | moved away | took 1 step |
  |---|---|---|---|---|
  | our queen (260) | 110 | 142 | 8 | 230 (§29) |
  | enemy queens (315) | 191 | 116 | 8 | 249 (§29) |
  - Most common (before → after): ours 1→1 (76), 2→1 (55), 2→2 (47), 4→3 (27). Theirs 4→3 (80), 2→2 (46), 2→1 (40).
  - Length at death: ours 2 (104), 3 (86), 4 (32); theirs 2 (111), 3 (92), ≥9 (70).
- [CORRECTED in §29: 'took 1 step' counts were wrong; at length 3-4 it holds, 0 of 118 ours.] Under the move rules a 2-long queen can't anyway (1 free step, no paid steps at
  L=2); a 3-long hunter reaches 2 tiles and a 4-long one 3. A tiny queen is slower than every hunter.
- **Was there an escape?** (tools/qescape.py) At the queen's last turn: is there a tile within her move reach, through
  free tiles, that no enemy head visible in her window could reach next turn (each by its own reach)?
  | queen length | ours: escape with sprint / with 1 step | theirs |
  |---|---|---|
  | 2 | 27% / 27% (n104) | 54% / 54% (n114) |
  | 3 | 81% / 42% (n88) | 82% / 63% (n99) |
  | 4 | 81% / 41% (n32) | 85% / 62% (n13) |
  | ≥5 | 74% (n39) | 92% (n105) |
  | all | 59% / 38% (n263) | 76% / 65% (n331) |
  - At length 3-4 our queen had a sprint escape in ~40 percentage points more cases than a 1-step escape and never
    used it: about 48 of 263 head-on deaths.
  - At length 2 she is trapped in 73% of her deaths: no move reaches safety.
  - Caveats: the danger set ignores other movers acting first, and it uses only enemies visible to her. In 11% of
    our cases no enemy was visible at her turn; the killer came from 4 out.
- **Not greed:** at her last turn a pearl was in her window in 60% of the 263 deaths. With one in view she moved
  closer to it 52% of the time (ate 8%), about chance. The fatal step is threat-blind movement, not food pursuit.
- **Film `qesc_190748` (Trophy r58-59):** our 3-long queen runs along the inside of the cup rim toward a 3-long enemy
  two tiles away, steps toward it, and both die. Her body blocks the way back; a 2-step sprint down and left was open.
- **Top teams' queens (top-v-top, head-on deaths):** forgot to mention 11 closer / 16 same / 2 away, 0 sprints; Cultery
  13/12/0, 0 sprints; aAah 10/11/0, 0 sprints (16 of 22 at length 2); larp 8/3/0. An escape tile existed in 34-82% of
  them. [CORRECTED in §29: the sprint counts here used the broken path; at length 3-4 top queens sprint 2 of 29.] Small queens in the meta don't flee with a sprint.

## 26. A better window-local queen signal (tools/qthreat.py; 70 random sprint-1 + 113 sprint-21 games, 9356 samples)
- **Problem with "quiet":** the window sees through kelp, so on walled queen maps the queen sees enemies she can't
  reach. Our queen r250-450: Portals quiet 0% but enclosed 100%; Schooltime 18% / 100%; Slithery 19% / 80%;
  weakhold 26% / 61%. The quiet signal would block growth exactly where growth wins.
- **"Threatened"** = an enemy head inside her 7x7 window has a free path to her head INSIDE the window (walls,
  portals, bodies as they are) within its move reach + 2. Rounds since last threatened = t.
  | | died within 30 |
  |---|---|
  | t ≥ 20 (n8694) | 3.6% |
  | t < 20 (n662) | 30.7% |
  | t ≥ 20 and quiet ≥ 30 | 2.3% |
  | t ≥ 20, enemy visible but no path | 5.0% |
  | t ≥ 20 and enclosed | 1.6% |
  | threatened, by length ≤4 / 5-14 / ≥15 | 29% / 35% / 42% |
- Our queen is threat-free (t ≥ 20) in 90-100% of r250-450 samples on every map except Default (75%), including
  Portals, Slithery and Schooltime (100%). Hazard while threat-free r250-450: us 2.8%, them 4.9%.
- Use: grow whenever t ≥ 20 (faster when also quiet); flee by sprint the moment she is threatened. The 30% cell is
  where she dies: 5-8% of her life.

## 27. Bread first search (#13, 2242): our worst frequent opponent, 13-39
- By map (W-L): QoS 0-8, Slithery 1-6, Schooltime 1-6, Portals 1-5, Trophy 0-3, weakhold 3-2, Maze 2-0, Around UNSW 2-0.
- Queen outcomes: ours dead / theirs alive 0-19; both alive 4-15; theirs dead / ours alive 7-0; both dead 2-4.
  **Their queen dies in only 13/51 games (25%; 82% for opponents overall).**
- We had their queen within reach in only 18% of games (45% v everyone else; conversion 0.13 v 0.20). They had ours
  in reach in 47%.
- Their queen state looks like ours (threat-free 95% v our 91%; enclosed 63% v 64%; 5.7-6.2 from spawn with 0.4-2.6
  own heads within 3). The difference is upstream: they out-number us from r60 (14 v 10; r100 24 v 16; r300 39.5 v 28.5),
  so our hunters never get through.
- Film `bfs_qos_sheet` (210940): by r21-41 BFS sends its starters into the bed room on OUR side (bottom) while ours
  take the top room on theirs. We never exceed 9 dragons; BFS reaches 20 by r145 and 31 at the end.
- **QoS eating, r100-400 (all 44 QoS games):** in losses our dragons eat 8.3 pearls per 100 dragon-turns v their 4.7,
  but we have 1,354 dragon-turns v 4,574. Pearls lying on the board (~36 per snapshot) are spread evenly over tile
  types, with no corridor bias. So numbers are the constraint, not food access or pocket avoidance (my first reading of
  the film, "starving among food", was wrong).

## 28. Death-cause mix (160 random games, per game)
| | r0-100 births / deaths | r100-250 | r250-500 |
|---|---|---|---|
| us | 53.9 / 34.4 (resign or failed split 25.3, enemy contact 8.3, ally 0.8) | 112.8 / 102.1 (resign 70.9, enemy 27.8, ally 3.4) | 145.9 / 162.7 (resign 133.0) |
| them | 60.9 / 38.5 (self/wall 19.4, resign 6.1, ally 4.7, enemy 8.3) | 114.1 / 106.4 (self/wall 46.7, ally 16.0, resign 15.8) | 124.8 / 145.1 |
Our deliberate deaths are resigns; theirs are steps into walls or their own bodies. Volumes match, so churn is not the
differentiator (as §13 found). Enemy-contact deaths are equal (8.3 / 27.8 / ~27 for both).

## 29. CORRECTION: decoder path bug (found 2026-10-07 late)
- `tools/decode.py` replaced the turn's path on every per-step `dragonUpdate`, so a multi-step move kept only its
  last tile. Fixed (the path is now accumulated). Affected:
  - **"Portal steps"** (feat.py: a jump >1 between path points) counted every sprint as a portal step. Recount
    (portsprint.py, fv_ids 60 W + 60 L; top-v-top), per game:
    | | r0-60 portal crossings | sprints (multi-step moves) | paid steps | r60-100 portal / sprints |
    |---|---|---|---|---|
    | us W / L | 2.6 / 3.0 | 0.6 / 0.6 | 0.7 / 0.5 | 3.5 / 2.5, 3.3 / 1.0 |
    | them W / L | 4.6 / 6.3 | 2.2 / 6.0 | 0.9 / 2.1 | 5.6 / 2.2, 8.3 / 9.1 |
    | forgot to mention | 9.1 | 1.4 | 1.4 | 7.3 / 2.5 |
    | aAah | 12.3 | 14.5 | 6.8 | 11.5 / **54.6** |
    | Cultery | 10.4 | 3.4 | 2.7 | 10.7 / 6.7 |
    | larp stock | 8.7 | 0.5 | 0.5 | 10.1 / 0.7 |
    The portal gap stands at about 1/2 to 1/4 (the old 4.3 v 9.8 v 17.4 mixed in sprints). The sprint gap is new:
    opponents who beat us sprint 10x more in r0-60, and aAah sprints constantly.
  - **Queen last move (§25):** "took 1 step in 254/260, never sprints" was the broken counter. With action strings:
    ours 1 step 230, 2 steps 19, 3+ 5, none 6. All 24 multi-step moves are at length ≥5; **at length 3-4, 0 of 118.**
    Enemy queens 63 of 315 multi-step, at length 3-4 6 of 105. Top teams at length 3-4: 2 of 29. Closer/same/away
    counts are head-based and unchanged.
  - **Queen economy (§19-20)**, recounted per 100 queen-turns:
    | phase | us: bed / drop eats | them: bed / drop |
    |---|---|---|
    | r150-250 | 0.7 / 0.5 | 1.2 / 1.2 |
    | r250-350 | 0.6 / 0.6 | 1.0 / 2.8 |
    | r350-450 | 0.8 / 1.0 | 2.5 / 9.9 |
    | r450-500 | 8.1 / 21.9 | 4.4 / 12.4 |
    In the 56 queen-race losses their drop eats are 77/339/895/517 (r150-250 … r450+) v ours 3/1/43/855.
    Drop sources (r200+): ours 96% own resigns; theirs 41% own resigns, 36% own self-collisions, 17% own failed splits.
  - Unaffected: everything from info['ate'] (feat eats), head positions, deaths, splits, bc, bedprox, fortress, qquiet,
    qthreat, qescape (which uses each dragon's reach, not observed steps).
  - Partly affected (both sides alike, undercounted where moves had several steps): roomeat (§21) and pearlwhere (§27).

## 30. Speed: free double steps (tools/sprintuse.py, r0-150; fixed decoder)
Move rule: the first ceil(L/4) steps are free, so a dragon of length 5-8 moves 2 tiles a turn at no cost.
Per game, r0-150, by mover length:
| side | L3-4 multi-step (paid) and what it did | L5-8 moves / multi-step share | L9+ |
|---|---|---|---|
| us | 2.8 per game; 91% are kills (lunges), 13% eat | 43 / **22%** | 4 / 18% |
| our opponents | 6.8; 54% kill, 26% eat | 57 / 32% | 10 / 45% |
| aAah | 15.3; 20% kill, 35% eat | **238 / 76%** | 58 / 76% |
| Cultery | 8.4; 60% kill | 38 / 39% | 9 / 56% |
| forgot to mention | 6.0; 50% kill | 17 / 50% | 1 |
| larp stock | 1.9 | 48 / 18% | 3 |
- At length 2 nobody sprints (it's impossible: no paid steps at L=2 and 1 free).
- In our games (fv sample): moves at L≥5 per game, us/them: wins 72/45, losses 22/89. Free double-step use:
  us 24% (W) / 14% (L), them 33-34%.
- We almost never use paid steps except to strike (91% of our L3-4 sprints kill). Even when a dragon is long enough
  for 2 free steps, we take the second free step 1 time in 5. aAah keeps several dragons at length 5-8 early and moves
  them 2 tiles a turn 3 times in 4: it covers ground at twice our speed for free.

## 31. Queen-kill chances in detail (tools/qmiss.py; 160 random games first)
"Chance" = at the start of a non-queen dragon's turn, the enemy queen's head is reachable through free tiles within
its move reach (ceil(L/4) + L-2).
- **Short hunters take their chances:** ours at L2/L3/L4 strike 100% / 89% / 92%; theirs 100% / 97% / 75%. By distance
  (us): 1 → 100%, 2 → 87%, 3 → 90%, 4 → 70%.
- **Long dragons almost never lunge:** 193 of our 232 chances belong to dragons of length ≥5 (median L13, needing a
  median 12-step lunge = ~9 paid segments). We strike 10%; opponents 1% of 758.
- 160-game sample, games with a long-dragon chance while our queen was alive: struck → 15-1; passed → 5-4.
  (Full 676-game count below.)
- **Full 676 games:** our L2-4 hunters strike 97% of 159 chances, our L≥5 dragons 9% of 1,208. Enemy: 82% of 228,
  2% of 2,919.
- Games where our first long-dragon chance came with BOTH queens alive: we struck at some point → 57-6; never struck
  → **16-12**. Missed chances: median L15, 13 steps, round 408. A late 13-step lunge costs most of a 15-long dragon,
  but a dead enemy queen with ours alive is 252-0.
- Caveat: chosen by our bot, not random; strike games may differ in other ways. The upper bound is ~12 flipped losses.
- The other side: enemy long dragons had 2,919 chances at OUR queen and took 2%. Our queen survives on their restraint.
- **CORRECTION (visibility):** 1,077 of our 1,208 long-dragon chances had the queen OUTSIDE the hunter's 7x7 window
  (Chebyshev ≥4), so the hunter couldn't know. When she was visible our long dragons struck **79%** (131 chances); not
  visible 0%. Enemy long dragons: visible 214, struck 24%; not visible 2,705, 0%.
  So the lever is information, not willingness: a sonar relay of the enemy queen's position from whoever sees her,
  so long dragons within reach (median 13 steps) can lunge. The 57-6 v 16-12 split above is mostly about whether
  someone happened to see her near a long dragon.
- **Relay feasibility (160-game sample, qmiss2.py):** at the moment of an out-of-sight long-dragon chance, another of
  our dragons had the enemy queen in its window in 9% of chances (7 of 23 games). A same-turn relay alone is modest;
  it needs remembered sightings (queens of length 2-4 move 1 tile a turn, so a few-round-old fix is still close).
  Enemy side: 5%.
- **Warning time:** of 81 queen deaths after r60 (qthreat samples, sprint-1 70 + sprint-21 113), 46 (57%) had the queen
  threatened in the 20 rounds before her last 10-round sample. The rest became dangerous within <10 rounds, which only
  the at-the-moment sprint (qescape: escape tile in 81% at length 3-4) can answer.

## 32. ring-sprint-1 v ring-sprint-21 (tools/cmpvers.py, workerdeath.py; output in cmp_out.txt)
**Sets:** sprint-1 old = 676 server games (10-05 18:15 → 10-06 13:24); sprint-21 = all 588 server games (10-06 13:29 →
10-07 04:53; 355 ranked); sprint-1 new = 149 server games after sprint-21 (10-07 05:01 → 07:21), the time-matched
control, since opponents keep updating (e.g. 𓎼𓃭𓅱𓂋𓇌 8-1 v sprint-1 old, 14-37 v sprint-21, 4-10 v sprint-1 new).
**Intended changes, from the arena notes** (sprint-1 → 2 → 6 → 16 → 21): founder dispatch for small-start openings
(s2); feeding conveyor for the queen from r430 (s6); late queen kept out of corridors/pockets (s16); lunge guard +
escape run for long workers (s21).
| | s1 old | s21 | s1 new |
|---|---|---|---|
| result | 388-288 (57%) | 309-279 (53%) | 70-79 (47%) |
| both-alive queen races | 28-56 | 18-53 | 3-17 |
| thrown (theirs died first, then ours) | 44-63 | 30-66 | 7-16 |
| our dragons r30/r60/r100 (them) | 10.2/15.9/23.6 (10.3/17.0/25.1) | 10.0/15.9/23.8 (10.3/17.3/25.8) | 10.0/15.0/21.8 (10.8/18.0/27.0) |
| spread r30 us/them | 8.0/9.8 | 7.9/10.0 | 7.9/9.7 |
| board control r20/30/60/100 | .493/.482/.477/.490 | .485/.471/.460/.463 | .472/.459/.438/.434 |
| beds within 3 of heads r20/r40 us/them | 4.53/4.96, 5.26/5.32 | 4.40/5.03, 5.13/5.37 | 4.58/4.80, 5.07/5.34 |
| portal crossings r0-60 us/them | 4.0/6.6 | 3.9/7.4 | 3.7/8.0 |
| sprints r0-60 us/them | 0.6/3.5 | 0.6/4.0 | 0.6/3.6 |
| free double-step use at L≥5 (r0-150) us/them | 0.22/0.39 | 0.23/0.44 | 0.21/0.48 |
| small-start maps (≤2 starters): our dragons r20/40/60 | 3.8/5.9/8.2 | 3.9/5.8/7.9 | 3.7/5.4/7.5 |
| long workers (L≥5) lunged to death per game | 4.38 | 4.18 | 4.18 |
| our queen deaths per game after r300 (resign share) | 0.13 (1.3/100 games) | 0.14 (1.4) | 0.15 (3.4) |
| our queen eats per 100 turns r250-350 / r350-450 / r450+ | 0.9 / 1.6 / 26.9 | 1.0 / **4.1** / 27.0 | 0.8 / 2.2 / 30.9 |
| our queen length r450-500 (mean) / at r490 | 8.8 / 9.9 | **12.2** / 10.9 | 9.8 / 10.8 |
| enemy queen length r450-500 / r490 | 18.8 / 8.2 | 26.3 / 11.5 | 29.8 / 13.3 |
| queen-race losses, median their/our queen | 38.5 / 20.5 | 47 / 25 | 37 / 19 |
| our queen head-on deaths: closer/same/away; multi-step at L3-4 | 110/142/8; 0 of 118 | 94/129/12; 0 of 97 | 35/26/1; 0 of 22 |
**Reading:**
- **In the replays sprint-21 plays like sprint-1.** Opening, territory, spread, portal use, speed, small-start
  openings, long-worker lunge deaths, late queen pocket deaths and the queen's last move are the same within noise.
  Three of the four intended changes don't show on server maps:
  - founder dispatch: its note cited Colosseum, an arena map, not a server map;
  - lunge guard: its note claimed −24% lunge deaths, but 4.38 → 4.18 matches the time-matched sprint-1's 4.18;
  - pocket fix: late queen deaths are rare either way.
- **The one visible change is the conveyor:** our queen eats 2.5x more in r350-450 (4.1 v 1.6 per 100 turns) and is
  bigger at r450+ (12.2 v 8.8). Queen races still go 18-53, because opponents in that period fed harder (enemy queen
  26-30 at r450+ v 18.8 in the earlier period; their queens eat 14.7-18.4 per 100 turns in r350-450 v our 4.1).
  - Film `s21_portals_214722`: the uncancelables have fed everything into one 71-long queen by r405 (n1, 86 by r480).
    Our conveyor starts ~r430 and takes ours from 4 to 34. We still hold 7-11 dragons against their lone queen and
    don't hunt her.
- **Results:** the drop from 57% (sprint-1 old) to 53% (sprint-21) is mostly the opponent field moving on. Against
  the time-matched control, sprint-21 is slightly ahead (53% v 47%, n=149, roughly ±8%), consistent with the conveyor.
- **The field is pulling away:** between the periods, opponents' portal crossings (6.6 → 8.0), double-step
  use (0.39 → 0.48) and late queen size (18.8 → 29.8) all rose; ours stayed flat. The board-control gap widened
  (.477 → .438 at r60 in the control set).

## 33. Elo, not W-L: sprint-1 gained, sprint-21 lost (battles_all.json from the server's battle list)
Server battle records carry eloChange and the opponent's rating at the time. Our rating, reconstructed backward
from today's 2131: 2088 at sprint-1's start → 2197 when sprint-21 went live → 2146 (10-06 20:00) → 2160 when
sprint-1 returned → 2131 now.
| period | ranked battles | Elo |
|---|---|---|
| sprint-1 old (10-05 18:08 → 10-06 13:19) | 68 | **+109** |
| sprint-21 (→ 10-07 04:58) | 72 | **−37** (auto +16, ranked scrims −53) |
| sprint-1 new (→ now) | 21 | **−29** |
**Why the totals mislead:**
- **The step size shrinks with a submission's battles.** Mean |Δ| per battle: first 10 battles 20.2 (s1) / 13.8 (s21);
  battles 10-20 9.5 / 13.0; 20-40 6.2 / 5.2; 40-80 4.3 / 4.9; sprint-1 relaunched 3.0-3.5.
  - Sprint-1: battles 0-10 +72, 10-20 −55 (incl. −65 to ddabap), 20-40 +80, 40-80 +12.
  - Sprint-21: 0-10 +14, 10-20 −32 (horse −31, WeHaveQuizzes −29, horse −24 in its first hour, all at 2206-2208),
    20-40 −24, 40-80 +5.
- **Rating level:** sprint-1 started at 2088, below its level, and climbed; sprint-21 started at sprint-1's peak 2197.
  Same rating band, Elo per ranked battle:
  | our rating | s1 old | s21 | s1 new |
  |---|---|---|---|
  | 2080-2140 | +5.9 (15) | — | −2.0 (2) |
  | 2140-2175 | −1.5 (15) | +1.4 (46) | −1.3 (19) |
  | 2175-2210 | +0.5 (37) | −1.0 (20) | — |
- **The field moved:** the same sprint-1 code that made ~+1.9 per battle after warm-up on 10-06 morning makes −1.4 per
  battle on 10-07 morning, at the same rating where sprint-21 made about −0.4. Replays show the opponents improving in
  that window (§32: their portal use, double steps and late queen size all rose).
- **Opponent-adjusted performance** (game share v opponents' at-time ratings, ranked): s1 old ~2107, s21 ~2088,
  s1 new ~2094. Each is ±~20 (one SE), so they don't separate.
**Reading:** sprint-1's +109 is mostly (a) the big-step first battles of a new submission and (b) climbing from 2088
while the field was weaker. Sprint-21's −37 is mostly its first 20 big-step battles at our peak rating against
2206-2208 teams. At equal rating and time, the data can't tell them apart; sprint-1 relaunched is losing too. This
matches §32: in the replays the two play the same, except sprint-21's earlier queen feeding.
