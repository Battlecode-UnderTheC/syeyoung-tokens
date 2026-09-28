# gather-split-trade-93

## Idea

Crossover of gather-split-trade-90 (gst84 + crowd rules: bed/pearl goal discounted 0.55 per
ally head within 3, no growth split with 3+ ally heads within 3) and gather-split-trade-88
(gst84 + feed gate: feed only when the team can afford to lose the feeder). The two parents
touch disjoint code (gst90's crowd rules live in ChooseGoal/growth-split; gst88's feed gate
lives in the feeding branch of Decide), so this variant is gst90's base with gst88's feed
gate applied onto it.

## What changed from the parents, and why

- Base: gst90 (namespace/macros renamed `gst93`/`GST93_` to avoid clashing when loaded
  alongside the parents in one simulator).
- Ported from gst88 (diffed against their shared parent gst84 to isolate the change): a
  `feed_afford` gate before the existing `feeding` condition in `Decide`. A dragon only feeds
  the champion when `units_ > max(feed_keep_units, EnemiesSeen())` -- i.e. the team keeps more
  than `feed_keep_units` dragons (default 6) and more than the enemies recently seen/heard
  after this one dies. This fixes replay pattern #34 ("feeding into elimination"): late-game,
  every non-champion dragon walking to feed the champion, thinning the team below what it
  needs to hold the board.
- **Deviation from gst88, per an orchestrator note mid-task**: gst88 also gated on
  `champ.length >= feed_min_champ` (12). A sibling test (gst89, same gate ported onto gst68)
  found this hurts: on official maps it went 9-0-13, losing stronghold/trauma both seats,
  because feeding starts at round 380 and the champion is *supposed* to grow by being fed --
  gating on its current length before that growth creates a chicken-and-egg block. I kept
  `feed_min_champ = 0` (off) and applied only the team-size part of the gate.

## What I measured

- `check gather-split-trade-93 maps/official`: all checks passed, 0 avoidable deaths.
- vs gather-split-trade-90 (parent), official maps, both seats: 12-0-10 (as A) + 10-0-12
  (as B, roles swapped) = 22-0-22 combined; essentially even, longest/total margins near
  zero both directions.
- vs gather-split-trade-90, 20 fresh worldgen maps (default batch): 19-0-21 -- also roughly
  even, consistent with the official-map result.
- vs gather-split-trade-88 (parent), official maps: 13-0-9.
- vs trading-swarm-1, official maps: 19-1-2.
- vs trading-swarm-5, official maps: 11-1-10.

Net: at least even with both parents (slightly ahead of gst88, essentially tied with gst90),
and strong against trading-swarm-1, roughly even against trading-swarm-5. No avoidable
deaths, `check` passes cleanly.

## What should improve

The feed gate should reduce the "fed the team into elimination" failure mode; gst88's own
report showed it halves fatal-feed turns vs gst84. Combining it with gst90's crowd-discount
should mean fewer wasted growth splits in a crowd *and* fewer team-thinning feeds, without the
two mechanics interfering (they touch different goals/branches, confirmed by diffing cleanly).

## What I tried that didn't work

Kept gst88's `feed_min_champ` floor at first (as literally described in the brief), but per
an orchestrator note about a gst89 sibling test (same gate on gst68, 9-0-13 on official maps,
losing stronghold/trauma both seats), turned it off (0) before measuring. A length floor
evaluated before the champion has grown from being fed is self-defeating given this
codebase's round-380 feed-start design.

## Ready to submit

Yes: `check` passes, measurements show at least even with both parents on official and
worldgen maps.
