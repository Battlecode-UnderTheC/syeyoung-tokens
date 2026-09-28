# swarm-hunt-4

## Idea

swarm-hunt-3's hunters and keepers, with gather-split-trade-4's endgame consolidation: the
team agrees by sonar on one champion (the longest ally known) and late in the game every
other dragon walks to it and dies next to it, so the mass ends up in one dragon.

## What changed from the parents, and why

Base: swarm-hunt-3 (namespace swh3 -> swh4). swarm-hunt-3 sent no sonar (it only reads
`echo_present` on the first turn to tell keepers from split children), so there is no clash.

- Every dragon pings, in all four directions each turn, gather-split-trade-4's 64-bit word
  (magic 0xA7, team bit, id, head x/y, length, round) for the longest ally it knows
  (itself included). Receivers keep the newest report per id; allies in view update their
  report directly. Champion = longest report no older than 40 rounds, ties to lowest id.
- Replaced swarm-hunt-3's "biggest ally seen within 80 rounds" feeding target with the
  champion. Feeders walk to its reported head even out of view, but resign only when it is
  in view now and within feed_distance (2).
- Hunters feed from round 360 (was 400; also ends hunting at 360). Keepers that are not the
  champion feed it from 440 if it was located within the last 10 rounds (previously keepers
  never fed).
- From round 360 the champion ignores ally_yield (it no longer leaves targets to nearer
  allies).

## Measured

- vs swarm-hunt-3, official maps (both sides): 13-0-9, longest +2.5, total -12.2. The long
  games (big_empty, stronghold, trauma, schooltime, default) went 7-3; the short ones are
  side-determined mirrors. default was lost both ways.
- vs swarm-hunt-3, 20 fresh worldgen maps: 24-0-16, longest +3.5, total -4.7.
- vs gather-split-trade-4, official: 11-0-11 (swarm-hunt-3 there: 6-0-16).
- check: all checks passed (slowest turn 33 ms native on schooltime, under machine load).

## What should improve

Round-499 decisions on the longest dragon. Total length drops a little (feeders' odd
segments are lost), which only matters for ties.

## Didn't try / next

- No parameter sweep (feed rounds 360/440 taken as is from gather-split-trade-4).
- default was lost both sides vs swarm-hunt-3 (one a wipeout at round 446): worth a trace.
- Protecting the champion late (keepers' guard already helps) and routing feeders through
  unknown territory.
