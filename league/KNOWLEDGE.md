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
- **The cap trap is set before the cap is reached:** of ~2,100 length ≥ 4 resignations at the
  cap (40 worldgen matches), almost none had a shrinking-room trend: rooms read open for many
  turns then 1–3 the next (bodies close gaps within a round), or the dragon entered the pocket
  while it could still split out and other splits filled the cap. Avoiding crowds (g6-7) and
  reacting at the cap (g6-13) both did nothing; forecasting the cap before trusting a split
  escape didn't either (g6-16): long dragons walk in on steps that looked open. Caution on the
  long dragon's side is a dead end; try the supply side (fewer idle slots). (g6-13, g6-16)
- **Partial sightings under-report the champion (likely in every champion-feeding line):** an
  ally whose body runs past the 7×7 window is reported at its visible length, stamped fresh, so
  it beats the champion's own exact report and mid-size dragons elect themselves. At r460 on
  trauma only 0–3 of ~10 dragons knew the real champion. Fix: flag partial reports as lower
  bounds that never lower a known length (swarm-hunt-13; neutral in sims, but the mechanism is
  real). Isolated champions in portal pockets still don't get fed. (g11-6)
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

- **Worldgen maps are harder than the official ones** for window-only play: mazes,
  labyrinths, 1-wide warrens and portal lattices. anchor-greedy beats anchor-random 15-7 on
  the official maps but loses 11-29 on worldgen maps. Test on both. (league, 2026-09-28)
- Every official map and every worldgen map is symmetric, so both sides of a map are fair;
  the league plays both.
