# ring-raid-1 (g43-1, line ring-raid)

Base: ring-contest-1 (code: ring-contest-m1). Operator: mutate. One strategy: raid the enemy's
pearl beds through the map's symmetry.

## Idea
A bed and its mirror share one countdown, and due rounds don't depend on play (KNOWLEDGE.md
'Pearl timing'). Once the symmetry is committed, every bed a dragon has read gives its mirror's
exact tick. server-r16: we eat 8.5 enemy-half pearls a game to their 53.3, and 22 enemy-half
pearls a game lie idle 15+ rounds in r0-100.

## Change
1. **Early commit (from ring-symearly-1).** sym.hpp is symearly's: a candidate is used once it is
   the only one left, dragon 0 checks the round-0 layout, and every dragon shares its surviving
   candidates in champion (bits 1-3) and claim words (bits 1-3). Ring words had no free bits (qgate
   took 26-35, the call 36-55), so ring.cpp `QGateSymField` shares qgate's field: the gate reads any
   sighting before r300 only as "seen, stale", so such a sighting is sent as the candidate mask
   (0-7), a sighting from r300 as its round, and with none the valid bit stays clear and bits 26-28
   hold the mask. Without the ring-word share the starters committed at mean r48-56 (server, sim);
   with it r26 (server) and r11 (fresh mapgen). Beds are still mirrored into memory only on the
   dragon's own evidence, as in symearly (its 0-8 on dilemma/autarky).
2. **Raid targets (strategy.cpp ChooseGoal, RAID_MODE).** A mirror bed never seen here, of a bed
   this dragon read. Only a surplus worker takes part: not the queen, not the champion, and only
   when the scheduler matched it no home food within kSchedHarvest rounds (or it is already
   raiding), so home harvesters stay home. Skipped:
   - a pair whose beds lie within 2*kRaidReach of each other (a contested centre cluster, left to
     the contest logic);
   - a bed within kRaidReach of an enemy head seen within kRaidMemory rounds, or one detour tile
     off the route past it (ring-mirraid-1's safety);
   - a bed whose tick is more than kRaidSlack rounds after our arrival (arrive as it comes due,
     no waiting on their side);
   - a bed another dragon claimed (claim word) within hot_claim_memory rounds.
   A raid bed is worth a bed (a pearl unseen for -due rounds once its tick has passed), times
   kContestDeny (x2, as contested food: one pearl for us and one they don't eat). It takes no crowd
   or share discount. A bed with no known path yet scores the frontier tiles on the way to it.
3. **Commitment.** The chosen raid is claimed over sonar and held (kRaidKeep 1.25 on near-ties,
   and kept as the goal while still a valid target, except for food within view, which is eaten
   on the way). Measured cause: without the hold, 95% of raids were dropped within a step or two
   for a goal scored a little higher that turn.
4. **Window: r < 100 (kRaidUntil).** The idle enemy-half pearls are an r0-100 observation. With
   raids all game (v1), raiders died more often than they ate: fresh mapgen 1128 raid deaths v
   561 raid eats; with r<100 it was 104 deaths v 154 eats.

Debug: RAID_TRACE (counters, sim only), RAID_DBG (per-raid log). Both are off in the bot.

## Local numbers (sim; vs ring-contest-m1)
- Server maps, seed 1 (34 games): v1 (all game) 19-15; v2 (r<100) 24-10; mode 0 (raids off,
  same sym code) 17-17. The spread is noise: with raids off the result already moves +-3.
- Fresh mapgen (`--family all --seed 4306101`, 44 maps, seeds 1,2, 176 games): v1 96-4-76, v2
  92-4-80, raids off 95-4-77. No difference. (The sim prints no per-family split for 2 seeds.)
- Behaviour counters, fresh maps, v2: 3309 raid commits, 154 raid eats, 104 raid deaths over 176
  games. Eats in the enemy zone before r100: 5829 v 5684 with raids off (+2.5%). All eats: 118.8k
  v 118.8k.
- Why so few raids before r100 (fresh, surplus-worker turns): of the mirror beds known, 21% are
  centre pairs, 16% unsafe, 21% have no path yet, 27% come due too late; 11% are valid. Valid
  ones still lose the per-round score to nearer food most turns.

## Arena duels (vs ring-contest-1, random:2)
- v1 (eph-577f584d): server 30-38 (duel-99bdc11e), official 9-11 (duel-5bdc2298).
- v2 (eph-83325e99): server 35-33 (duel-37cae93d; elim 9-14, queen 22-11, longest 4-8; <=2
  starters 14-14, 3+ 21-19), official 8-12 (duel-9a71dfe8). CPU max 35.8M / 37.2M.
- v2 server per map: around_unsw 4-0, portals 4-0, dilemma 0-4, autarky 1-3, schooltime 1-3,
  slithery 1-3, tower_defense 1-3, queen_of_spades 1-3, others even or 3-1.
- Enemy-half eating from the decoded duel replays (per game, our side v theirs, zones by BFS from
  each team's starters): v2 r0-100 13.8 v 16.4, whole game 187.6 v 182.0; own-half eating 576.8
  v 574.8 (not ceded). v1: r0-100 14.1 v 14.9, whole game 179.6 v 170.7, own half 559.3 v 582.7
  (v1 did cede home food).

## Verdict
The mechanism works (raiders commit, reach the mirror beds and eat some, the safety keeps
raid deaths below raid eats before r100), but it doesn't move enemy-half eating in the opening:
there are too few valid raids before r100 against our own bot. W-L is within noise.
