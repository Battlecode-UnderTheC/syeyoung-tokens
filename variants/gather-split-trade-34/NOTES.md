# gather-split-trade-34

## Idea

Crossover stacking two independent wins over gather-split-trade-31: gst33's portal-crossing
fix (single-ping sonar through an unknown portal, cross next turn only if the echo is clean,
body fully known/in view, no enemy head visible, pure-explore goal, before round 480; plus
the loop breaker banning the current/previous goal for 50 rounds on a stuck last-12-heads
pattern) and gst32's `hunt_radius: 6 -> 0` (workers stop chasing enemy heads). Confirmed by
diff that gst32 vs gst31 differ only in that one parameter (plus namespace renaming).

## What changed from the parents, and why

Base is gst33's sources (namespace renamed gst33 -> gst34, including macros, to avoid clashing
when both are loaded into one simulator). The only functional change on top of gst33 is
`hunt_radius` 6 -> 0 in `strategy.hpp`, taken from gst32. No other logic touched.

## What I measured

- `check gather-split-trade-34 maps/official`: all checks passed, 0 avoidable deaths.
- `sim` official maps (22 matches, both sides as A and B):
  - vs gst33: 13-0-9, mean 364 rounds, leads by 6.0 longest / 3.1 total.
  - vs gst32: 11-0-11, mean 339 rounds, leads by 0.3 longest / -0.8 total.
- `sim` on 20 fresh worldgen maps (40 matches, both sides as A and B):
  - vs gst33: 18-0-22, mean 422 rounds, leads by -2.2 longest / +18.1 total.
  - vs gst32: 23-0-17, mean 456 rounds, leads by 3.6 longest / -7.0 total.

Net: roughly even-to-ahead of both parents across official + worldgen maps (clear win vs
gst33 on official, even vs gst32 on official, split but overall positive across the two
worldgen sets). No avoidable deaths, no wall/self-collision issues introduced.

## What should improve

Combines fewer idle-loop losses (gst33's portal/loop fix) with fewer worker deaths from
chasing enemies into bad trades (gst32's hunt_radius=0), which should compound on maps with
both portals and aggressive enemy contact.

## What I tried that didn't work

Nothing else attempted; budget was for the single stacked change only, no parameter sweeping.
