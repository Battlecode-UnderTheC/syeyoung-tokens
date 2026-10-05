# ring-qgate-1 (g42-4): gate the late queen growth on the enemy queen not being known gone

Base: ring-sched-17 (code ring-sched-m3). Arena draft: ring-sched-30.

## Idea
arena-r16 C2 (116 losses) and server-r16 cause #1 (9 losses at P .82-.99): our queen leaves her
ring at r430 (Ring4Exit) to grow for the queen race, although the enemy queen died long before,
and is lunged at r443-477. A dead enemy queen means a parked live queen already wins.

## Change (one behaviour, three tries)
- `EnemyQueenRace()` (strategy.hpp) gates the r430 `Ring4Exit` and the r450+ `Ring4Supply`
  (feed14 worker sacrifices next to the queen). Everything else is unchanged.
- Signal: the last round the enemy queen (id 0/1 of the other team) was seen, from
  - our own view (existing `enemy_queen_seen_`),
  - the champion-word relay (existing; only sightings within 30 rounds),
  - NEW: the ring's 0xBA sonar word (sent by the ring queen and the workers around her ring) now
    carries the last sighting round in bits 26-35 (`enemy_queen_heard_`). Why: in sim the ring
    queen heard only 1-15 champion words in 430 rounds, so she never learned what workers saw.
- Death: the enemy queen seen beside our head last turn (plain step) and wholly gone now
  (mirrors the existing ally obituary rule).
- Final rule (try 3): block when her last sighting is before r300 or she was seen to die.
  Never seen at all = exit as before. r300 is the arena-r16 separator (seen in r300-429 in 65/66
  races we won, 6/271 C2 losses). The server-r16 report suggested 60-100 rounds of freshness; that is
  stricter than r300 at r430 and try 1/2 already lost races to unseen live queens, so I kept r300.

## Tries (arena duels, server suite random:2, 68 games vs ring-sched-17)
| try | rule | W-L | longer-queen |
|---|---|---|---|
| 1 | require a sighting since r300 (queen's own knowledge) | 29-39 | 10-23 |
| 2 | + ring-word relay | 29-39 | 13-23 |
| 3 | never seen = exit as before | 36-32 | 16-13 |
Try 1: official random:2 9-11 (longer queen 1-3). The sim showed why: at r430 the queen had heard
no sighting in 26/39 games, and on maze/around_unsw the parked enemy queen is never seen at all
(0-4 on both maps).

vs cs-queen-split-tail-risk-v1-7 (server, seeds 11,12): try 3 44-24, base ring-sched-17 47-21 on the
same seeds; only 3 games differ, all queen races that try 3 lost because the gate blocked (last
sighting before r300; the enemy queen was alive, parked far away, and grew late).

Local: fresh mapgen (seed 4242, 44 maps x 2 seeds) vs ring-sched-m3: try 1 88-88; try 2 90-86
(every family even, rooms 10-6). Gate at r430 (try 1, sim): blocked 26 of 39 live queens.

## Replays watched
- duel-9e541ce6 g10 (weakhold, try 2): enemy (base) queen died r245; ours stayed parked at length 2
  to r499 and won 2-0. Intended.
- duel-9e541ce6 g16 (around_unsw, try 2): ours parked at 2 all game, the base queen walked out at
  r450 and grew 2→15; lost 15-2. Gate blocked a live race: the motivation for try 3.
- duel-73e0ddfd g44 (islands, try 3): the base queen died r375; ours kept to her ring (len 2-5),
  won 4-0. duel-26c6e951 g56 (autarky v cs, try 3): ours stayed at 3 while cs's queen grew 3→19
  from r450; base on the same seed exited and grew to 32 and won. The cost case.

## Arena verdict (ring-sched-30, 448 games, 252-0-196)
- vs parent ring-sched-17: 57-0-55 (50.9%, BEATS parent within noise); server 34-34, custom 18-16,
  official 5-5. Decided by: elimination 23-23, longer queen 23-22, longest 11-10. Eval P vs parent
  0.50 at r100/r200/r300 (play is identical before r430).
- vs ring-rush-1 68-44, cs-queen-split-tail-risk-v1-7 66-46, ring-plus-21 61-51.
- Target metric vs those three opponents: enemy queen dead before r430 and ours alive at r430 →
  ours then died at r430+: 3 of 109 (2.8%), vs ring-sched-17's own games 17 of 171 (9.9%).
- CPU: worst p99 27.3M, max 38.4M.
