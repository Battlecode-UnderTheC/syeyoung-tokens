# gather-split-trade-73

## Idea

Stop our small stubs from queueing nose to tail in 1-wide corridors (replay pattern #27,
"column jam"). A corridor tile is one with kelp on at least two sides.

## What changed from the parents, and why

Parent: gather-split-trade-68 (namespace renamed gst68 -> gst73). All changes behind
`corridor_jam` / `corridor_no_split` (on; `GST73_OFF` turns the jam checks off).

- `Jammed(to)`: walk the 1-wide corridor that a step onto `to` enters (stop at a fork, an
  unknown edge, or its end) for up to length + `jam_extra` (2) tiles; an ally segment (head
  or body, seen this round) in it = jammed. SafeStep won't take a jammed goal step, and ranks
  a jammed step below any unexposed, escapable one (key: !exposed, level>0, !jam, ...).
- With no goal ("no goal: roomiest"), SafeStep prefers a non-corridor step over a corridor one
  (same rank slot, after !jam). Debug dumps on devil showed most column-jam deaths had
  "no goal: roomiest" as the previous action (61 of 106 in one match), i.e. goalless stubs
  wandering into long columns and meeting an ally head-on from the far end, out of view when
  they entered. This change cut devil trapped deaths in that match 106 -> 59.
- Decide: no growth split while the head stands on a corridor tile.
- EscapeLevel was left alone: it already treats every other dragon's tile (ally heads
  included) as a permanent blocker.

## Measured (sim, both seats)

| opponent | maps | gst73 | gst68 (same opp.) |
|---|---|---|---|
| gst68 | official (22) | 12-0-10, +3.0 longest, +16.5 total | - |
| gst68 | worldgen A (40) | 21-0-19, +0.2 / +10.7 | - |
| gst68 | worldgen B (40) | 20-0-20, -1.4 / +4.3 | - |
| trading-swarm-5 | official | 13-1-8 | 14-1-7 |
| trading-swarm-5 | worldgen A | 30-0-10, +94.5 total | 31-0-9, +102.7 |
| trading-swarm-5 | worldgen B | 31-1-8, +80.8 total | 27-1-12, +65.2 |

Trapped L2-3 deaths not at cap (per match, "fatal turns") vs trading-swarm-5: worldgen A
92.2 vs 98.7, worldgen B 74.4 vs 90.6, official 35.5 vs 35.5. Mirror vs gst68 on official:
58.4 (ours) vs ~66 (theirs). Devil vs trading-swarm-5 both lose 0-2; ours dies later so it
counts more trapped deaths (81.5 vs 34).

The first version (jam check + no-split only, without the no-goal corridor preference) cut
almost nothing (worldgen mirror 132 vs 139, 119 vs 110) and was 10-0-12 on official vs gst68.

## What should improve / next

A modest cut in non-cap stub traps; gather results about even. The remaining column deaths
are head-on meetings in corridors longer than the 7x7 view: neither side can see the other
when it enters. Next: remember a recently seen ally in a corridor (stale occupant within a few
rounds), or announce "entering corridor X" by sonar; or give goalless stubs a real goal
(feed / patrol open ground) so they stop wandering.

`check` slowest turn is noisy on the shared machine (the parent measured 91 ms on trophy in
the same session); the added work is O(length) per candidate step.
