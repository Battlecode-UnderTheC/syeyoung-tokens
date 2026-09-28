# giant-2

## Idea

giant-1's design (round-0 dragons are "giants": they split only until round 100, then avoid
every enemy head and grow; split children are helpers that gather, trade, and later feed the
king named by sonar gossip), with the helpers' economy brought up to the gather-split-trade line.

## What changed from the parents, and why

- **Target sharing (helpers only):** a pearl/bed a visible ally head is torus-nearer to is worth
  0.1×. Giants don't yield (they are the growers). Biggest single piece: turning it off on
  official went 11-11 → 3-19 vs giant-1 (with an earlier bug present), 19-21 vs 27-13 on worldgen.
- **The lunge (helpers only):** a MOVE of up to min(length−1, 6) steps over free, in-view tiles
  onto an enemy head that is already a wanted trade. Off: 6-16 official, 23-17 worldgen.
- **Split window 300 → 350, feed from 300 → 340** (helpers).
- **Ghost stub:** a giant's trapped split (length−2) now demotes the stub to helper, so it stops
  naming itself king; the heir (child ≥ 8 long) takes over. Ranking was already
  `length − 0.5 × age` with a 40-round expiry in giant-1, so the ghost problem was smaller here.
  Beware: a length-4 giant's growth split (length/2) is also `length−2`; the first version
  demoted every giant that way (0-22 official vs giant-1). Fixed by keying on the reason.
- **Cap feeding** (length ≤ 3 helper at the unit cap dies next to a longer ally within 2) is
  implemented but off (`cap_feed_length = 0`): neutral to slightly worse (11-11 vs 12-10
  official, 32-8 vs 33-7 worldgen).

## Measurements (final build, 20 fresh worldgen maps × 2 sides)

| opponent | official | worldgen |
|---|---|---|
| giant-1 | 12-0-10 | 33-0-7 |
| gather-split-trade-23 | 10-0-12 | 13-0-27 |
| (giant-1 vs gst-23) | 5-0-17 | 10-0-30 |

## What should improve

Helpers' economy (more pearls per helper, fewer duplicate chases), so the giants get fed a
bigger swarm. Still behind gst-23 on worldgen.

## Tried / noted

- Accidentally all-helper (giants demoted at their first split) went 27-13 worldgen vs giant-1
  but badly on official: the giants themselves are not obviously paying on worldgen.
- giant_split_until 300 (giants split as long as leaders do): 34-6 worldgen vs giant-1,
  9-13 / 15-25 vs gst-23; within noise, kept 100 to keep the giant idea distinct.
