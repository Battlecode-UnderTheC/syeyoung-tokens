# gather-split-trade-128

## Idea

Strategist-m P2 (strategy/strategist-m-r6.md): in the opening (round < 150), let dragons cross
portals into the portal-only maze regions (Trauma, Default) instead of pacing the portal edge.

## What changed from the parents, and why

Parent gather-split-trade-115. Namespace/macros renamed gst115 -> gst128. Three parameters:

- `opening_portal_cross = 1`, `opening_portal_end = 150`: in SafeStep, a blind portal step
  (exit out of view) is lifted ("portal: opening known") when the exit tile was ever seen,
  not only in the last 2 rounds.
- `opening_portal_blind = 1`: ... and also when it was never seen, unless last turn's ping
  through it found a dragon (`exit_dirty_`).
- In the opening, the "portal: explore" crossing of a clean-pinged UNKNOWN portal uses the
  gst101 poor-home rule for every dragon, not only poor ones: cross even when the current goal
  has value, as long as that goal is >= `poor_cross` (6) steps away.

The first two turned out to change almost nothing: in sim they fire ~3-5 times a game and
replays on Trauma/Default (seeds 5-7 vs ts8) were identical to gst115's. The crossing
bottleneck is not SafeStep's blind check. It is the gate on unknown portals: before, a
non-poor dragon crossed a pinged-clean unknown portal only when it had no goal or a worthless
one. The third change is the one that matters.

## What I measured

Crossings in r0-99 (unswbc run, seeds 5/6/7, us as A vs trading-swarm-8; build/cross.py over
league-1 regions.py), gst115 -> gst128:
- Default: 5/2/5 -> 17/13/5 crossings; portal-only pearls eaten 18/2/16 -> 31/11/22.
- Trauma: 5/9/8 -> 7/10/10 crossings; eaten 17/29/14 -> 22/32/24. Still well behind ts8
  (56-123 eaten inside).

sim (W-D-L, gst128 first):
- vs gst115: official 12-0-10 (default 2-0, devil 1-1, stronghold 0-2, trauma 1-1);
  worldgen seed 107000 x20 both seats 20-0-20. r100 total 47.4 v 50.5 official, 109.9 v 112.8 wg.
- vs gst96 (live v7): official 12-0-10 (default 1-1, devil 2-0, stronghold 0-2, trauma 2-0);
  worldgen 23-0-17. r100 total 57.6 v 43.2 / 119.8 v 96.6.
- vs trading-swarm-8: official 18-1-3 (default 1-1, devil 0-2, stronghold 2-0, trauma 2-0);
  worldgen 28-0-12.
- Own-move deaths per match (official, vs gst115): body 3.1 v 2.5, head-to-head 29.3 v 28.8.
  A few more body deaths, probably blind crossings.

For comparison, the first-two-changes-only version (commit af19dbc): vs gst115 11-11
official, 22-18 worldgen; vs gst96 13-9 / 24-16; vs ts8 18-1-3 / 27-13. Within noise of the
final version.

## What should improve

More maze pearls on Default and Trauma in the opening. Trauma vs ts8 went from 1-1 to 2-0 in
sim. Elsewhere it's neutral: even with the parent overall (32-30).

## Not tried / next

- The Trauma maze is still under-farmed. Next: stop needing a ping before each dragon's
  first crossing (share portal pairs over sonar) and weight maze pearls seen through a portal.
- Not checked in the judge sandbox, which seeds pearls per match.
