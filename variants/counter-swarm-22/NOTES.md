# counter-swarm-22

## Idea

counter-swarm-20 plus gather-split-trade-39's obituaries and heard-split: a champion that dies or
trap-splits its mass away stops being named, because the correction is relayed. Strikes, trade
rules and the champion guard are unchanged.

## Diagnosis (counter-swarm-20 vs gather-split-trade-39, worldgen set 1, 20 maps x 2 seats)

Trace (`CS22_TRACE`, compiled out): every one of our dragons at r340/400/460/499 prints its
length, the champion it names, that report's age and its reason. "Dead-named" = naming an id
that no longer plays.

| round | dragons | name true champion | name a dead id | feeding / held (that turn) |
|---|---|---|---|---|
| 340 | 1386 | 510 (37%) | 375 (27%) | 0 / 0 |
| 400 | 1262 | 480 (38%) | 548 (43%) | 10 / 3 |
| 460 | 547 | 189 (35%) | 271 (50%) | 1 / 1 |
| 499 | 425 | 149 (35%) | 202 (48%) | 1 / 0 |

- The biggest problem is naming a dead dragon: about half the team at 460/499. The true
  champion's id was gone by the next checkpoint in 63 of 76 checkpoint pairs: the mass keeps
  moving to new ids (trapped split: child carries the mass, stub keeps the id and dies) or the
  champion dies. Ghost reports are young (median age 16): ghosts are freshly relayed, not stale.
- Only 12% of ghost naming points at the previous checkpoint's champion: most ghosts are
  short-lived intermediate champions.
- Feeding / held is rare in snapshots (feeds are a few per match; "held" nearly never): the
  hold rule is not the bottleneck.
- The champion is almost always alive in some form (longest keeps growing between checkpoints),
  so this is naming, not losing the champion.

## What changed from the parent

- Namespace/macros cs20 -> cs22, files cs22.{hpp,cpp}.
- Ported from gather-split-trade-39: obituaries (`obit_*`: a report cutting a known >= 10 ally
  below half, or our own length falling below half of our last self-report, or an ally >= 5
  beside our head vanishing on a plain step, is pinged on odd rounds for 20 rounds), heard-split
  (growth split / long trapped split chosen so the child hears our tail ping), and the `fresh`
  ordering fix in ally sightings.
- Kept: counter-swarm's strike (not the lunge), trade rules, hunting cut at 300, champion guard.

After the fix, same maps and seats: true champion named 43% (was 35%) at r460, dead-named 42%
(was 50%); at r400 56% (was 38%). Better, but still well below gst39's own reported 72%.

## Measured (sim, both seats, threads 2)

| opponent | official (22) | worldgen set 1 (40) | worldgen set 2 seed 8808 (40) |
|---|---|---|---|
| counter-swarm-20 | 12-0-10 (+2.3 longest) | 20-0-20 (-1.2) | 19-0-21 (-2.2) |
| gather-split-trade-39 | 11-0-11 (+1.7) | 21-0-19 (-1.4) | 19-0-21 (-1.1) |
| (parent cs20 vs gst39) | 9-0-13 (-3.5) | 21-0-19 (+0.8) | 21-0-19 (+3.4) |

Totals: cs22 vs gst39 51-51, cs20 vs gst39 51-51; cs22 vs cs20 51-51. Official improved
(9-13 -> 11-11), worldgen slightly worse; all within noise.
check: all checks passed (slowest turn 38 ms under load average ~150; parent 53 ms same run).

## What should improve

Fewer feeders walking to ghost champions; mostly visible on official maps.

## Next

Ghost naming is still ~40%: the mass changes id too often. Ideas: a trapped champion splits
off a small tail (so the stub keeps the mass and id) when that frees a move; or a heartbeat bit
in the champion's self-report so relays can tell alive-and-far from dead.
