# How our bot plays, what it loses to, and why (deep analysis v2, 2026-10-07)

**Scope.** Every claim comes from replays and per-game metrics, not from bot code. The rules come from `docs/game.md`.

**In short**
1. **Half our losses are lost by r60 on territory.** We are the most home-bound, most crowded bot in the meta. Opponents reach dense bed fields by r10-20; on Devil and Trophy, our two worst maps at ~1800, our heads never get there, even in wins. We also barely use free speed: a dragon of length 5-8 moves 2 tiles a turn for free, aAah takes that on 76% of such moves and we take it on 22%.
2. **The late game is the queen.** Our queen died in 207 of 288 losses.
   - 64 losses are thrown wins: their queen died first, then ours.
   - When both queens survive we lose the length race 28-56.
3. **Our queen is safe most of the game and wastes it.** At r150-450 she is enclosed 52-69% of the time and threat-free (a signal she can see in her own window) 90-100%, dying ~2-4% per 30 rounds, yet she stays 2-3 long until r450.
   - Enemy queens in the same state grow, and their big enclosed queens win 65%.
   - When she is threatened (30% death per 30 rounds), she never sprints at length 3-4, although a safe tile was in reach in 81% of those deaths.
4. **New ideas** (section 8): **Living Queen** (grow while threat-free, sprint out when threatened, never sit at length 2) and **Swarm Territory** (fast committed exploration toward seen bed density, free double steps, portal entry). A sonar relay of the enemy queen's position for long dragons ranks lower.
5. **Correction made tonight:** the replay decoder dropped multi-step moves. Portal and sprint numbers and queen-eat counts were recounted (NOTES §29); the conclusions stand, with smaller portal numbers and a new sprint gap.

**Data**
| Source | Games | Result |
|---|---|---|
| ring-sprint-1, all real server games (v17933) | 676 | 388-288, rated 2138 |
| ring-sprint-21, current live (cross-check) | 113 | 50-63 |
| Arena, ring-sprint-1 v non-ring bots | 1,832 | |
| Dev test 1 v other top teams | 99 | |
| Top-v-top (Cultery v aAah ×2, Cultery v forgot to mention, aAah v forgot to mention; all 17 maps) | 68 | |
| Ring-sprint-1 v dev test 1 | 5 | 2-3 |

**Tools** are in `tools/`; **filmstrips** I looked at are in `img/`; working notes with every number are in `NOTES.md`.

**Ladder now (2026-10-07):**
| Rank | Team | Rating |
|---|---|---|
| 1 | aAah | 2526 |
| 2 | forgot to mention | 2501 |
| 3 | Cultery (team 306, formerly Vibing++) | 2438 |
| 4 | dev test 1 | 2426 |
| 5 | larp stock | 2390 |
| 10 | horse | 2323 |
| 13 | bread first search (our worst frequent opponent, 13-39; NOTES §27) | 2242 |
| **23** | **us** | **2131** |

The goal (#10) needs about +190.

---

## 1. Where our losses come from (287 server losses, one primary cause each)
| Cause | Losses |
|---|---|
| **Lost the opening:** eval stays below .5 from ≤r60 | **150 (52%)** |
| Queen race lost with both queens alive | 38 |
| Our queen died first, after r60 | 37 |
| Lost on longest/total while holding MORE material | 28 |
| Mid/late elimination | 18 |
| Lost on longest/total with less material | 16 |

**Half our losses are decided in the first 60 rounds.** Everything else together is the other half. (This table assigns one cause per loss from the eval series; the queen table below counts end states, so its queen-race figure of 56 is larger than the 38 here.)

**The same losses by queen outcome** (675 games; queen death doesn't end the game, it sets the round-500 tiebreak):
| Our queen / their queen at the end | W | L |
|---|---|---|
| Ours alive, theirs dead | 252 | 0 |
| Ours dead, theirs alive | 0 | 104 |
| Both alive (queen length decides) | 49 | 80 |
| Both dead, ours first | 43 | 39 |
| **Both dead, theirs first** | 44 | **64** |
- **The last row is a thrown win:** we reached the 252-0 state and then lost our queen, in 64 losses (22% of all).
- **Both-alive games are queen races:** they go 28-56 on queen length, and their queen beats ours 38.5 v 20.5.
- **Our queen died in 207 of 288 losses.** Her survival and her length are where the late game is decided (sections 4, 5 and 8).

## 2. What predicts winning
- **Board control** (share of tiles closer to our heads):
  | r60 board control | Win rate |
  |---|---|
  | 0.2-0.3 | 32% |
  | 0.5-0.6 | 76% |
  | 0.7-0.8 | 90% |
  - At r30, 0.6-0.7 already wins 88%.
  - **Our board control is below 0.5 at r20-30 on 13 of 17 maps** (Islands .39, Devil .41, QoS .42, Stripes .41, weakhold .38). Only PD is clearly ours early.
- **r30 differences:**
  | Lead at r30 | Behind | Ahead |
  |---|---|---|
  | Middle eats | 36% (−4 or worse) | 72% (+4 or more) |
  | Dragon count | 38% (−3 or worse) | 72% (+3 or more) |
  | Total eaten | 45% (−6 or worse) | 78% (+6 or more) |
- **Logistic model** (accuracy .64 / .70 / .74 at r30 / 60 / 100):
  - home eating speed (+.45 at r30);
  - length lost to early deaths (−.32 to −.42);
  - middle eats (+.22 to +.32);
  - spread (+.17 to +.33);
  - by r100, dragon count dominates (+.65).
  - **Fast home eating matters, so this is "eat home fast AND claim more ground", not one instead of the other.**
- **The queen war:**
  | Queen outcome | Server | Arena |
  |---|---|---|
  | Only the enemy queen died | 252-0 | 970-0 |
  | Only our queen died | 0-104 | 0-188 |
  - Enemy queens die in 82% of our games. Queens on both sides are 2-3 long; a head-on kills both whatever their length.

## 3. Weakness 1: we start behind on territory (the 150-loss pool)
**What it is NOT** (all measured, both sides):
- **The split rule:** 74% of splits are 4 → 2+2 for both sides; r0 behaviour is identical (1.7 splits at r0; 6.3-6.7 dragons at r10).
- **Local food choice:** with a pearl in view, both sides eat 15-17%, step closer 27-31% and miss 11-14%.
- **Early self-inflicted deaths:** opponents lose as much to walls, self and failed splits as we lose to boxed resigns.
- **Sonar:** everyone pings 2-4 times a turn.

**What it IS: macro positioning, decided between r10 and r30.**
- Opponents' dragons sit next to food more often (in losses, 56% of their turns have food in view v our 51%) and they have more dragon-turns.
- Our team eats its first pearl later (r12.7 / r14.9 v r11.8 / r11.6) and makes its first post-r0 split later (r30.7 / r37.1 v r26.5 / r32.0).
- **Our spread at r30 is the lowest of every bot measured:** 8.0, against our opponents' 9.8, top-v-top teams' 10.0 and dev test 1's 11.2.
- **Portal crossings by r60** (recounted with the fixed decoder, NOTES §29): us 2.6-3.0 v opponents 4.6-6.3, top teams 8.7-12.3.
- **Sprints (multi-step moves) by r60:** us 0.6 v opponents 2.2-6.0, aAah 14.5. In r60-100 aAah makes 54.6 per game. We almost never spend the free or paid extra steps.

**Four concrete failure modes, each seen in films:**
1. **Not contesting the central bed field.**
   - On Devil, even in our WINS we have 0 heads in the middle zone at r20 (0.7 at r30) v their 1.7 (3.7); our losses look the same.
   - Bread first search, Vibing/Cultery and dev test 1 all send every starter, often including the queen, into the central lanes by r10-20 (`devil_202566_L`, `op_devil_vibing_191242`, `dt_devil_1151248`, `top_devil_1301695`).
   - Trophy: the opponent fills the cup by r26-38 while ours ride the rims (`trophy_189460_L`, `top_trophy_1301706`).
   - Map eating r0-60: Trophy 30 v 60, Devil 38 v 81.
2. **Not leaving our own base or maze.**
   - **Tower Defense v bread first search:** we sit in our spiral tower with total length 4-5 for 60 rounds; they walk out at ~r55 onto the middle field (`op_tower_bfs_206942`).
   - **Trauma:** all food is in the central maze; they enter by r20-30; we stay in our empty base until ~r50-60 (`trauma_208818_open`). Trauma eating r0-60: 7.8 v 18.7.
   - Corridor-tile eating r0-100: Trauma 14 v 35, Islands 42 v 78, Portals 85 v 117.
3. **Not entering portal-gated food.**
   - **Stripes:** our starters eat NOTHING for 40 rounds next to portal-entered 2x2 boxes (`stripes_190622_L`).
   - Across maps our portal use is 1/2 to 1/4 of opponents'.
4. **Crowding on many-starter maps.** Slithery v Preyas: we lead 24-16 dragons at r10, then stall while boxing ourselves in (28 boxed resigns in r0-30). 29 v 46 dragons at r30 (`op_slith_preyas_200526`).

## 4. Weakness 2: our queen (37 + part of the opening pool)
- **Death modes** (294 deaths):
  | Mode | Deaths |
  |---|---|
  | Far from home (≥8 tiles from spawn) | 97 |
  | Lunged near home | 90 |
  | Stepped into near home | 77 |
  | Boxed | 30 |
  - Killers are mostly 2-3-long stubs; there is no ally within 3 in roughly half the deaths.
- **By map:** Trophy 87%, Islands 79%, QoS 77%, Default 74%, Devil 69%, Stripes 67%, against Maze 6%, Trauma 10%, Slithery 13%, Portals 14%.
- **QoS:** 24 of 34 deaths are far from home. She walks to the torus edge, which borders the enemy spawn (`qos_190705_queenpath`, `dt_vs_us_qos_185498`).
- **The top teams do the opposite:** their queens travel WITH the swarm, 9-11 tiles from spawn at r30-200 (ours 1-2) and inside its mass (`top_trophy_1301706`: the queen in the cup among 9 own dragons). Top queens still die often (68% per side), but they die as part of a winning economy.

- **Her last move** (260 head-on deaths; NOTES §25):
  - she moves CLOSER to the killer in 110 and keeps the same distance in 142;
  - at length 3-4 she takes a single step in all 118 cases (sprints happen only at length ≥5: 24 of 260);
  - enemy queens do the same (6 of 105 sprint at length 3-4; top teams 2 of 29).
  - **A tiny queen is slower than every hunter.** At length 2 she gets 1 free step and no paid steps, while a 3-long hunter reaches 2 tiles. 73% of her length-2 deaths had no safe tile in reach at all.
  - **At length 3-4 she had a safe tile in reach in 81% of deaths but used the single-step move.** One step reached safety in only 41%. About 48 deaths were escapable by spending one segment.
- **Thrown wins** (64 losses; NOTES §23):
  - their queen dies at a median r100 and ours at r273;
  - when we made the kill we had 12 dragons v their 23 (in the 44 such games we won: 24.5 v 21.5);
  - afterwards they grow to 40 while we hold 12-15, and our queen is enclosed in only 17% of samples (52-69% normally);
  - she then dies head-on to an enemy, 40 of 63 times to a lunge;
  - films `thrown_devil_202570` and `thrown_devil_206390`: by r260-330 she is our last dragon, against 45-62 of theirs, and survives 60-70 rounds alone before they catch her.
- **Elimination losses** (125; NOTES §22):
  - we stop growing at r40-60 and sit at 6-7 dragons for 100-200 rounds while they go 9 → 13 → 22 → 32;
  - our queen dies a median 31 rounds before the wipe, with 3 dragons left v their 27;
  - this is the opening failure carried to its end: a starved periphery (Trophy corners, Devil side lanes).

## 5. Weakness 3: converting late (38 + 28 + 16 losses)
- **Our queen is 2-3 long at r400 in 402 of 463 live games,** and still 2-3 at r499 in 222 of 425.
- **Queen-map losses** (Trauma, Slithery, Portals, weakhold): their queen is 14-18 at r450 v our 2-4. They grow it from r200-400; ours grows only in the last ~50 rounds, to 15-30.
  - Enemy queens ≥10 by r300 die 63% overall, but that splits by enclosure: reachable ones die 93%, enclosed ones 35% (NOTES §19).
- **Longest-map losses** (Schooltime, Islands, Australia, Around UNSW):
  - Their champion is already 22.7 at r380 v our 10.9.
  - 24% of their late death length lands within 3 of their champion v our 13.7%.
  - Final 48.6 v 24.
- **Schooltime:** both queens are sealed at length 3, so the longest dragon decides. Bread first search grows a 113-126 champion in the walled bottom room (`school_191752_late`), and we leave that room full while holding 3x the material (`school_202411_end`).
- **The top teams' end state:**
  - On fight maps the winner fills the unit cap (50-64 dragons) and eliminates (60% of top-v-top games end by elimination).
  - On queen maps the winner sometimes ends with ONLY its queen (Portals: 1 dragon, queen 41, `portals_196688_late`).

## 6. What we do well (keep it)
- **Queen safety near home:** enemies convert only 7% of their chances on our queen.
- **Queen assassination:** short hunters (length 2-4) strike 97% of their chances (NOTES §31); their queen dies in 82% of games; 252-0 when only theirs dies.
- **Strong maps:** Maze 34-2, PD 39-8, Tower Defense 20-7, Around UNSW 33-10. These are maps where the queen war decides and the opponent's queen wanders.
- **Against dev test 1:** 2-3 (wins on Maze, PD). Against the #6 team: 8-1.

## 7. Map loss types (how each map is lost)
| Type | Maps | How it's lost | What wins it |
|---|---|---|---|
| ELIMINATION | Trophy (1849), Devil (1894), QoS (2020), Stripes (2038) | Opening territory, our queen dies (67-87%) | Contest the field from r10, queen inside the swarm, portal entry |
| LONGEST | Schooltime, Islands, Australia, Around UNSW | Their champion | A champion from ~r300, feeding next to it, vault rooms |
| QUEEN | Trauma, Slithery, Portals, weakhold | Their queen 14-18 at r450 | Living Queen: threat-free growth from ~r250 |
| OURS | Maze, PD, Tower Defense, Autarky | (we win) | Keep |

## 8. Strategy: a new doctrine, not a tweak

**Two new ideas, ranked by the losses they reach:**
| # | Idea | What is new | Loss pools it targets |
|---|---|---|---|
| 1 | **Living Queen**: the queen grows while threat-free, sprints out of reach when threatened, never sits at length 2 | No team in the meta conditions growth on what the queen can see, and small queens don't flee with a sprint (at length 3-4: ours 0 of 118 deaths, opponents 6 of 105, top teams 2 of 29) | Our queen died in 207/288 losses (64 of them thrown wins); 56 queen-length races |
| 2 | **Swarm Territory**, heading for bed clusters | We are the most home-bound bot. Opponents reach the dense bed fields by r10-20; we never do on Devil and Trophy | 150 opening losses; 125 eliminations, which are the same failure 200 rounds later |

Idea 1 is cheap and orthogonal to everything else, so it can go first and alone: one change at a time on the head. Idea 2 is the bigger rebuild.

### Blueprint check: aAah (#2) already plays this way
Measured on 102 top-v-top games:
- **aAah** has the best board control (0.54 at r30), the most eating (75 by r60), the most portal use (12.3 crossings by r60) and by far the most sprinting (14.5 multi-step moves by r60, 54.6 in r60-100) and the best queen survival among top teams (51% died).
- **It never grows its queen** (1.8 at r300-450), exactly like us.
- **Its dragons are evenly spaced:** only 29% have an ally within 2 at r30, against our 43%, the most crowded bot measured.
- **Its queen roams 11-17 tiles out INSIDE its carpet** with ~2 escorts.
- **Among top teams, the board-control leader wins 72% at r60** (30-4 with a ≥0.2 lead).

Films: `aaah_islands_1301696` (an even carpet, 58 dragons by r86), `aaah_qos_1263292` (claims the top bed room by r14).

### Idea 2 in detail: "Swarm Territory"
Play the game as area control from round 0.
- **Objective:** maximise board control (Voronoi share of heads), the single best predictor of winning (r60: 32% → 90% across bins). Food, queen access and elimination all follow from it.
- **Doctrine:**
  - every worker's goal is scored by the territory it claims plus the food it reaches;
  - the swarm advances as a front through explored ground: starters commit to outward headings (fast exploration), and the front follows seen bed density, with no centre or mirror target (your anti-overfitting rule);
  - home food is eaten fast by the newest children behind the front.
  - The g43 workers found that a FOOD bonus loses to nearby home food. Territory is a different, positional objective: a worker standing on uncontested ground behind the front adds nothing; one at the front adds a lot.
- **Movement competence is part of it:**
  - leave spawn mazes, towers and bases (Tower Defense, Trauma);
  - cross portals into gated rooms (Stripes, Trophy, Default);
  - don't over-crowd on many-starter maps (Slithery).
  - **use free speed:** a dragon of length 5-8 moves 2 tiles a turn for free (first ceil(L/4) steps are free).
    - aAah keeps several dragons at 5-8 early and takes the double step on 76% of their moves (238 such moves per game in r0-150).
    - We have 43 such moves and take the second step on 22%.
    - In our losses we make 22 moves per game at length ≥5 against the opponent's 89 (NOTES §30).
    - Scouts and front-runners at length 5-8 that always take both free steps are a cheap way to reach bed clusters first.
- **Evidence it is THE lever:** 150/287 losses are opening losses; we are the most home-bound bot in the meta; board control at r30 is below .5 on 13/17 maps; the top teams' spread and portal crossings are about 1.3x and 3-4x ours, and they sprint far more.

### Companion ideas that ride on the territory front
- **Queen in the swarm, not in a corner.**
  - Our queen dies most where she sits alone (QoS 24/34 far from home, unescorted).
  - Top teams carry the queen inside their swarm's mass (9-11 tiles out, inside the cup or lanes).
  - With a forward front the "safe place" moves: the queen stays a few tiles behind the front, surrounded by our own bodies, never at the torus edge next to the enemy spawn.
- **Long lunges at the queen: an information problem** (NOTES §31):
  - short hunters (length 2-4) strike 97% of their chances;
  - dragons of length ≥5 had 1,208 chances to reach the enemy queen this turn (median 15 long, 13 steps, ~r408), but in 1,077 she was outside their 7x7 window. They struck 0% of those and 79% of the 131 visible ones;
  - games where such a chance came with both queens alive: 57-6 when we eventually struck, 16-12 when we never did.
  - **Idea:** whoever sees the enemy queen relays her position by sonar, and any long dragon within reach lunges. A long dragon's reach (ceil(L/4)+L-2) is far bigger than its window, so today most of its range is blind.
  - **Feasibility is modest:** at those moments another of our dragons saw her in only 9% of cases (7 of 23 sample games). The relay needs remembered sightings, not just same-turn ones; a small queen moves 1 tile a turn. Rank it below Ideas 1 and 2.
  - Enemy long dragons strike only 24% even when they see our queen. Their restraint is part of why ours survives.
- **Hunters from the front.**
  - Queens are ids 0/1 and visible; the enemy queen lives 2-5 tiles from her spawn.
  - In 57% of games we never once had her in reach (win 44% v 75% when we did); the bottleneck is reach, not conversion.
  - The front brings cheap 3-4-long hunters to her doorstep.

### Idea 1 in detail: "Living Queen" (the queen grows while threat-free)
This is the cleverest cheap lever in the data, and it is not a tweak of the r450 feeding.
- **Our queen is unreachable most of the game.** At r150-450 no enemy head can reach her through free tiles in 52-69% of samples. That pen is the back of our own territory: median 364 free tiles and 23 beds, with 7-12 uncollected pearls inside. In 46% of samples its border is entirely our own bodies.
- **While unreachable she dies at 0-1.5% per 30 rounds.** Yet she stays 2.5-2.8 long, because she splits away what she eats and we only feed her after r450.
- **Enemy queens in the same situation grow:** 5.7 at r250-350, 10.7 at r350-450, 19.1 at r450+. They are fed by their own sacrifices from r250.
- **It decides games:**
  - big enemy queens that were enclosed at r300 won 65%; reachable ones won 22%;
  - the queen-length tiebreak decides the QUEEN maps (Trauma, Slithery, Portals, weakhold: their 14-18 v our 2-4 at r450);
  - we lose queen-length races with both queens alive 28-56; their queen beats ours 38.5 v 20.5 at r500.
- **The bot can see it.** Bots only have the 7x7 window, so enclosure itself is invisible. Two window-local signals work (NOTES §19, §24, §26):
  - **quiet:** no enemy segment in her window for 30 rounds. Reachable-but-quiet queens die 1.9% per 30 rounds, against 14.0% with an enemy in view recently. But the window sees through kelp, so on walled queen maps she always "sees" enemies she can't reach: on Portals our queen is quiet 0% of the time while enclosed 100%.
  - **threat-free (the one to use):** for 20 rounds, no enemy head in her window has had a free path to her inside the window within its move reach + 2.
    - threat-free: 3.6% per 30 rounds (2.3% if also quiet, 1.6% if also enclosed);
    - threatened: 30.7%, whatever her length (29% at L≤4, 42% at L≥15);
    - our queen is threat-free 90-100% of r250-450 on every map but Default, including Portals, Slithery and Schooltime.
  - Measured on 9,356 samples (70 sprint-1 and 113 sprint-21 games).
- **Doctrine:**
  - while threat-free, the queen stops splitting and eats the pearls around her;
  - from ~r250, workers inside our territory that run out of useful work die next to her (resign/self) to feed her, as every top team does;
  - the moment she is threatened, stop feeding and keep her among our bodies (big queens die head-on in 85% of cases, so length is not the danger; reach is);
  - **length is speed:** never let her drop to 2 (73% of her length-2 deaths were inescapable). When an enemy head can reach her, she sprints, spending segments, to a tile outside every visible enemy's reach; at length 3-4 such a tile existed in 81% of her deaths. Growth and safety pull the same way.
    - At length 5 she moves 2 tiles a turn for free and can pay for 3 more, which out-runs a 3-long hunter's 2-tile lunge.
    - Today threatened queens die at the same rate whatever their length (29-42%): small queens never run, and bigger ones rarely do. The speed is there but unused.
    - 57% of her deaths after r60 came with a visible threat in the preceding ~20-30 rounds; the rest came fast, so the sprint at the moment of threat matters as much as the early warning.
- **What feeding costs:** a dead dragon drops a pearl on every second segment from the head, so a 3-long sacrifice gives 2 pearls and a 2-long one gives 1.
  - In the 56 queen-race losses the opponent's queen ate ~6 drops per game in r250-350 and ~16 in r350-450 (NOTES §29). That's on the order of 10-20 small dragons over 200 rounds, from a side that holds 30-60.
  - Our own late feed already runs at ~30 eats per 100 queen-turns (r450+). Started ~100 rounds earlier while threat-free, it would cover the median 16-length gap in most of those losses: 37 of 56 had margins ≤18.
- **Fortress films:** `fort_trauma_1308235` (forgot's queen 14 → 49 in its base behind its maze front), `fort_slith_1308232` (17 → 90 behind its half of a full-cap carpet), `cultery_portals_qgrow`.
- **Fit with the user rule "queen plays safe":** this doesn't ask her to forage into mazes or pockets. She grows only where nothing hostile has been seen, and that is exactly the condition the data shows is safe.

- **Replicated** on ring-sprint-21's 113 server games: quiet or enclosed 1.1-3.5% per 30 rounds, exposed 16.4%. Length doesn't change it: when safe, 2.0% at L≤4 v 2.7% at L≥15.
- **The won position:** once their queen is dead, our queen's survival is the whole game (252-0). In 64 losses she died anyway. The same quiet/enclosed discipline is what keeps her alive.
  - Our team only knows it is in that position if someone saw the kill. The killer dies in the head-on and sonar is cast only by survivors, but another of our dragons had her in its window in 43% of enemy queen deaths.

### Devil and Trophy: go to the bed clusters
- **These are our two worst server maps** (1814 and 1857, ~200 below every other map).
- **On both, our heads never reach the bed clusters, even in games we win.** Beds within 3 of a head at r40, us v them:
  - Devil: 5.7 v 8.6 in wins, 5.4 v 8.5 in losses;
  - Trophy: 8.4 v 9.3 in wins, 8.2 v 11.2 in losses.
- **Top teams sit at our opponents' level there,** with medians of ~7 on Devil and ~10 on Trophy.
- **In the Devil film the gap is visible at r10:** their six starters are on the central 2x2 clusters while ours are still within 5 tiles of spawn. Our starters then work the lanes on our own side.
- **Across all maps,** starters are 1.5-3.5 tiles closer to spawn than the opponent's at r20, and net displacement is 5.4 v 6.1 tiles per 10 moves. With only a 7x7 window, their rush is a committed heading: leave spawn and keep going.
  - **Your rule applies here:** a geometric-centre pull is a map assumption, and the arena's trick maps punish it.
  - The copyable part is **fast exploration with commitment**: starters fan out and keep a heading until they see beds, then report bed sightings by sonar so the swarm converges on dense regions it has actually seen.
  - On Devil our r10 heads saw 2 beds within 3 against their 5. A sighting relay only helps once someone has gone far enough to see the field, so commitment comes first.
- **More generally:** in our losses the opponent's heads sit among ~30% more beds from r20; ours are the same in wins and losses. In top-v-top games, the side with more beds near its heads at r40 wins 70% (70 of 101).

### End-game planner (the 82-loss late pools)
From ~r300, read which tiebreak is live and feed it:
- **Queens sealed or equal:** build one champion early, ideally in a walled bed room, and let our late deaths happen next to it (theirs 24% v ours 14%).
- **Both queens alive:** grow ours whenever she is threat-free (see Living Queen), from ~r250.
- **Theirs dead, ours alive:** stop culling, protect numbers (Autarky 192217 gutted itself from 25 to 2).
- **Fight maps:** fill the unit cap and eliminate, as the top teams do.

### What NOT to do (evidence against)
- **Don't skip home food:** home eating speed is the top r30 coefficient.
- **Don't change the split rule** (identical to everyone's) **or local food pursuit** (identical).
- **Don't add sonar chatter** for its own sake (everyone already pings 2-4 per turn).
- **Don't grow the queen where enemies can reach her:** enemy queens ≥10 at r300 that our heads could reach died 93% (their side won 22%). Enclosed ones died 35% and won 65% (NOTES §19). The old "never before r300" rule was this effect averaged.
- **Don't look for an edge in trade efficiency:** head-on exchanges are ~1:1 in length in both wins and losses (NOTES §16).
  Top teams trade more because they meet more often, which follows from holding a front.

### How to check each idea in replays (behaviour first, metrics as evidence)
The tools in `tools/` run on any arena or server replay set. The decoder was fixed for multi-step moves; see NOTES §29.
- **Living Queen**
  - `qthreat.py`: our queen's death rate while threat-free should stay ~2-4% per 30 rounds as she grows.
  - `qlife.py`: queen eats per 100 turns from r250 when threat-free (today 0.6 bed + 0.6 drop).
  - `qlast.py` / `qescape.py`: multi-step escapes at length 3-4 (today 0 of 118), and the share of head-on deaths with an escape tile left unused.
  - Films: the queen sprinting away when an enemy head with a path appears. `fortress.py`: our queen's length at r300/r400 against theirs.
- **Thrown wins:** the "both dead, theirs first" row of the queen-outcome table (today 44-64).
- **Swarm Territory and speed**
  - `bedprox.py`: beds within 3 of our heads at r20-40 (Devil today 5 v 8.5; Trophy 8 v 11).
  - `portsprint.py` / `sprintuse.py`: portal crossings and free double steps at length ≥5 (today 22% use).
  - `bc.py`: board control at r30/r60.
  - Films: Devil and Trophy r0-40 sheets, where our starters should be in the bed fields by r20.
- **Hunting:** `qmiss.py`: long-dragon chances at the enemy queen taken while our queen lives.

## 9. ring-sprint-1 v ring-sprint-21 in replays (NOTES §32)
I compared all 676 sprint-1 server games, all 588 sprint-21 server games, and 149 sprint-1 games played after sprint-21 (a time-matched control, since opponents keep updating).
- **Sprint-21 plays like sprint-1.** The same tools give the same numbers for:
  - dragon counts r30-r300, spread, board control, beds near heads, portal crossings, sprints and free double-step use;
  - small-start openings (r20/40/60: 3.9/5.8/7.9 v 3.8/5.9/8.2);
  - long workers lunged to death (4.18 v 4.38 per game, the same as the control's 4.18);
  - late queen pocket deaths;
  - the queen's last move: she never sprints at length 3-4.
  - So three of its four intended changes (founder dispatch, lunge guard, queen pocket fix) leave no visible trace on server maps.
- **The visible change is the feeding conveyor.** Our queen eats 2.5x more in r350-450 (4.1 v 1.6 per 100 turns) and is bigger at r450+ (12.2 v 8.8).
  - But queen races still go 18-53. Opponents in that period feed earlier and harder: their queens are 26-30 at r450+ and eat 15-18 per 100 turns in r350-450.
  - Film `s21_portals_214722`: by r405 the opponent has fed everything into one 71-long queen. Ours goes from 4 to 34 from r430 on, while our 7-11 dragons never hunt their lone queen.
- **Elo** (NOTES §33): sprint-1 gained +109 and sprint-21 lost −37, but the totals mislead.
  - A new submission's first ~20 ranked battles move 10-20 Elo each; later ones move 3-6. Sprint-1's first 10 made +72. Sprint-21's battles 10-20 cost −32, mostly three losses to horse and WeHaveQuizzes (2206-2208) in its first hour, from our peak of 2197.
  - Sprint-1 also started below its level (2088) while the field was weaker.
  - At the same rating band the two are within noise: at 2140-2175, sprint-21 makes +1.4 per battle, sprint-1 −1.5 (old) and −1.3 (relaunched).
  - Opponent-adjusted performance: sprint-1 ~2107, sprint-21 ~2088, sprint-1 relaunched ~2094, each ±~20.
  - Sprint-1 relaunched is losing Elo too (−29 over 21 battles). The field got stronger; the replays agree.
- **The field is pulling away on exactly the axes in sections 3 and 8.** Between the two periods, opponents' portal crossings rose 6.6 → 8.0, free double-step use 0.39 → 0.48 and late queen length 18.8 → 29.8, while ours stayed flat. Our r60 board control in the control set fell to .438. Incremental changes on this line are not closing the gap; the ideas in section 8 target these axes directly.

