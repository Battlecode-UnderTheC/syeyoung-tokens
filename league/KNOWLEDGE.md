# What we know

Claims about the game and about strategies, each with its evidence and when it was last
confirmed. The orchestrator curates this after every generation: add what a result
supports, mark what a later generation confirms or contradicts, delete what is disproved.
Workers read it before starting; challenge a claim when your evidence disagrees.

Sources: `EXPERIMENTS` = syeyoung-rl `strategy/EXPERIMENTS.md` (strategy-v1's hand tuning
against the RL policy, native engine, 2026-09-28). `g<N>` = `league/generations/g<N>.md`.

## Numbers and trades

- **More dragons win the head-to-head attrition on small, crowded maps.** Splitting earlier
  kept helping: split at length 12 / 8 / 6 went 53-27 / 69-11 / 74-6 on 40 pool maps, and
  4 vs 6 went 22-0 vs 20-2 on the official maps (the losses were arena and Colosseum, where
  the opponent's swarm won one-for-one trades). (EXPERIMENTS)
- **Don't trade evenly on a guess that you outnumber the enemy.** A dragon sees few enemies
  in its 7×7 window, so "we outnumber them" guesses are wrong; trading on them dropped 69-11
  to 47-33. Trading only for an enemy at least as long is fine (trade margin 0 vs 2: 69-11 vs 65-15).
  Requiring 2× length when outnumbered was about neutral (79-1 vs 78-2). (EXPERIMENTS)
- **The lunge is a weapon nobody guards against.** A MOVE may take up to length − 1 steps,
  so a dragon of length L kills an enemy head up to L − 1 free steps away in one turn (nothing
  else moves during our turn). swarm-hunt-1 uses it (~21 kills a match); turning it off
  dropped 60-20 to 46-34 vs strategy-v1 on 80 worldgen matches. Guarding only the tiles next
  to enemy heads (strategy-v1 and descendants) leaves you open. (g1-5)
- **Fleeing loses.** Turning off a survivor mode (the last dragons avoid all contact)
  improved 74-6 to 78-2. (EXPERIMENTS)

## Movement and safety

- **Track your own body.** Portal exits outside the window caused wall and self deaths
  until the agent remembered its own last `length` head tiles. (EXPERIMENTS)
- **When trapped, split (length − 2).** The child's head is the parent's tail, so it walks
  out the way the parent came. This turned trapped deaths into splits: 8-14 → 18-4 on the
  official maps. (EXPERIMENTS)
- **Trapped deaths are mostly caused by our own allies,** not corridors: debug dumps from
  two workers show length-2/3 workers whose last free tile an ally's body filled, often near
  the 64-dragon unit limit. Stricter room checks (room_margin 6, room_min 14) cut them only
  ~8% on worldgen maps, raised them 9–20% on official ones, and rated 1752 vs strategy-v1's
  1775 (gather-split-trade-2, g1-6). A body-simulated look-ahead barely moved them per split
  (0.55 → 0.46). (g1-1, g1-5, g1-6)
- **Small dragons still die trapped a lot.** strategy-v1 loses ~47 dragons per league match
  to "trapped: no move" (no safe step, resigns), and 3–4× more on worldgen maps (mazes, 1-wide
  warrens). Room checks before entering corridors, or avoiding splits into pockets, are the
  obvious fix. (league reports, 2026-09-28)

## Targets and economy

- **Consolidate into one champion late: the biggest single gain so far.** 70% of the
  leaders' losses were round-499 longest-dragon losses with more total length. A 64-bit
  sonar word pinged in all four directions each turn, naming the longest ally known
  (relayed, newest per id), lets the team agree on one champion; workers feed it from ~360
  and die only when they see it within reach. gather-split-trade-4 beat its parent 29-11
  (+70 Elo); counter-swarm-5 +30. Feed deaths are still only ~5–8 a match: reaching the
  champion is the limit. Feeding from 260 is clearly worse than 300–360. (g2-3, g3-1, g3-3)
- **Ghost champions (a bug in every champion-feeding variant up to gst22):** a trapped
  champion splits (length − 2); the child walks off with the mass under a new id and the stub
  that keeps the champion's id dies. Reports ranked by length with a 40-round expiry keep
  naming the dead one: 50–100% of our dragons fed a ghost from round 340 on. Rank by
  `length − 0.5 × age` (gather-split-trade-23): the live heir takes over in a few rounds;
  longest at 499 went 39 → 78 in a traced game. (g5-7)
  Where ghosts still come from (g15-22, gst62 probe): the champion splits most of its length off and
  the short stub under its id dies; the stale reports are young (10-21 rounds), and on the split
  turn the dragon still pings itself at full length, seeding its child and neighbours. Announcing
  the new short length (and starting the obituary) on trapped splits of 10+ dragons cut those ghosts
  186 -> 52 at r450 (longest's share of team length .32 -> .385). Announcing routine growth splits
  too cost sonar turns and lost.
  Saturated for good (g16-22): an obituary for a champion missing from its reported tile plus a faster
  late fade cut ghost-champion team-rounds ~80% (15-20% of late rounds on worldgen) with no W-D-L change.
  Split announce ported: helps gst/bw/ls, neutral in cs/swh. (g16-13, g16-28..30)
- **The cap trap is set before the cap is reached:** of ~2,100 length ≥ 4 resignations at the
  cap (40 worldgen matches), almost none had a shrinking-room trend: rooms read open for many
  turns then 1–3 the next (bodies close gaps within a round), or the dragon entered the pocket
  while it could still split out and other splits filled the cap. Avoiding crowds (g6-7) and
  reacting at the cap (g6-13) both did nothing; forecasting the cap before trusting a split
  escape didn't either (g6-16): long dragons walk in on steps that looked open. Caution on the
  long dragon's side is a dead end; try the supply side (fewer idle slots). (g6-13, g6-16)
  Confirmed in detail (g15-18): of body-wall's trapped deaths, the turn before was almost never
  'toward goal' (42 of 8,860); rooms flip from 60+ tiles to 1–2 within one round when 3+ heads are
  near; every one of 2,047 trapped deaths of length ≥ 4 was at the unit cap with no split possible.
  Supply side works when broad: at <= 1 free slot, idle 2-3-long stubs among 3+ heads resign
  (body-wall-21): trapped deaths -10-35%, vs trading-swarm-5 83-19 (parent 77-25), even in the mirror. (g15-24)
  Ported to gather-split-trade-70, counter-swarm-32, swarm-hunt-26: cap-trapped deaths fell 5-7x in
  every line (e.g. 170 -> 27 a match on worldgen), W-D-L even vs parents. (g16-3, g16-9, g16-10)
- **Partial sightings under-report the champion (likely in every champion-feeding line):** an
  ally whose body runs past the 7×7 window is reported at its visible length, stamped fresh, so
  it beats the champion's own exact report and mid-size dragons elect themselves. At r460 on
  trauma only 0–3 of ~10 dragons knew the real champion. Fix: flag partial reports as lower
  bounds that never lower a known length (swarm-hunt-13; neutral in sims, but the mechanism is
  real). Isolated champions in portal pockets still don't get fed. (g11-6)
  Farming portal-bounded pockets raises pocket pearls a lot (default 25 → 437 a match) but not
  wins (portal-farmer-1/-2 even with gst39): matches follow the longest dragon and consolidation,
  not gathering totals. Trauma's entry portals lead into maze corridors, not to the stores seen
  through the wall. (g12-25, g13-3)
  At r400 (4 portal maps vs gst39) 61 of 136 dragons knew no path to the team champion, only 10
  of them in pockets; pockets held ~3.6 length a dragon. Feeding a regional champion instead
  lost (44-58): the team's length ends split. (g13-12) "No known path" mostly means unexplored, not
  walled off: 80–90% of such feeders have a path through unseen ground, ~1% are boxed in by portals.
  Letting feeders of length ≥ 6 path through unseen ground from r380 delivered 25% more length to
  the champion but barely moved wins (33-29 on unbiased sets); every feeder seeking lost (stops
  gathering). (g13-20)
  Mostly the team names a DEAD champion (39 of 48 dragons naming one dead id), not an isolated one:
  the slower late fade keeps its old report alive and the corrected stub report never spreads.
  Obituary pings (relay a sharply shrunk or dead champion's correction for 20 rounds) raised
  knowledge 48% → 64% on a worldgen set; gather-split-trade-39 55-47. (g12-10)
  A heartbeat (drop a champion whose self-report round stops rising) cut dead-champion naming
  further but didn't move wins (48-54): the champion-knowledge lever is saturated; most games are
  decided before the endgame. (g12-19)
- **Trapped long dragons resign whole at the unit cap** (no split escape): ~110 dragons of
  length 4–19 in one worldgen match. (g5-7)
- **Don't grow the champion early:** a lone gatherer grows ~0.05–0.1 length a round; an heir
  that stops splitting from round 150–200 cost the swarm more than it gained (15-25 on
  worldgen at 150). Freezing the champion from 300 (no splits/trades/lunges) went 57-45. (g5-6)
- **Feeding logistics:** holding the champion still (13-27 on worldgen) and walking feeders
  to stale reports both lose; routing feeders raised feed deaths 25% but not the champion's
  length, since a walking feeder stops gathering. (g3-6)
- **Defending against strikes/lunges didn't pay** (g2-2, g2-7: even or worse); neither did a
  look-ahead safety layer on counter-swarm-1 (g2-5: 16-24 vs parent). (g2)

- **Target sharing is a big win:** a worker leaves a pearl or bed to an ally in view that is
  nearer to it. It was gather-split-trade-1's biggest single gain (rated 1922 vs
  strategy-v1's 1775, 45-0-17 head to head); swarm-hunt-1 found the same. (g1-1, g1-5)

- **Feeding, exploration weight and dead-end cost matter little** for strategy-v1: sweeps of
  `feed_from_round`, `explore_value`, `trap_cost` moved results by a few games in 80.
  (EXPERIMENTS)

- **Our openings are slow (watched, 13 local games; unconfirmed by a variant yet).** In r0–200 the
  ts4 opponent clone eats 2–5× more pearls per 100 moves than our lines (Devil 15.7 vs 8.6–9.9,
  Trauma 11.6 vs 2.0–3.5). Our explore goal is the nearest unseen tile (dragons dither along the
  spawn edge while the enemy takes the central field), a dragon waits on a bed about to ripen next
  to a full pearl field (a bed scores `max(dist, due)`, fields count as single pearls), and goals
  change before they are reached on 36–50% of turns. A known portal is a one-way door: its far end
  is out of view, so the blind-step liberty cap turns every goal step onto it into a detour (653 of
  1,048 unsafe detours in one game). Tasks: g13-5 (portal); opening/bed scoring next. Replays and
  films: `../syeyoung-tokens.league/replays/league-1/`, VISUAL.md #11–17. (league-1 watch)

- **Charge bed waiting; value pearl clusters (late-surge-17 62-40 vs its parent, 71-31 vs the ts4
  clone).** A bed was scored ready at `max(dist, due)`, so waiting 5 rounds on a bed tied with
  walking 5 steps to a pearl. Charging `dist + 2·max(0, due − dist)` plus 0.5× the value of other
  pearls/beds within 2 steps won on official and worldgen maps. Weighting explore by unseen area
  and a centre pull for the first 60 rounds were neutral to harmful (the centre pull hurt). Goal
  stickiness (×1.2–1.5 on last turn's goal) was flat (gst44 51-51). Confirmed in every line it was ported to:
  body-wall-12 35-27, swarm-hunt-19 41-21, counter-swarm-25 42-20 vs their parents. (g13-6, g13-11,
  g13-17, g13-19, g13-22)
  The constants (wait ×2, cluster bonus 0.5, radius 2) are near best: ×3, bonus 0.3/0.8 lost clearly;
  ×1.5 went 37-25 then 17-23 on a second worldgen set (always confirm on a second set). (g13-23)
- **Known portals were one-way doors.** The blind-step liberty cap refused every goal step onto a
  known portal (its exit is always out of view). gather-split-trade-42 crosses after a clean ping,
  an exit seen empty within 2 rounds, or 2 refusals: crossings 68 → 231 a match on default; even
  in sims (53-49) but #2 in the league with holdout 100-50. (g13-5) Its cost, measured: on 4 official
  portal maps 143 lifted crossings, 36 deaths within 2 rounds, mostly head-to-head on the crossing
  move into an exit with 3 unknown sides (2 unknown sides: 2 of 57 died). An enemy beside the exit
  was known in only 2 of 248 crossings, so memory-based guards never fire (gst43). Vetoing the
  fallback / exit-seen-empty crossings into open exits halved those deaths but stayed even (gst46
  51-51); vetoing pinged ones too brought back portal pacing. (g13-18)

- **late-surge's trader switch is nearly inert by late-surge-17.** Making it decay (on 57% → 35% of
  dragon-turns) left every action count and death rate unchanged vs late-surge-17 (11-11, 0.0
  margin). Don't spend tasks on it. (g12-24, g13-21)

- **Crowded midgame exploration pays in every line.** When a dragon sees > 3 allies, long-unseen
  frontier tiles get explore_value × min(4, age/50) and patrol competes inside the goal scan
  (swarm-hunt-17's idea): gst49 34-28, late-surge-21 35-27, counter-swarm-26 37-25, body-wall-14
  36-26 vs their parents, mostly on worldgen (+31..+62 total). Port it with a per-ChooseGoal mine
  mask: Occupied(t) per tile walks the body (O(board × length), 8 → 52 ms). (g13-28, g14-2/3/5)

- **Champion dead-end skip + unknown-portal length gate (body-wall-15, league #1):** a frozen
  champion skips dead-end goals; unknown portals are crossed only at length ≥ 4 when a split is
  possible (a 3-kelp exit killed 17 of 17 crossers). body-wall-15 53-27 on worldgen, counter-swarm-29
  45-35, late-surge-24 46-34, swarm-hunt-24 thin (33-29) — but gather-split-trade-51 36-44: its
  cap-safety no-trap gate already covers the champion case and an outright dead-end ban costs
  growth there; the portal gate alone read 21-19. The fix depends on the line. (g14-15, g15-1..4)

- **The losing side's flat midgame is a numbers problem, not a pickup problem.** Trade corpses:
  <1% left uneaten; the winner eats ~53% (≈30 pearls a match more). Goal steps refused next to an
  enemy head: ~1% of turns, ~4 pearls a match if loosened. The loser has fewer dragons at each trade
  and fewer allies to recycle its trapped dead. Fix earlier (dragons alive when sides meet).
  Letting an enemy head contest only its 2 nearest targets (gst56) helped a little (55-47). (g15-6)

- **A mild early centre pull on small maps beats swarms.** Multiplying goal scores by
  1 + 0.3 × closeness-to-centre before r50 on maps ≤ 1,024 tiles (most worldgen maps qualify): body-wall-17
  83-19 vs trading-swarm-5 (parent 73-29), 54-48 vs its parent; at 1.0 it lost 8-14 (the team crowds
  the middle, trapped deaths double). A split needs length ≥ 4 (2 ≤ n ≤ length − 2). (g15-14)
  Confirmed in other lines: gather-split-trade-63 32-30 and league #1, late-surge-27 34-28,
  counter-swarm-31 35-27; all better vs trading-swarm-5. (g15-20, g15-23, g15-26)
  Its settings (0.3, until r50, <= 1,024 tiles) are a local optimum: 0.2 and until r80 lost, 0.45,
  until r30 and 1,600 tiles were within noise. (g15-30)
  Devil is not about fast beds: learning each bed's gap exactly (the countdown reads the newly drawn
  gap the round it's drawn) and valuing fast beds changed nothing (body-wall-20). There the swarm has
  20+ dragons (avg length ~2.5) by r50 to our 7-10: the gap is how many dragons we field. (g15-19)
  Not the split rule: on small maps our dragons split on 160 of 166 turns at length >= 4 (always
  SPLIT 2, like the swarm; 6 blocked by SplitSafe). The gap is eating: on devil at r100 we have 4
  dragons / 11 length to its 31 / 79, and its lunges start even trades that favour the bigger side. (g15-25)

- **Don't trigger consolidation on a static "champion still short" test.** Starting to feed at r300
  when the champion is <= 6 lost 13-27 on worldgen: it fires on games that are still growing normally.
  Round-robin staggering of feeders cost more than the corpse pile-ups it avoided. (g16-8)
  Don't gate feeding on the champion's length either: feed_min_champ 12 lost 9-13 on official maps
  (13 v 56 longest; the champion is short at r380 and grows by being fed). Gating on team size vs
  enemies seen (units > max(6, enemies)) halved fatal feed turns on gst84 but rarely fires on gst68. (g17-9, g17-10)
- **The swarm's opening is not better than ours any more.** Playing its opening (value-per-round goals, no
  contest discount, split 2 always) until r120 lost 49-53 to gather-split-trade-84: our contest discount
  and centre pull take pearls from the swarm, and gst84 already out-eats trading-swarm-5 by r100 on official
  maps (20.9 dragons / 56 length v 19.4 / 47). Opening trades lose. (g17-8)

- **"No goal" means hemmed in, not idle.** On 97% of goalless turns the dragon's whole reachable region
  was in view and smaller than the room it needs: the goal search treats every body as a wall, so no
  out-of-view tile is reachable to score (3-12% of dragon-turns, ~70% at length 2-3). Searching past
  ally bodies was neutral (gst75). Goalless stubs preferring open ground over corridors helped a little
  (gst73 53-49). (g16-7, g16-17)

- **Poor-home portal push helps only the gst line.** Pushing dragons through portals early when home food
  is scarce won trauma both seats for gather-split-trade-78/-83 (early pearls 87 -> 133) but was inert in
  late-surge (its frontier scoring already covers it; the push fired 1-2 times) and in body-wall (its
  length >= 4 unknown-portal gate blocks it). (g16-20, g16-26, g17-3, g17-4)

- **Opening fan-out (exclude a frontier tile an ally in view is nearer to, before r80) helps only gst**
  (gather-split-trade-84 55-47); late-surge 27-35, counter-swarm 50-52 with totals down: their own
  frontier scoring already spreads dragons. Discounting instead of excluding left dragons goalless
  (26-36). Most cross-line ports now come out neutral: lines have diverged. (g16-25, g16-27, g17-5, g17-6)

## Growth and roles

- **Late-surge beats the champion-feeding line on worldgen:** a quiet swarm (never trade
  evenly; strike/lunge only at strictly longer enemies; split at 4 until 400) that sends every
  dragon, leaders included, to the sonar-agreed champion from round 380 (die within 3 steps)
  went 53-27 vs gather-split-trade-23 on worldgen in sims and led the league (2180, holdout
  195-45) on worldgen games; it loses to trading swarms on small crowded official maps. The
  surge sweet spot is 380–400 (350 too early). A crowding switch to even trades fixes the
  trading-swarm matchup but hurts vs counter-swarm (late-surge-2). (g6-8, g6-14, g6-15)
- **Late-surge on official maps:** late-surge-1 loses 8-14 to gather-split-trade-27 there (it
  wins worldgen). On big_empty feeders can't path to a champion in unexplored territory (~5
  feeds a match). Fixes: trust an ally's visible length when its whole body is in view (the
  "stub ghost": heirs fed a 2-long stub still reported as 55), feed only a champion ≥ 25%
  longer (late-surge-6, 37-25), and a late walk (from 440, only feeders ≥ 10 long) toward an
  unreachable champion (late-surge-7: 11-11 vs gst27 on official). Regional champions (feed
  the nearest longer ally, then merge) lose everywhere: two hops deliver ~1/4 of the mass.
  (g7-4, g7-5, g7-6)
- **Hunting enemy heads is the costly part of the busy mid game:** gather-split-trade-31 with
  hunt_radius 0 (still accepting even trades that come to it) beat its parent 54-48 and
  late-surge-6 60-42 (parent: 47-55), without losing the small crowded official maps. Late-surge's
  other quiet trait, trading only with strictly longer enemies, lost 44-58 on its own. (g10-2)
- **Most of late-surge's edge is its endgame:** gather-split-trade-27 with late-surge-6's
  endgame (split until 400; everyone incl. leaders feeds the champion from 380 within 3 if
  < 80% of it; trust_visible; champion freeze at 330) went 27-13 vs gst27 on worldgen and
  12-10 on official, and closed most of the gap to late-surge-6 (18-22). (g7-7)
- **The main line's movement layer is transferable and was the giant line's whole early gap:**
  giant-8 = gather-split-trade-34's movement/goal layer (escape levels + TurnOut, dead-end rule,
  contest/share scoring, portal fix) with the giant roles: 77-25 vs giant-7, about even with gst34,
  total length at r50/r100 now equal or ahead. Giant-7's surge at 340 was also wrong once movement
  improved: gst34's timing (surge 380, splits until 400) was needed. (g11-4)
  Confirmed on counter-swarm: counter-swarm-19 (the same wholesale swap, keeping strikes/trade
  rules/guard) closed its growth gap too (official total at r340: 71 → 133), 54-48 vs its parent,
  51-51 vs gst34. Piecewise ports never did this; swap the layer whole. (g12-6)
  And a third time: swarm-hunt-14 (swap + hunting off + no split reserve) 63-39 vs swarm-hunt-13.
  Hunting costs growth even in the line built around it. (g12-12)
  A fourth time: late-surge-15 (gst39's layer + late-surge's quiet trading, trader switch,
  outnumbered mode, endgame) 57-45 vs late-surge-14 and 60-42 vs gather-split-trade-39. (g12-21)
- **Giants don't survive on official maps:** giant-8's round-0 leaders split at 4 until r100, sit at
  2–4 long and die trapped by r150–300, so the line plays like the main line with a different leader
  split cutoff. Stopping leaders splitting (length ≥ 8) loses the opening (6-16). Making the giant
  real needs protection by movement, not fewer splits. (g12-9) Neither worked: movement guards
  (giant-11) and shedding SPLIT 2 instead of halving plus an heir handoff (giant-12) never kept a
  round-0 leader to r499; delaying the opening split is fatal (7-15). Early numbers beat leader
  length; the long dragon comes from the late surge. The giant idea is parked. (g12-15, g12-22)
- **Portal pockets fragment consolidation:** on trauma 8+ champions are believed at once (sonar
  doesn't cross pockets); longest 6 at r380 with 93 total. (g12-9, g11-6)
- **The late-surge endgame transfers to every line** (g8): vs its own parent swarm-hunt-9
  34-28, counter-swarm-14 38-1-23, body-wall-3 36-26, giant-4 35-1-26; each closed most of its
  worldgen gap to gather-split-trade-31 (e.g. counter-swarm 13-27 → 18-22, body-wall 22-40 →
  30-32 overall). The giant line needs the surge earlier (340; 380 lost badly), its helpers
  being spread out. After g8 the top ~8 span four lines within ~20 Elo. (g8-1..4)
- **The split window matters across lines:** workers splitting until ~350 (not 300) helped in
  all three older lines: gather-split-trade-13 (+80 Elo), counter-swarm-12 (its collapse after
  300), swarm-hunt-8 (37-25). late-surge splits until 400. It's the workers' window: leaders'
  (g4-10) and keepers' (g9-2) windows at 350 were washes. (g4-4, g6-9, g7-2, g9-2)
- **League caveat (fixed 21:10):** the scheduler played a pair's worldgen games before its
  official ones, so new variants' early ratings were worldgen-only.

- **A lone giant loses:** never splitting went 1-21 vs strategy-v1 on official maps. A giant
  fed by a helper swarm (giant-1) rated 1802, about even with strategy-v1. Sonar gossip about
  the giant's position reaches few helpers (~10 feeds a match). (g1-3)
- **Split at length 4 beats 5 and 6, still, with modern variants and on worldgen too:** on
  gather-split-trade-5, split at 6 went 5-17 official / 8-32 worldgen, at 5 6-16 / 17-23.
  Trapped deaths fell, but splits halved: more heads eat more pearls. Trapped deaths of
  small workers are the price of the economy, not a leak to plug. (g1-1, g4-5)
- **The unit cap is hit constantly:** ~26% (official) and ~30% (worldgen) of our
  dragon-turns are at UNIT_COUNT = 64, where no one can split. About a third of worldgen
  trapped deaths happen at or near the cap. Reserving escape slots doesn't help (more stubs
  die); having length-2/3 workers at the cap die next to a longer ally in reach (freeing a slot
  for a productive split) went 57-45. The problem near the cap is idle slots. (g5-4)
- **Split window:** with champion feeding from 360, leaders splitting until round 350 beat
  300 (33-29, +5.9 longest on worldgen); 250 was worse. (g4-4)
- **Turn order (disputed):** g4-9 found a real first-mover edge on the official maps for
  swarm-hunt play: with side B's view rotated 180° (exact mirror play, `MirrorAgent` in
  swarm-hunt-5), self-play still goes to side A 9-1-2 (worldgen ~even). g4-3 had argued there
  is none: head-to-heads kill both, and a deciding dragon always
  moves before any enemy moves again. The only id-order effect is a tie at a bed's growth
  round (the lower id eats). Per-map side results come from deterministic play drifting apart
  (absolute direction tie-breaks). Conceding pearls an enemy head reaches first went 35-27.
  (g4-3, g4-9) Where going first wins is still open (pearl ties, lunge timing).
- **Hunting the enemy champion rarely fires:** late workers are 2–4 long (lunge reach 1–3),
  and enemy leaders keep off tiles next to heads (29-33). (g4-1)
- **The champion is almost never killed** (<1 guarded death a match); late losses are
  attrition: too little team left to feed at the feed round. (g3-10)
- **Round-0 identity:** "first turn is round 0" doesn't identify a map dragon if something
  splits in round 0 (a child born then also acts in round 0). (g1-3, g1-5)
- **Children don't know their body beyond the 7×7 window**; heirs of trapped splits
  overestimate their room, so traps chain. Mark own segments seen each turn. (g1-3)

## Measuring

- **League Elo shifted ~100 down at 19:05** when scheduling became neighbours-only past 12
  variants (the top meet the anchors less). Compare Elo only within a snapshot; prefer head to head.

- **strategy-v1's `SafeStep` never tries West when it has no goal** (`(preferred + k) % 4`
  with preferred −1 tries −1, 0, 1, 2). Anything copied from strategy-v1 must fix it. (g1-4)
- **Sonar sharing of pearls/enemies didn't pay** (sonar-pack-1: 152-168 vs strategy-v1 on
  320 worldgen matches); delivery rate was never measured. (g1-2)

- **Judge CPU is far under the limit:** in the judge's sandbox (`unswbc run --sandbox`, after
  `scripts/make-bot.sh`), gather-split-trade-5 and counter-swarm-5 used p50 4.1M, max 6.5M
  points a turn (limit 100M) on schooltime. The 10–50 ms native spikes workers saw were
  machine load. (orchestrator, 2026-09-28)
- **Noise is large:** a one-parameter change swings ±8 wins in 80 matches; the 11 official
  maps alone can't separate variants. Native turn times under machine load spike 10–100×;
  count search work instead. (g1-1, g1-3)

- **Target claims over sonar don't pay, even on a modern base.** 14.8% of pearl/bed goal-turns
  chase a tile an ally also chases; claims (73k heard a match) cut that only to 13.3%, and giving
  way went 46-56 vs gather-split-trade-39 (0.4 weight: 41-61). A duplicate chaser re-targets once
  the pearl is gone; a claim sends it to a worse target on stale news. Sending the champion word
  on only two directions before r330 cost nothing (40-40): early ping slots are free. (g13-2)

- **Portal ambushes don't fire in the league.** Waiting beside busy exits (portal-play-2) produced
  ~0.1 ambush lunges a match: a head fresh out of a portal shows one segment, so its length is
  unknown (12 of 18 in reach); dragons see only ~14 enemy crossings a match in their 7×7 windows;
  escapes through portals never came up. The real opponent crosses far more often. (g14-4)

- **Bodyguards and escorts don't work.** A short ally trading into a lunger that threatens our giant
  fired 0 times, opportunistically (gst86) or with placed escorts beside the champion (gst87): a lunger
  strikes 2-3 tiles in one turn, an escort of length 2-4 reaches 1-3 and must also act first. Escorts
  cost feeding (they crowd the champion). Champion-only lunge avoidance was neutral too (49-53, gst92): the lunge problem has no cheap fix. (g17-2, g17-7, g17-13)

- **A 40-game worldgen win can be seed luck.** gather-split-trade-90 went 29-11 vs its parent on one set
  and 17-23 on an independent one (g17-12). Always confirm a promising worker result on a second set
  (seeds >= 20 apart) before building on it; the league decides.

## Walls

- **Multi-step seals work, but matter little:** laying the head across a pocket exit in one
  2–4-step MOVE seals 6–8 enemies a worldgen match with ~85% confirmed kills (single steps: ~2, rarely
  lethal), worth 54-48; ~7 kills is small next to ~230 trapped deaths a side. Skip enemies already
  doomed; when flooding the enemy's options, treat other bodies as passable. (g12-18)

## Sonar tricks

- **A ping checks a tile out of view:** pings go straight (and through portals) and ECHOES
  reports what they hit, so a clean echo means the line is clear. It cut deaths on blind
  portal crossings from 27% to ~5%. (g5-1)
- **Portals matter on real maps (overturns g5-1's "barely matter"):** in the first server battle
  (M489641, 10 maps) 8.7% of our moves were idle loops (opponent 1.3%), 75% of them orbiting an
  unknown portal: `ChooseGoal` gives explore value to a tile beside a portal whose far end we
  haven't seen, `SafeStep` never takes that step, so the dragon circles forever (one leader did so
  for 240 turns). The fix works (gather-split-trade-33): ping only through the portal the turn
  before (ECHOES totals all pings, so send one), cross if clean, body known, no enemy head in view,
  exploring; explore value for an unknown portal only if we'd cross; loop breaker bans the goal 50
  rounds after 12 heads on ≤ 6 tiles with nothing eaten (exempt: bed waits, feeding). Idle loops
  fell ~3× and it beat gather-split-trade-31 59-43. (g10-8)
  Ported to late-surge (49-43), swarm-hunt (54-46, +28% pearls on official) and counter-swarm
  (idle 3.5–4× down but 65-77 in sim). Local maps understate the orbit's cost: the server
  opponent farms portal stores the league's variants don't. Judge portal changes by server
  results, not only the league. (g10-7, g10-14, g10-15) On Default 11 portal-only rooms hold 138/249 beds and 64 of the 84 pearls left
  at r499; on Trauma 122/142 beds are portal-only. Fix: step through the unknown portal when
  exploring (sonar-check the exit first), plus a loop breaker. (server M489641)

## Real opponents (server)

- **split-v7-style trading swarm** (M489641, 5-5 vs gather-split-trade-27): no sonar, always
  `SPLIT 2`, keeps splitting to r499, starts nearly every head-to-head as an even trade, 3–5-long
  dragons lunge 2–4 steps at our long ones (killed a champion ≥ 14 in 4 of 10 games), sloppy
  (many self/ally deaths, affordable at ~1 split a round), no consolidation (its longest grows
  late by eating, e.g. our champion's corpse). It wiped us out on small crowded maps (Devil,
  Prisoners, Trophy: out-ate us ~4:1, twice our dragons by r150) and beat us on longest on big
  maps by lunging our champion. Our trade rule (only enemies ≥ our length) gives its 2-long
  kamikazes free even trades.
- **A stronger opponent (M491281, we lost 2-8 with gather-split-trade-27):** the same swarm shape
  plus sonar every turn, heavy portal use (1,098 crossings; it farms portal-only stores: 709 v 8
  pearls on Trauma, 241 v 1 on Default), late champion feeding (own shorts die beside a longer
  ally: longest 8 → 55 in 100 rounds), the reverse split, and kamikaze pincers (two 2-longs trap
  our last long dragon). It lunged our champion's reverse-split head the round it appeared
  (Schooltime r450). With a pearl in view we're as efficient as it; we lose by standing where
  there are none (no pearl in view 66–78% of the time on portal maps). Our idle loops: 6.7%, 73%
  portal orbits. Priority: the portal fix. (server M491281) Replays, decoder and renderer: `../syeyoung-tokens.league/replays/`.

## Maps
- **Small official maps are decided by the seat.** In self-play (cs27 v cs27, gst48 v gst48)
  Colosseum, arena and default_small go to B, devil and trophy to A (on arena A falls behind from
  r9, before contact). W-D-L on those maps flips on tiny changes: judge small-map fixes by the
  favoured seat's results and by length traces, not by the win count. (g14-10)

- **Worldgen maps are harder than the official ones** for window-only play: mazes,
  labyrinths, 1-wide warrens and portal lattices. anchor-greedy beats anchor-random 15-7 on
  the official maps but loses 11-29 on worldgen maps. Test on both. (league, 2026-09-28)
- Every official map and every worldgen map is symmetric, so both sides of a map are fair;
  the league plays both.
- **Corridor room flood (g18).** In the gst line, SafeStep's RoomAround flood excluded Exposed tiles, so any enemy head near a corridor made a through-corridor look sealed. Dropping that (gather-split-trade-97) won 12-10 official and 16-10 labyrinth, but was only even on caverns and default worldgen. The same one-liner is neutral on body-wall (51-51; it has separate head gating) and hurts counter-swarm on labyrinth (15-25). It is line-specific.
- **Dead-end harvest (g18).** Loosening room_min/room_margin globally loses (gst95 16-24 labyrinth): short dragons die trapped as often as they gain. Pricing a dead end by expected loss, and letting 2-3-longs in only when the pocket grows them to 4 so they can split out (gather-split-trade-96), became league #1 (2214 vs 2196) and was submitted as v7. (Correction: 10729 is OUR v6's server id; the column dive on Devil was our own bot, in a win.)
- **Portal-only stores (g18).** gst90 lost Trauma in both seats to trading-swarm-2 by idling in the strip (VISUAL #37). gst83's poor-home portal push ports cleanly onto gst90 (gather-split-trade-98): it wins Trauma in both seats and is even elsewhere.

- **Reading server replays (g19).** `botA`/`botB` hold OUR submission id; the opponent's is blank. Identify our team by 0xA7 sonar words (`M491281/analysis/analyze.py`, `ours`), never by the name. The real series vs the current opponents was 7-18: out-eaten 2-10x from ~r20 as the opponent takes the contested centre (VISUAL #38).
- **Corridor farming (user idea, g20).** A farmer role (short dragons claim corridor bed farms by sonar and stay small; gather-split-trade-109) engages heavily: 240-470 claims, 6-11k farmer-turns and 600-1050 farmer-eaten pearls per game vs trading-swarm-8. It does not grow the team faster (r50/r100 counts identical to gst107) and is slightly worse vs ts8 (11-10 vs 13-8). corridor-farm-1 (spawn-time farm detection + assignment) is even with gst107 and in the league. The Devil opening gap (r50: 9 v 20 dragons vs ts8) is untouched by farming. Open questions for strategists: dead-end-only farms, and whether ts8's hunters catch committed farmers.
- **Sonar delivery, measured (g20-10).** From replay sonarPing hit kinds: 12-49% of our 0xA7 words hit an ally, depending on the map (Trauma ~15%, QoS ~24%, Default ~24%, Devil ~38%). Each report reaches 5-9 allies, or 5-15 with relaying. Relaying fresher ally reports helps open maps (Default 24->40%) and slightly hurts corridor maps. There is enough delivery to carry shared state (e.g. bed timers); the relay alone doesn't win (25-27 vs gst107).
- **Where the opponent's pearls come from (strategist-h, all 25 server games).** Classifying every eaten pearl by tile (<= 2 open sides = corridor), the opponent's eating edge over us is proportional: 1.22x in corridors, 1.30x in open ground. Corridors are not an untapped edge, which explains why corridor farming engaged heavily without growing the team. The Devil deficit is open-ground spread from r0 (server: 6 v 16 dragons at r50, 7 v 31 at r100).
- **Merging tagged wins (g20-g22, strategist-i).** Merges fail when both changes act in the same function and phase (gst99, gst108, and gst119: early split gate + fan-out decay, 22-40 vs gst117). Disjoint code is necessary but not sufficient: gst111 + gst101's portal push (gst118) went 42-44% vs both parents, because a portal crossing changes where a dragon later consolidates. The one clean compounding merge so far is gst111 + cs40's SafeStep crowding tie-break (gather-split-trade-120). Beating trading-swarm-8 alone is not enough; several variants beat it and still lost to their own parent.
- **sim vs judge sandbox (g24).** `sim` runs each map with a fixed pearl seed; the sandbox draws a seed per match. gst115 vs gst96 went 35-27 in sim but 14-18 over 32 sandbox games. Before a submission, confirm in the sandbox with 20+ games; a sim edge of ~55% can vanish.
- **Don't gate splits to cut trapped deaths (g24-4).** Trapped-L2-3 deaths scale with the number of splits; the per-split rate barely moves. Gating short splits on ally congestion loses the attrition race, and the stronger the gate the worse (8-32 on worldgen vs gst115). Measure split gates on worldgen: official maps hide the damage.
- **Population-pressure feeding doesn't pay (g24-3).** Feeding short workers to the champion near the unit cap (from r250) only converts "give up slot" deaths into "feed champion" deaths. It loses length on the walk and never raises longest. Trapped-L2-3 deaths happen below the cap, where no cap trigger is live. Any walk-to-champion rule must still let a worker give up its slot, or long dragons trap at the cap.
- **Simpler dead-end pricing wins (g25-1).** Going back to a flat trap_cost 3.0 all game and dropping gst96's expected-loss model (trap_loss) beat gst115 61-41 over three map sets, with dead-end entries halved. The cheap opening trap cost was a mild net harm. gst115's opening nearest-pearl preference is live (turning it off: 27-35).
- **worldgen seed 113012 produces an invalid map** (sim aborts: "dragon 0 segment 8 is not adjacent to the one before it"). Skip it if a batch includes it.
- **sim gains don't reach the sandbox (g25).** Four variants that beat gst96 (v7) by 55-60% in `sim` (gst115, 127, 128, 131) all came out 21-24 of 44 vs gst96 in the judge sandbox (11 official maps, seeds 11 and 12, both seats). `sim` and the league use a fixed pearl seed per map, so they overfit it. Judge submission candidates and important changes with multi-seed sandbox games (`unswbc run --sandbox --seed N`); sandbox games take only 2-15 s each.
- **sim now reproduces the judge (g25-5, 2026-09-29 21:10).** The judge (unswbc 1.2.2) seeds pearls with `std::mt19937_64(--seed)`; our vendored 1.0.0 engine used `mt19937(0x5eed5eed)`. So every sim and league result before this was measured on a pearl schedule the judge never produces, and many "wins" were overfitted to it. `tools/judge_engine.cpp` fixes sim/match/check with no shared/ABI edits: 44/44 official-map games identical to the sandbox (orchestrator re-verified 22/22 at seed 11). Use `sim --seed 1,2,...` (several seeds). The league now plays each match on its own seed; its database was restarted from scratch (old one kept as league.db.pre-judge-engine-20260929). `scripts/sim-vs-judge.sh` re-checks agreement after unswbc updates. Under the faithful engine, gst115 vs gst96 (v7) is 114-106 over seeds 1-10: a coin flip.
- **Faithful baseline facts (g26, faithful engine).** Every gst variant since gst90 is even with gst96 (109-117 of 220, 10 seeds). gst96 already splits at 4 from the start: ~20 dragons at r100 on official maps, 61 on big_empty (near the 64 cap). Split rules don't matter before r100. The server's early gap is eating rate and position.
- **Hot beds (arena territory team, round 1).** Beds whose countdown reads <= 1 on two rounds close together refill every round. They make most of the food on devil (about 18 of 18.6 pearls a round), stronghold and trauma (about 12 of 14). A bed's timer fires even when the tile is covered, so a bed under a body loses that pearl. This is the concrete mechanism behind the user's "persistent farming by small snakes" idea: a dragon cycling a short loop through hot beds.
- **Old "wins" re-tested on the faithful engine (strategist-r r9, g26).** These are coin flips vs gst96 (50-52% over 220 games): the opening nearest-pearl preference (gst115), centre pull (gst107), reactive Consolidate (gst111) and the crowding tie-break (counter-swarm-40). Only the dead-end pricing change in gst131 is a real edge (57%; submitted as v8). trading-swarm-8 loses to almost everything, so it doesn't discriminate; use trading-swarm-9 or later. Server losses are often total wipes (0 dragons left in 2/10 and 5/10 games of M580806/M588431).
- **The server shuffles spawns (g26-6).** In 27 of 45 server games the DRAGON lines' team codes are swapped relative to maps/official (seat A starts where the file puts B). In 3 Prisoners Dilemma games the server map had 10 dragons, not 6. Walls and portals always match. Always test both seats (sim does), and don't trust seat-specific conclusions from our map files. `variants/trading-swarm-10/clone-eval/gamemaps.py` rebuilds each server game's exact map from its replay.
- **The real opponent wins on dragon count mid-game, not pearl finding (g26-6).** On Schooltime it had 1,626 dragon-turns in r50-99 against the clone's 707, while the clone ate more per turn. Its early dragons spread along the whole edge; clusters die. trading-swarm-10 (tagged) is the hardest clone so far but still too weak (gst96 beats it 35-23 locally; the real one beat gst96 31-14).
- **Pearl supply feeds back (g26-9).** A bed refills only after its pearl is eaten, so the side that eats new spawns sooner gets more pearls overall. On Schooltime the server spawned 165/140/153/173 pearls per 50 rounds against 162/123/119/145 when our clone played the same game, which accounts for the real opponent's larger dragon count (31 vs 21 at r200). The real opponent is not greedier per turn (it eats 0.082 per turn vs the clone's 0.101); it covers the map's beds faster. When boxed in at length 4+, it splits (59% of those splits survive), and it steps into an adjacent enemy head only ~10% of the time.
- **Clone agreement is noisy (g26-9).** The same clone logic with 4 random seeds scores 20, 24, 18, 19 of 45 server results (±2.3). Judge clone changes with `variants/trading-swarm-13/clone-eval/multi.sh` (4 seeds). Worldgen seed 26900's batch also contains an invalid map.
- **Seeds overfit too (arena, 2026-09-30).** mass-swarm-2/-3, tuned on seeds 1-5, beat v8 53-54% on those seeds and went 288-312 (48%) on fresh seeds 21-40. Judge a candidate on seeds it was not tuned on (the arena now scores on held-out seeds).
- **Noise floor of a 150-game test (arena free, round 5).** The same bot played against itself over 150 games (15 maps, 5 seeds, both seats) came out 66-6-78. Seat and tie-break effects alone move a 150-game result by about 12 wins, so smaller differences are noise.
- **Late economy on big maps (arena free, round 5).** On big_empty both sides sit at the 64-dragon cap from mid-game. After r380, v8's total length rises to ~630 (longest 56-68) while a feed-by-dying team's total falls. Consolidating by resignation destroys length that eating at the cap would keep.
- **Consolidation is limited by the champion's eating rate (arena swarm, round 5).** A champion eats at most one pearl a round, and feeders dying in a cluster at its head leave remains it can't reach in time. On stronghold, 68 feed deaths in r300-440 cut team length from 104 to 61 while the champion ended at ~60. "Feeding wastes half of each body" matters little, because swarm dragons average ~2.4 long. Splitting long feeders into 3-long pieces was neutral. Stopping 2-3-long dragons from feeding collapses results (54-95). Candidate fixes: feeders die in a line along the champion's path, or two candidate champions.

## Turn time: `check`'s slowest turn is a load artefact (arena round 6, hunter team)
`check` reports only the single slowest turn, and on a loaded machine (load average 170-260) that turn is an OS pause of 20-55 ms, not bot work. Measure CPU with `build/tools/match <a> <b> <map> <seed>`, which prints per-turn `turn_us`, and compare the 99th percentile against a reference bot in the same game. arena-hunter-6: median 2.6-11.4 us, p99 8-70 us; v8: median 14-48 us, p99 36-219 us. The judge limit (100M points, about 1.1 ms by the 0.1 ms ≈ 9M rule) is far above either.
The real CPU price comes from the judge sandbox: `unswbc run <map> <botA> <botB> --sandbox --seed S` prints "team X points per turn: p50 .. p99 .. max ..". arena-search-5 on big_empty (its heaviest map): p50 15.4M, p99 30.4M, max 45.9M, under the 100M limit; v8 in the same game: max 7.2M. Wall-clock p99 under load (12.7 ms vs 0.28 ms) overstated the gap; the 0.1 ms ≈ 9M rule does not hold under load.

## Late hunting of the enemy champion doesn't pay (arena round 6, hunter team)
v8's champion (40-60 long) stays in its own ground and almost never enters our dragons' 7×7 view: after r350 the longest enemy any of our dragons saw was 7-15. Every way of finding it measured neutral or worse against v8 (tuning seeds 1-10, 300 games; arena-hunter-6 159-141):
- sharing enemy sightings over sonar: 157-143;
- scouts to the last sighting: 145-154;
- a strike group sent to an estimate of the enemy's home from early head sightings: 142-158, and it never found a 25+ enemy.
The early-sighting estimate marks where the two sides met, not where the enemy champion lives. Dragons sent looking cost our own consolidation more than they gain.
Territory, same round: the team splits into up to 3 groups with separate champions that never hear each other's sonar, and the champion ends with only ~32% of total length (swarm confirmed, same for v8).
Late growth at the cap (stop routine splits at the dragon cap, feed only late and only near the champion) lost in every form on arena-giant-6's core. On the maps where the team reaches the cap, it scored 11-12 wins of 30 vs v8, against 17 for the base. Fewer, longer dragons eat less than a capped swarm, and late local feeding leaves the champion shorter (giant team, round 6).
Why the groups stay apart (swarm, round 6): a sonar ping stops at the first dragon it hits, so groups separated by kelp or by other dragons never hear each other. Distinct champions believed at r450: 2-5 on default, slithery_fight and stronghold, and only 1 on big_empty. On big_empty the champion is agreed on but 28-30 other dragons still hold ~170-180 length at r450: there the walk is too long. Delivery changes (walk to any champion segment, start by distance) raised the champion's share from 32% to 35% without changing W-D-L (148-151 vs v8 held out). Parking starves the champion (30.9 long against 41.3 without it).

## Consolidation mechanisms have not paid in any line (arena round 6-7, territory, swarm, search)
Every attempt to raise the champion's share by delivery has been neutral or negative on held-out seeds. Feeder deaths are only 5-11 a match in every line (search's included), and the champion grows mostly by eating for itself. Tried:
- pantry, and den/parking (the champion starves: 30.9 v 41.3 long);
- one-champion agreement over sonar (believed champions 2.75 → 2.06, but 133-165 vs its own base, arena-search-6);
- a map-centre gathering tile (no change in distinct champions);
- live position relay and arrival timing (share 32% → 35%, W-D-L unchanged);
- the pre-fight corpse ping (73-73).
The gap between lines is economy (growth before r300), not consolidation: arena-search-6 beats mass-swarm-7 200-99 held out.
Build note: cmake can miss an edit made within a second of the previous build. Check A/B builds with `cmp`.
Group merge (arena-search-9): a champion that hears a longer one becomes its feeder, and reports fade more slowly (0.15 a round, kept 80 rounds). Distinct champion ids at r450 fell from 2.32 to 1.82, but results didn't change: 72-5-73 vs arena-search-7 on held-out seeds. The longest dragon comes from the largest group, so a second group rarely decides a game. Fewer champions is not the lever.

## Why v9 (arena-search-7) lost its first server battle 0-5 (strategist w, M665147)
Context: the user chose this matchup, v9 against the #1 bot on the leaderboard. Read it as how the top bot plays, not as a verdict on v9 against typical opponents.
- **Corridor economy.** The opponent farms dead-end "hot beds" (a pearl respawns within 3 rounds) with a tip split: it eats its way up a dead end, and at length 4-5 at the tip splits so the rear half walks out while the trapped length-2 front dies on its own body. Hot-bed pearls, opponent to us: Autarky 150 to 6, Slithery 225 to 29. arena-search-7's room check (fewer than min(length+1, 14) reachable tiles is penalised) keeps it out of dead ends. gst131 farms Slithery's hot beds locally (565) but barely Autarky's (6).
- **Small-map openings.** Dragons at r50: Prisoners Dilemma 1 v 7, Queen of Spades 2 v 8, Autarky 13 v 21.
- **Local measurement misled us.** The ts13 clone has no tip split and gets stuck (arena-search-7 beats it 47-0-3 on the server maps and seeds). `maps/official` has stale dragon placement (Dilemma 6 dragons against the server's 10; team order swapped on 4 of 5 maps), so use `maps/server/`. Longest-dragon wins hide total-length losses. The server seems to draw opponents from a pool.
Cloning the #1 bot (hunter team, opp-clone-1..3; arena/round-7/hunter.md): no clone beats v9 over the 200 games on the 10 server maps (best: v9 112-88), while the real #1 beat v9 5-0.
- Reproduced: split 2 at length 4 up to the 64 cap, no lunges, trapped-dragon self-deaths, hot-bed farming, and 60-64 dragons by r100 on Slithery.
- Not reproduced: 63 and 35 dragons at r100 on Schooltime and Autarky, and growing a champion out of a 64-dragon swarm. A hunter-style opening breaks gst's late consolidation.

## v9's endgame edge and our resign waste (strategist x, seed 22, v8 vs v9)
- About 45% of official games end in eliminations before the endgame. They split about 50/50 and are largely decided by seat; no systematic v9 flaw shows there. So 90% vs v9 isn't plausible (about 65-75% at best).
- v9 wins round-limit games by consolidating from less material: its longest roughly doubles after r420 by feeding.
- gst resigns 112-395 dragons after r300 and destroys 265-1005 length; v9 resigns 7-44. The ratio of longest to total at r350 is about 0.3-0.5 for v8 and 0.6-1.2 for v9.
- v9 trusts 0xA5 champion words from anyone (search.cpp L141-152, L337-340), so a spoofed champion could hijack its feeding. This is a flaw only in our own bot and useless on the server; not built.
Autarky and Dilemma vs v9 (search team, seeds 21-25, 10 games per map): gst-search-3 went 0-10 and 5-5; with the crowd gate off, 0-10 and 1-9 (so the gate isn't the cause); gst-giant-2 went 1-9 and 10-0; gst-merge-2 went 7-3 and 10-0. v9 out-splits gst there (76+ splits a game against 38). gst-merge-2's edge probably comes from hot-bed farming (Autarky has 8 beds with gap 1 and 4 with gap 5); not isolated yet.
The late lookahead lever (search-7's per-dragon lookahead steering gst's plain moves from r250) gained on all three gst bases it was added to: gst-search-1 → gst-search-3 went 90-60 vs its base; gst-merge-2 → gst-search-4 (live v11) went 83-63 held out; gst-giant-2 → gst-search-5 went 80-70 held out and 37-23 on server maps. The gst-giant-2 and gst-merge-2 bases split the server maps between them: gst-giant-2's wins Dilemma, Default and Devil; gst-merge-2's wins Portals, Trauma and Autarky.

## v10's 80 server games (strategist y, 42-38)
- **How the 38 losses happened:** 18 small-map eliminations (Trophy 5, Autarky 4, Devil 3, Queen of Spades 3, Dilemma 2, Default 1); 14 behind on total at r499; 6 behind on longest while ahead on total.
- **Small maps are decided by pearls eaten before r60:** 26 v 51 in our losses, 41 v 22 in wins. On the server Trophy seed 13553692030115652581, v10 and v11 eat nothing for 30 rounds (reproduces locally on maps/server/trophy.map).
- **Strong teams open with many 2-long dragons:** splitting a 2 off every turn, or an 18-long dragon on refilling beds leaving a 2 behind every 2 rounds. We halve the starter once.
- **Late game:** our longest dragon has a pearl in view on about 40 of the last 50 turns but eats about 8.
- **Routed:** P1 (early food-taking) → giant, P2 (early 2-splits) → search, P3 (late champion harvest) → swarm, all on gst-search-6.
- P2 (shed-2 opening, strategist y) failed on gst-search-6: Autarky 5-15 against gst-search-6 in every form; adds about 1 dragon at r50, but the 2-long dragons get picked off, and it costs a Dilemma seat vs v9. It's in gst-search-7 (79c70e0), switched off. (search team)
Round 7 morning (search and swarm):
- **gst-search-8** = gst-search-6 with the judge's contested-pearl review stopped at gst's feed round. It fixes Slithery (15-5); on hot-bed maps that review had been pulling feeders and the champion off consolidation.
- **Devil** is decided by seat, not bot: gst-search-8 wins 9/10 as B and loses 9/10 as A vs gst-giant-2.
- **Default:** our small-team feeding schedule (feed from 320) dumps feeders early. Shifting it to 380/330 for teams under 48 dragons won default 24-16 (gst-search-9), but gave back Slithery.
- **Consolidation on the gst-search line** (swarm): every mechanism has been noise. Tried: reserved ground, delivery, map-centre gathering, local feeding, the pretender gate, champion harvest and tail shed. The multi-giant endings (2-2.5 dragons of 20+ at r499) come from groups that never hear each other. The real gains came from movement and fight decisions: the trade value, the rollout judge, the late lookahead and the contest-review stop.

## Queens under 1.2.3 (strategist z2, 20 new-rules server games plus sandbox)
- **Recognising queens:** window segments carry id and team, so an enemy segment with id 0/1 is their queen. Each dragon knows its own id at spawn.
- **Survival:** queens almost always die today. In 20 server games every queen died except one; median death about r50; none grew past 13. Our v11's queen resigned itself to death in 6 of 20. In 4 of our round-limit losses the enemy queen was already dead, so a living queen would have won them.
- **Doomed starts:** on dilemma, autarky and slithery_fight both queens are doomed at spawn (head facing a 1-wide dead end, splits keep the trapped head as the queen), so 7 server maps are queen maps. Detect it from observation: queen room at spawn < its length.
- **Sandbox prototypes:** a queen guard won 10/10 games decided by the queen vs gst-search-8 (27-29 overall: it costs early economy). Kill and chase rules on the enemy queen barely fired, since its head is in view only about 38 dragon-turns a game. Ordinary gst trades already kill most queens.
- **Free sprint:** dragons of length 4 or less are unchanged. Long dragons move ceil(L/4) free steps and can now outrun small queens. Enemy trade reach on our queen is ceil(L/4)+L-2.
