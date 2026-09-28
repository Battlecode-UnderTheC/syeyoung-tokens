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

## Sonar tricks

- **A ping checks a tile out of view:** pings go straight (and through portals) and ECHOES
  reports what they hit, so a clean echo means the line is clear. It cut deaths on blind
  portal crossings from 27% to ~5%. (g5-1)
- **Portals barely matter** to the current strategies: ~0.25% of moves cross one. (g5-1)

## Maps

- **Worldgen maps are harder than the official ones** for window-only play: mazes,
  labyrinths, 1-wide warrens and portal lattices. anchor-greedy beats anchor-random 15-7 on
  the official maps but loses 11-29 on worldgen maps. Test on both. (league, 2026-09-28)
- Every official map and every worldgen map is symmetric, so both sides of a map are fair;
  the league plays both.
