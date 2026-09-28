# gather-split-trade-41

## Idea

gather-split-trade-39 with one change: a lunge (multi-step MOVE onto an enemy head) fires only
at an enemy at least one longer than us (`lunge_margin = 1`). Adjacent kamikaze trades keep the
old rule (enemy at least as long). A lunge is us *starting* an even trade from a distance; on
crowded maps those one-for-one trades are what the trading swarms win.

## Where the mid-game growth race is lost (trace study, gst39 vs late-surge-12 / counter-swarm-20)

`sim --trace` already prints both teams' dragons and total length every round, so no debug
code was needed for the snapshots (script: per map and seat, dragons/length at r50/100/200/300/380).
gst39 went 12-10 / 22-18 vs late-surge-12 and 13-9 / 17-23 vs counter-swarm-20 (official / 20 worldgen).

- In nearly every loss the gap is open by r50-r100 and only widens: e.g. Tidal Gates (pg_268416)
  s1: r20 13 v 18 dragons, r50 26/62 v 62/163, r100 21/53 v 63/182. Trophy s1 r50 7/20 v 14/33;
  Colosseum s0 r50 10/25 v 15/37 then r100 2/5 v 21/51. Games are decided in the first ~100 rounds.
- The same map in the other seat is often a win: the early race is chaotic (small differences in
  who reaches which beds first snowball), not a systematic deficit. Whole-set averages of action
  reasons and deaths are nearly identical to late-surge-12's.
- Early-phase tallies (temporary `GST41_PHASE`, compiled out) in a lost vs won game on the same
  map: splits/turn 3.9% v 5.1%, pearl goals 18% v 20%, bed waits 27% v 18%, no goal ~8-10% in both.
  No single leak (idle loops, stuck splits) stands out.
- On crowded official maps (Trophy, Colosseum) the early deaths are mostly our lunges and
  kamikazes: 7 of our lunge deaths by r100 on Trophy, 10 + 13 on Colosseum.

## Measured (sim, both seats)

| vs | official (22) | worldgen set 1 (40) | worldgen set 2 (40) |
|---|---|---|---|
| gather-split-trade-39 | 12-0-10 (+8 total) | 24-0-16 (+46 total, +6.7 longest) | 21-0-19 (+0 total, +4 longest) |
| late-surge-12 | 12-0-10 (parent 12-10) | 21-0-19 (parent 22-18) | |
| counter-swarm-20 | 10-0-12 (parent 13-9) | 24-0-16 (parent 17-23) | |

57-45 vs the parent overall: slightly positive, within noise.

## What should improve

Fewer self-started even trades in the early race, so more dragons keep gathering. Worst risk:
crowded official maps vs counter-swarm (10-12, parent 13-9), where lunging back may matter.

## Not tried / next

The early growth gap looked chaotic rather than a single leak; next would be many-map statistics
of the r0-r50 phase (pearls eaten per dragon-turn, bed-wait time) rather than single traces.
