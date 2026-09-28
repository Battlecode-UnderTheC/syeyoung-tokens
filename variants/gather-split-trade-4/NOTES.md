# gather-split-trade-4

## Idea

Concentrate the late-game mass into one dragon, the team's **champion** (the longest ally
known). Every dragon pings, in all four directions each turn, a 64-bit sonar word describing
the longest ally it knows of (itself included): magic, team, id, head x/y, length, round.
Receivers keep the newest report per id, so the report is relayed dragon to dragon (a ping
stops at the first dragon it hits, so relaying is what carries it across the team).

## What changed from the parents, and why

Parent: gather-split-trade-1 (namespace renamed gst1 -> gst4).

- Feeding target: instead of "the biggest ally this dragon has seen within 80 rounds" (each
  worker picked its own, so mass was spread over several dragons), every dragon feeds the
  one champion: the longest report no older than 40 rounds, ties to the lowest id.
- A feeder walks to the champion's reported head even when it is out of view, but only dies
  (resigns) when the champion is in view now and its head is within `feed_distance` steps,
  so a stale report never makes it die next to nothing.
- Feeding starts at round 360 (was 400): more time to walk over and for the champion to eat.
- Leaders that are not the champion also feed it from round 440, if they located it within
  the last 10 rounds.
- From the feed round the champion ignores `share_factor` (it no longer gives way to allies
  for pearls: the feeders' pearls are meant for it).

## Measured (vs gather-split-trade-1, both sides of each map)

- official maps: feed 400 -> 12-0-10, longest +2.4; feed 360 -> 12-0-10, longest +3.4
  (the short games are side-determined mirror results; the long ones went 6-4 at feed 400).
- 20 worldgen maps: feed 400 -> 21-0-19, longest +1.7; feed 360 -> 25-0-15, longest +5.7.
- check: all checks passed; slowest turn ~14 ms native under load (the parent showed 56 ms).

## What should improve

Long games decided on the longest dragon (big_empty, default, open worldgen maps). Total
length drops a little (feeders' odd segments are lost), which only matters for ties.

## Didn't try / next

- Feeders only die when they see the champion; many never reach it (feed deaths ~5 per
  official match). Routing towards the champion through unknown territory could help.
- Protecting the champion (escorts, stricter exposure rules late) is untouched.
