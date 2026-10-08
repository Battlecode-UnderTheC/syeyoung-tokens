# ring-field-loneq (g52-8)

- **Line:** ring-field. Base: ring-field-freecost (arena `ring-field-18`).
- **Brief:** ../syeyoung-tokens.league/briefs/g52-8.md (strategist A #2 / B #2, arena-r26 S2).

## Idea
A queen left alone mid-game sits in her 6-cell ring, which avoids beds, and starves (arena-r26 S2:
165 losses with the queen alone 100+ rounds eating <= 2 pearls). The opening forage (forage35 /
EARLY15 mode 6) already makes a lone short queen roam for food and split at 4, but it latches only
at round 0. Re-arm it whenever she is alone.

## Change (strategy.hpp, strategy.cpp QueenMove only)
- Counters `lone_rounds_` (units == 1 in a row) and `crowd_rounds_` (units >= 2 in a row).
- Trigger: `lone_rounds_ >= kLoneQueenRounds (8)`, `0 < round < kLoneQueenUntil (400)`, she is not
  already roaming (`!queen_growing_`: the r0 opening roam and portal escapes are unchanged), no
  corridor at her head and `OpenRoom >= kForageRoom` (a sealed pen never triggers it). Sets
  `queen_forage_ = queen_growing_ = lone_forage_ = true`, `forage_since_ = round`, clears the ring.
- From then the existing forage path runs unchanged: no home-distance pull, corridor and low-room
  penalties, threat penalty, the queen17 split at length 4 (no threat on her head, not in a
  corridor, SplitSafe).
- Settle (existing forage35 settle: split if safe, else stop): units >= kForageSettleUnits, or
  kForageSettleRound rounds after the spell began (was: absolute r120; the r0 forage is unchanged
  since forage_since_ = 0), or for a lone spell 2+ dragons for kLoneQueenClear (10) rounds in a
  row, or round 400.
- `LONEQ_DBG` (off by default) prints a per-turn queen line for local measurement.

## Local
- check maps/official: pass.
- Target maps v ring-field-1, 6 seeds x 2 seats (dbg build v the same code with the trigger off):
  | map | W-L | lone rounds/g | lone pearls/g | units r200 | queen alive r500 |
  |---|---|---|---|---|---|
  | prisoners_dilemma | 6-6 v 6-6 | 38 v 124 | 0 v 0 | 7.2 v 7.5 | 0 v 0 |
  | fixed-pen | 6-6 v 6-6 | 150 v 126 | .25 v .25 | 2.8 v 2.8 | 7 v 5 |
  | fixed-portal-larder | 6-6 v 6-6 | 73 v 66 | .50 v .08 | 7.5 v 7.5 | 2 v 1 |
  | fixed-seam-meet | 6-6 v 6-6 | 1 v 1 | 0 v 0 | - | 0 v 0 |
  - Dilemma seed 1 (A): alone from r136 circling 4,5-6,6; trigger r144; she leaves the ring and
    walks the top row (r144-r180) but finds no pearl and dies head-on at r180 (base: alive to
    ~r285-375, same loss). On dilemma the losing seat's queen dies ~100 rounds earlier.
- Fresh mapgen (--family all, seed 852008, 44 maps, seeds 1,2) v ring-field-freecost: 86-4-86;
  per family all even (8-0-8; maze and sealed-spawn 7-2-7). The local sim is seat-mirrored for
  near-identical bots, so it can't separate them.

## Arena
- Not run by the worker: the arena was unreachable from the worker sandbox. Bot dir built with
  scripts/make-bot.sh at /tmp/g528/bot.
