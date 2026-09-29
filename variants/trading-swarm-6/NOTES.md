# trading-swarm-6

## Idea

A sparring partner cloning real-server opponent 10729 (which beat our live bot
gather-split-trade-90 7 of 20, out-eating it 244-49 on Devil and 148-27 on
Default). Measured its rules from 20 replays (M547322/*, M561834/*, M572141/*)
with `analyze10729.py` (kept alongside this dir; not part of the build) and
`decode.py`/`walk()` from league-1's analysis tools.

## What I measured from the replays

- **Split rule**: 10729 splits off length-2 children almost entirely from
  exactly parent length 4 (93%+ of ~3000 splits pooled across all 20 replays;
  the rest is a long tail up to ~20-35 on a few maps, presumably from getting
  stuck long). It never waits for 5 or 6.
- **Never consolidates**: unlike our line (which, from round 350, feeds short
  dragons into a "champion" ally to save their pearls), 10729 keeps splitting
  and growing dragon *count* all game. On Devil (M572144) it reaches 12
  dragons by r30 and 17-26 by r65-100 while we sit at 8-10; final tally
  244-49 pearls, eliminated at r122. On Default (M561838) similarly: 148-27,
  16 dragons at r100 vs our 5.
- **Dead ends**: it does walk 2-3-long dragons straight into 1-wide dead-end
  columns toward the bed at the end, then splits there to send a 2-long child
  back out while the remainder farms the bed on its timer. I did not get to
  build a clean per-tile dead-end classifier to quantify "how often" within
  the 45-minute budget; qualitatively this matches what ts4/ts5 already do
  (`kNew` portal/room farming, `RoomBed`), so I left that alone rather than
  guess at a change I couldn't measure.
- Sonar and head-trading: no obvious difference from what ts5 already does
  (relay a champion's position, trade adjacent/lunge onto shorter or
  equal-length enemy heads); I didn't find a rule in the replay events that
  contradicts ts5's existing behaviour there.

## What changed from ts5, and why

Namespace renamed `ts5` -> `ts6` (and `TS5_DEBUG`/`TS5DBG` -> `TS6_DEBUG`/`TS6DBG`)
so it doesn't clash when loaded into one simulator alongside ts5.

Tried and reverted: removing the late-consolidation "feed" behaviour entirely
(section 3b) and letting splitting continue uncapped to the end of the game,
to directly clone "10729 never consolidates." This is closer to what the
replays show, but it blew the CPU budget badly on large maps: `check`'s
slowest turn on schooltime went from ~0.1ms (parent) to 117ms with the
consolidation cutoff pushed to r420, because it keeps far more short dragons
alive doing full remembered-board pearl searches late game. I could not find
a CPU-safe way to fully remove the late cap within the time budget, so I kept
the existing late-consolidation safety valve (round >= 350, `kLateRound`)
essentially as ts5 has it, but nudged two dials in the direction the replays
support without regressing CPU:
- the rare "keep splitting past length 8" roll: probability up from 1/40 to
  1/25 per turn (round < 300), so short-lived long dragons throw off a few
  more late children, closer to 10729's long tail of higher-length splits.
- kept the primary split window (length 4-7, immediate) as-is: I confirmed
  ts5 already splits at the *first* opportunity once L==4 and unit_count
  allows, so its split-length distribution was already close to 10729's
  "almost always exactly 4" — the random 4-6 `NewThreshold()` value is
  actually dead code (nothing reads `threshold_` in the split decision), so
  there was no real discrepancy to fix there.

Net effect: this is a small, CPU-safe nudge toward more sustained splitting,
not a full population-growth clone (that would need either a cheaper pearl
search for long-lived swarms or a different cap strategy than I had time to
build).

## Measurements

`build/tools/check trading-swarm-6 maps/official`: all checks passed, slowest
turn <=0.1ms on every official map (all comfortably under the 0.5ms rule of
thumb). Note: an earlier, more aggressive draft (uncapped splitting, no late
consolidation) blew this badly (117ms on schooltime, 10-28ms on several other
maps) even though trading-swarm-5's own big_empty baseline is already a slow
outlier at ~12.6ms from unrelated causes; the final version reverted that and
is fast everywhere.

- vs trading-swarm-5 (parent), maps/official, 2 threads: **8-4-10** (near
  parity, as expected from a small nudge).
- vs trading-swarm-5 (parent), 20 worldgen maps (seed default batch): **24-0-16**
  (net positive, no draws).
- vs gather-split-trade-90 (our live bot), maps/official: **7-1-14** overall.
  Per map of interest: Devil - win as B (85-0 pearls) but loss as A (26 vs 88,
  gst90 pulls ahead late); Default - win as A (80-0) but loss as B (95-33,
  close game gst90 wins late). So the clone beats gst90 on one seat of each
  target map but not both; it did not clearly achieve "beats gst90 on both
  seats of Devil and Default" that the brief wanted. That would need the
  fuller (CPU-safe) population-growth clone I ran out of budget for.

## What I'd try next

- A cheaper pearl-search fallback for dragons once team unit_count is high
  (e.g. skip the full remembered-board search and just drift/wander when
  there are already many teammates nearby), so the late-consolidation cutoff
  can be pushed later or removed without CPU blowup — this is the real
  blocker to fully cloning 10729's relentless swarming.
- A proper per-tile dead-end classifier over BoardMemory to measure how often
  10729's dragons commit to 1-wide dead ends vs how often ts5 already does,
  instead of the qualitative read I had time for.
- Re-run the Devil/Default gst90 matchup after the above to see if eating a
  turn earlier via less passive consolidation flips both seats.
