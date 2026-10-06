# ring-sprint-sighting

Parent: ring-sprint-2 (code: variants/ring-sprint-founder). Task g44-8, strategist g44 proposal #4.

## Idea
arena-r18 T3 (D1): in the live race the queen is lunged by long enemies from outside her own 7x7
view (killer out of view in 109 of 195). Workers often see those dragons first. Relay the sighting
to her by sonar, and let her race beams treat it as a threat for 3 rounds.

## What changed from the parents, and why
- `sighting.cpp` (new):
  - from r427 (kRaceFrom - 3) a worker that sees an enemy head of length 6+ within 8 (+2 per
    round of staleness of her report) of our queen's last known head sends a sighting word
    (magic 0xAB: team, enemy id, tile, length, round). Aim: directions whose straight ray meets
    a queen segment in view; else ones passing her last known head or a body tile of hers seen in
    the last 3 rounds; else every non-tail direction, so allies hear it. A worker that heard a
    sighting at most 1 round old (and sees no long enemy itself) relays it the same way.
  - the queen keeps each heard sighting for 3 rounds. From kRaceFrom, if it is within 8 of her
    head and she doesn't see that dragon this round, she adds its zone to QueenRisk: flood from
    the sighted tile through tiles not seen occupied, risk 9 - max(1, dist - age*freesteps) within
    its reach, capped at 10 tiles. QueenRisk feeds QueenMove's harvest beam (10000x) and Ring4Exit.
- `ring.cpp`: QueenRisk takes max with RelayedRisk.
- `strategy.cpp`: HearSightings in Decide, SendSighting last in Turn; `#include <cstring>`
  (std::strncmp; the base only compiled through a transitive include, it fails on this toolchain).
- Values: 6+ = a dragon whose lunge reach (L+3)/4+L-2 >= 6 exceeds her 3-tile view; 8 =
  QueenRisk's reach cap; 3 rounds = the proposal; drift 2/round = her non-sprinting step speed.

## Local
- check maps/official: all passed.
- Self-play smoke (server maps, seed 7, -DQSIGHT_DBG): ~86 own sends, ~56 relays, 6 hears by
  the queen, 2 activations in 34 games. Delivery is the bottleneck: most rays end in kelp
  (debug ray codes: K on most directions on maze/portals/trauma/slithery).
- Fresh mapgen maps (22, seed 4408, pearl seeds 11,12) vs ring-sprint-founder: 42-4-42, every
  mirror pair identical (no game changed).

## Iteration 1 bug (found in arena duel replays)
The first build (eph-71d2df43) duelled 34-0-34 / 10-0-10 with all 44 mirror pairs IDENTICAL,
although replays showed the queen receiving 21 sighting pings in 6 of 54 games (e.g. duel-afa2c011
games 22, 41: enemy 6-7 tiles away, out of her view). Cause: the danger flood started at the
sighted tile, on ground the queen had never seen, so BoardMemory::Step returned kUnknown and the
zone never reached her (debug: 45 risk tiles, 0 within 2 of her head). Fix: unknown edges are
assumed open in that flood (159 tiles, 6 near her head).

## Arena duels (fixed build eph-fbef59df vs ring-sprint-2, random:2 seeds)
- server 34-0-34 (only 1 of 34 mirror pairs diverged: schooltime, at r431 a worker, via the
  sonar words the sighting pings replace); official 9-0-10 (1 of 10 pairs diverged, 1 game lost).
- duel-f30d0600 games 4/5 (stronghold): at r439-440 a worker relayed enemy 165 (len 6, 6-7 tiles,
  out of her view); the queen moved SS instead of the base's S, and that seat's queen ended 73
  instead of 63.
- The targeted situation is rare in our-bot duels: the change alters ~1 game in 30.

## Arena verdict: ring-sprint-7 (draft, parent ring-sprint-2)
- vs parent 56-0-57 (49.6%, loses within noise); server 34-35, custom 17-17, official 5-5.
  Decided by: elimination 29-29, longer queen 17-18, longest 10-10.
- vs ring-contest-4 62-50, ring-sched-17 68-46, ring-rush-1 72-40. CPU p99 25.8M, max 41.9M.
- eval P(win) vs parent (seat-corrected mean): r100 0.499, r200 0.498, r300 0.501.
- 53 of 55 mirror pairs vs the parent are move-for-move identical; the 2 that diverge split 1-1
  like their parent pair, so the single-game difference is the one unpaired game.
  - portals 194340 v 194329: at r476 a relayed sighting (enemy 632, len 6, 2 rounds old, out of
    her view) made the queen step W instead of the parent's NN; she ended 20 instead of 29 (still
    won 20-19). Over-caution, the enemy never came.
  - maze 194383 v 194372: first divergence is a worker (r443), from the sonar words the sighting
    pings replaced; the queen got 4 relays (enemy 897 at 2-4 tiles).
- Targeted weak spot NOT fixed: our queen died to a late lunge (r430+) in 10 games v the
  parent's 10-11. In all 10 she received no sighting in her last 4 rounds; in 9 none was sent.
  In 6 no ally had the killer in view, in 2 she had no allies left; where an ally did see it
  (194601: a 2-long child saw 7-8 segments of a 19-long killer 5-7 tiles from her), it didn't
  know where the queen was (no queen report within 5 rounds), so it never sent.
- Conclusion: the relay's reach is bounded by who sees the killer (usually nobody near her late)
  and by sonar delivery (kelp stops most rays). Not worth stacking.
