# counter-swarm-5

## Idea

counter-swarm-1 with gather-split-trade-4's **champion** consolidation: the whole team agrees
over sonar on one longest ally and late in the game every other dragon walks to it and dies
next to it, so the round-499 longest-dragon count goes to one dragon instead of ~15 mid-size ones.

## What changed from the parents, and why

Base: counter-swarm-1 (namespace cs -> cs5, files cs5.*). From gather-split-trade-4:

- Every dragon pings, in all four directions each turn, a 64-bit word naming the longest ally
  it knows (magic 0x5C, team bit, id, head x/y, length, round). Receivers keep the newest
  report per id (reports older than 40 rounds are ignored), allies in view update their
  report directly, so the report is relayed and the team converges on one champion
  (longest, ties to lowest id). This replaces counter-swarm-1's (disabled) rumor gossip and
  its per-dragon "biggest ally seen in 200 rounds" target.
- Workers feed from round 340 (counter-swarm-1's round, kept); non-champion leaders feed from
  440 when the champion was located within 10 rounds. A feeder walks to the champion's
  reported head even when out of view, but only resigns when it sees the champion *now*
  within feed_distance (2) — this is what fixes the stale-position problem of counter-swarm-1's
  gossip attempt.
- If the reported head is not reachable through the known map, and the report is at most
  10 rounds old, the feeder heads to the known tile nearest to it (counter-swarm-1's rumor
  walk, but on a fresh, single target).
- From the feed round the champion no longer discounts pearls nearer to allies (x1 rule off).

Everything else (strikes, trading, safety, splitting) is counter-swarm-1's.

## Measured (vs counter-swarm-1, both sides of each map)

- official maps: 13-0-9, longest +4.2, total -54.5 (big_empty skews total).
- 20 fresh worldgen maps: 22-0-18, longest +2.3, total -0.6.
- check: all checks passed.

## What should improve

Round-499 losses on the longest dragon, where counter-swarm-1 had more total length spread
over many dragons.

## Next

- Feeding is still rare (~8 feed deaths per match): most workers never find the champion.
  Better routing toward it, or starting earlier (gst4 used 360 and gained there), untested here.
- Protecting the champion late (it is the only dragon that matters) is untouched.
