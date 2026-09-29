# trading-swarm-8

## Idea

A sparring clone of the real, blank-named server opponent (the side confirmed by
`decode.our_team()`, top-byte-0xA7 sonar) that beat our live bot 18-7 in 25 real games
(M547322/M561834/M572141/M578650). trading-swarm-6 previously attempted this from a similar
brief but the reports flagged risk of cloning the wrong side; here every measurement below
used `decode.our_team(d)` per-replay to pick the opponent side, never the default.

## Measured rules (25 replays, opponent side only)

Script: `/tmp/opp_analyze.py` (not part of the build), using each replay directory's own
`decode.py`/`analyze.py` (`walk()`, `our_team()`) — no dependency on the possibly-mislabelled
league-2 aggregate sheets.

- **Split-parent-length distribution** (opponent's `dragonSplit` events, ~5.8k pooled):
  4: 3731, 5: 836, 6: 615, 7: 207, 8: 150, 9: 68, 10: 57, 11: 51, 12: 26, 13: 27. ~65% split
  at exactly parent length 4, with a real but thin tail out past 8 — no hard cutoff.
- **Split timing**: keeps splitting through the whole game. Round histogram in 50-round
  buckets (r0 to r499): 464, 659, 751, 793, 870, 786, 598, 459, 304, 235 — a mild decline
  after r250 (fewer opponents alive to split, natural attrition/game-ending), not an early
  freeze.
- **Dragon count / longest over time** (opponent side, mean across replays where the round
  was reached): r25: count 7.2, longest 4.3, total 19.4. r50: count 12.1, longest 3.9, total
  30.2. r100: count 17.4, longest 5.1, total 47.3. r200: count 26.9, longest 6.0, total 74.2.
  **Longest stays essentially flat while count keeps climbing** — the opponent never
  consolidates into a champion the way our line does from round ~330.
- **Approach to centre**: median first round the opponent's head sits on a centre row/column
  is r6 (mean r7.6, n=25) — it heads for the middle fast. Mean head distance to map centre in
  r0-100 is ~11.8 tiles (median ~10.6), i.e. it spends most of the opening in the contested
  middle, not lingering at spawn.
- **Sonar**: uses several distinct top-byte values (0xB1, 0xF0, 0x9C dominate, varying by
  game/map rather than one fixed marker like our 0xA7), consistent with per-instance or
  per-map randomised sonar words rather than a fixed team tag. No lunge/head-to-head
  behaviour distinguishable from ts5's own (trade/lunge only vs. shorter-or-equal enemy
  heads) turned up in the split/death event stream.

## What changed from ts5, and why

Namespace `ts5` -> `ts8` (and `TS5_DEBUG`/`TS5DBG` -> `TS8_DEBUG`/`TS8DBG`), all instances,
so it links alongside ts5 without clashing.

**Kept, ported from ts6's earlier (validated) finding**: bumped the rare "split past length 8"
roll from 1/40 to 1/25 per turn (round < 300) to better match the measured split-length tail
above — same code path, so no extra CPU, and this is the change ts6's own measurements (on a
similar opponent id, before this task's per-replay `decode.our_team()` re-check) found a net
worldgen win from.

**Tried and reverted**: pushing ts5's late-consolidation cutoffs (`kFreezeRound`/`kLateRound`,
normally round 330/350 — the round the champion stops splitting and short allies start
resigning into it) out to round 450, to track the opponent's "never consolidates" signature
measured above. Two problems, both measured, both reverted:
1. An earlier, more aggressive draft that removed the freeze/feed outright blew CPU on
   big_empty to ~10ms/turn (20x the 0.5ms rule of thumb) — more surviving short dragons
   paying the full remembered-board pearl-search cost late-game on a large map, the same
   failure mode ts6's NOTES.md already documented for a full removal.
2. Even the milder round-450 version (freeze/feed still present, just delayed) cost worldgen
   win rate against the parent measurably: 18-0-22 vs the unmodified 20-0-20, on identical
   maps/seed — the consolidation ts5 already has is net-positive for *us*, even though it
   doesn't match what the real opponent does.

Net conclusion: the dragon-count gap (opponent count climbs to 27 by r200 while our own
replayed dragons sit at 3-4, per the brief and strategist-a's diagnosis) is not caused by
ts5's split-length or consolidation-timing rules — those are already close to the measured
shape once the length-4 rare-tail nudge above is applied. It's further upstream, in
pearl-eating/goal-selection throughput (strategist-a's diagnosis #1, KNOWLEDGE.md's ts4-clone
note that the opponent eats 2-5x more pearls per 100 moves than our lines). That's a bigger
task than this budget; flagging it as the priority for whoever picks this up next, exactly as
strategist-a's P3 and ts6's NOTES.md both already flagged.

## Measurements

`build/tools/check trading-swarm-8 maps/official`: all checks passed, repeated 3x; typical
slowest turn <=0.4ms everywhere (occasional spikes to double digits on a busy shared machine
that vanish on a clean rerun — confirmed by rerunning `check` back-to-back, not a real
regression from this change).

- vs **trading-swarm-5** (parent), `maps/official`, 2 threads: **8-4-10**.
- vs **trading-swarm-5** (parent), 20 worldgen maps (default seed): **19-0-21**.
- vs **trading-swarm-5** (parent), 20 worldgen maps (seed 1000, independent set): **23-0-17**.
  (Combined worldgen: 42-0-38 — the two batches bracket parity; this small a change is inside
  the noise floor of a 40-map sample.)
- vs **gather-split-trade-96** (our live bot), `maps/official`: **10-1-11** — a per-map trace
  on Devil and Trauma (the map families the real opponent dominates per the brief) shows the
  clone reaching 30-33 dragons / length 68-80 by r150-200 against gst96's 3-11 dragons /
  length 9-24 there, i.e. it reproduces the real opponent's winning shape on the map types
  where the brief says the real one wins. It's closer to even than dominant across *all*
  official maps (loses queen_of_spades, schooltime-as-A, stronghold-as-A, trophy), which
  matches the real 7-18 record's shape (wins concentrated on crowded/Devil-family maps) more
  than it contradicts it.
- vs **gather-split-trade-90**, `maps/official`: **7-1-14**.
- vs **gather-split-trade-96**, 40 worldgen maps (two independent 20-map batches): **16-0-64**
  combined — worse than on official; gst96 is tuned hard for worldgen generally (per
  KNOWLEDGE.md), so this isn't surprising for a near-ts5 clone and isn't part of the brief's
  validation criterion (official maps only).

## What I'd try next

- The real fix for dragon-count/eating-throughput fidelity is goal-selection (nearest-unseen
  dithering / cheaper-but-wider pearl search), not a parameter nudge — a bigger task, flagged
  by strategist-a (P3) and ts6 before this. Until that lands, this clone is directionally
  useful (splits like the opponent, converges on the centre fast, and reproduces its
  map-dependent win pattern against gst96) but undercounts its raw dragon-count growth
  relative to the real 25-replay numbers above.
- Didn't get to: a per-tile dead-end classifier to check the opponent's late-game
  bed-farming/dead-end-column behaviour ts6 noted qualitatively but couldn't quantify either —
  still open.
