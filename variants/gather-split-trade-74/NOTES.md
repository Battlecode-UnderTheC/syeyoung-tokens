# gather-split-trade-74

## Idea

Crossover: collect the gst line's two most recent isolated wins onto the gst72 base
(gst68's centre pull + champion dead-end skip, dead-end skip for all >= 10 from r400,
late merge of a >= 20 non-champion into the champion).

1. **split_announce** (from gather-split-trade-64, diff vs gather-split-trade-62): when a
   trapped/doomed split sends at least half our length off, we ping our own id at the new
   (short) stub length that same turn, in all four directions, and start our obituary relay
   for it. Without this, our last-known report still names us at the old (long) length, so
   the child that just split off — and anyone else nearby — keeps electing the soon-dead
   stub as champion; that's the ghost champion problem (r450 186 -> 52 named-dead on gst64's
   own measurement). Gated on `reason_[0] == 't' || 'd'` (trapped/doomed splits only; growth
   splits are routine and would just crowd the sonar with obituaries).
2. **give-up-at-cap** (from gather-split-trade-70, diff vs gather-split-trade-63; itself
   ported from body-wall-21): step 4b in `Decide()`. A 2-3-long non-leader, non-champion stub
   idling far from its goal, at or near the unit cap, next to a long ally or a crowd of
   enemy/ally heads, resigns to free a cap slot instead of dying trapped with no split room.
   `league/KNOWLEDGE.md` notes every trapped death of length >= 4 happens at the cap with no
   split possible; this targets exactly that failure by having a cheap dragon sacrifice its
   slot voluntarily before it's forced into one anyway.

## Collision check

- **Sonar**: split_announce only overwrites `a.sonar` on the turn of a trapped/doomed
  mass-off split, and only when `a.kind == kSplit`. give-up-at-cap only returns `Because(...,
  Resign())`, never touches sonar directly (the champion/obit sonar block above it in `Turn()`
  still runs for a resign action). No overlap: a dragon can't split and resign the same turn.
- **Turn order**: give-up-at-cap is inserted as step 4b, right after step 5's goal is chosen
  (needs `goal`/`paths.dist[goal]` to know the stub is idle) and before the loop-breaker. It
  runs after growth-split (step 4) and before any goal-directed move, matching gst70's
  placement relative to gst63. split_announce lives entirely in `Turn()`, after `Decide()`
  returns, so it can't interact with step ordering in `Decide()` at all.
- Namespace `gst72` -> `gst74` and every `GST72_*` macro -> `GST74_*`, so this links cleanly
  alongside its parents in one simulator.

## Measured

`build/tools/check gather-split-trade-74 maps/official`: all checks passed, 0 avoidable
deaths (per-map "slowest turn" ms readings are noisy on this shared machine — re-running
`check` on the unmodified parent gst72 showed the same swings, 0.1 ms to 44 ms turn to turn,
so this isn't a regression from these changes).

vs gather-split-trade-72 (base), `maps/official`, both seats, threads 2:
- 13-0-9 (W-D-L) over 22 matches; at the end leads by 3.0 longest, -6.0 total.
- @cap trapped-death actions barely present either side on this small official set.

vs gather-split-trade-72 (base), 20 fresh worldgen maps, both seats, threads 2:
- 22-0-18 (W-D-L) over 40 matches; at the end leads by 2.1 longest, -2.0 total.
- Cap-trapped deaths fell sharply as intended: `trapped: no move L2-3 @cap` 6 vs 57,
  `L4-5 @cap` 8 vs 37, `L6-9 @cap` 5 vs 18 (gst74 vs gst72, totals over 40 matches).
  `give up slot: crowd` (160) and `give up slot: long ally` (17) actions fire regularly and
  those turns are rarely fatal (160.0 and 17.1 "fatal turns" tallies are the resigns
  themselves, not extra deaths caused by them — no action ever kills more than one dragon).
- "no action" (natural end-of-round deaths, mostly from resigning) rose 346 -> 479.8 as
  expected: give-up-at-cap trades a controlled resign for what used to be a forced trapped
  death, and head-to-head/self/wall/body deaths didn't move.

Given the short budget I did not also run vs gather-split-trade-64 or trading-swarm-5 as the
brief's stretch goal asked; the two required parent-vs-base comparisons (official + worldgen)
were the priority and both came back even-or-ahead on W-D-L and longest margin.

## What I'd try next

- The total-length margin is still slightly negative in both sets even though W-D-L and
  longest are ahead: worth checking whether give-up-at-cap's resigns are giving away pearls
  the team would otherwise have kept (a resigning dragon leaves its length as pearls, but if
  it resigns *before* being cornered it may have had more room to keep growing first — a
  slightly stricter idle-distance or unit-count gate might close this without losing the
  W-D-L gain).
- Run against gather-split-trade-64 and trading-swarm-5 directly (not just the base) to see
  whether the combined change holds up outside the gst72 matchup.
- Consider whether split_announce's four-direction ping should be split-directional instead
  (cheaper, less sonar contention) now that give-up-at-cap adds more resigns and thus more
  obituary relay traffic in the same rounds.
