# swarm-hunt-13

## Idea

swarm-hunt-12 with a sounder champion election in the endgame: allies seen only in part
no longer shrink the champion's reported length, and late relayed reports fade slower.

## Diagnosis (trauma vs gather-split-trade-34, per-dragon dump at r340/400/460/499)

- Champion knowledge is fragmented. At r460/r499 most of ~10 dragons name *themselves* or
  a local 10-13-long dragon as champion; the real champion (19-27 long) is known to 0-3 of
  them. In one seat the champion (id 170, 20 long) was known to nobody and did not grow
  from r460 to r499. Trauma is pockets joined by portals: pings and feeders don't get there.
- Partial sightings under-reported the champion: an ally whose body runs out of the 7x7
  window was reported with only its visible segments (and with no stale report at all, as
  just n), stamped with the current round, so it overrode the champion's exact self-report
  (e.g. `champ60/14 age0` while 60 was 19). Mid-size dragons then out-scored it and elected
  themselves.
- Late trapped splits (length - 2) keep creating new ids (ids 198-222 born after r460),
  each child starting with no reports, adding to the churn. "trapped: no move" deaths are
  mostly 2-3-long dragons at the cap; they don't matter for the longest.
- "held" (a peer >= 80% of the champion) is rare in the tallies (6-10 a 22-match run).

## What changed from the parents, and why

- `lower_bound_reports`: a report built from a partial view carries a lower-bound flag
  (bit 0 of the sonar word; the other 9 low bits still must be 0). A newer lower-bound
  report moves the champion's tile/round on but never lowers the length an earlier report
  gave. Exact reports (self, whole body in view) still replace, so stub corrections after a
  trapped split still spread.
- From round 330 reports are kept 80 rounds (was 40) and fade 0.25/round (was 0.5), so a
  far champion whose relays arrive old still beats a local mid-size dragon.
- `feeder_no_split` (dragons walking to the champion in 380-400 don't split): built, off.

## Measured (sim, both seats)

| a vs b | official | worldgen set 1 (20) | worldgen set 2 (20) |
|---|---|---|---|
| sh13 vs swarm-hunt-12 | 11-0-11 (longest -0.6) | 18-0-22 (-1.2) | 21-0-19 (+3.1) |
| sh13 vs gather-split-trade-34 | 11-0-11 (-2.3) | 17-0-23 (-3.5) | 22-0-18 (-4.5) |
| sh12 vs gather-split-trade-34 | 14-0-8 (+2.1) | 15-0-25 (-3.6) | 21-0-19 (+0.5) |

Trauma vs sh12: 2-0 (24-20, 24-22). Trauma vs gst34 still 0-2 (20-25, 18-23): the isolated
champion case isn't fixed. Single-component runs vs sh12: lower-bound only 10-12 / 42-38;
late fade only 9-13 / 43-37; feeder_no_split only 40-40 worldgen but totals -25..-33.

## What should improve

Fewer self-elected mid-size champions where sonar does reach; overall this is within noise
of the parent.

## Didn't work

- Feeders not splitting on the way to the champion (380-400): even W-L, lower totals.
- Neither change fixes a champion no ally can hear (portal pockets on trauma).

## Next

Make the champion reachable/audible: e.g. the champion walks toward the team centre /
where its reports come from once it's frozen, or relay two words (champion + second best)
so isolated clusters merge locally, then into the champion. Pass reports to a trapped-split
child (it knows nothing at birth).
