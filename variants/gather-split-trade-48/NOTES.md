# gather-split-trade-48

## Idea

Crossover: gather-split-trade-45's base (gst42 + late-surge-17's bed/cluster scoring) plus
gather-split-trade-46's open-exit veto on lifted blind portal crossings. Both parents descend
from gst42's known-portal blind-step lift; gst46 measured that most deaths right after a lift
are head-to-head on the crossing move into an exit with 3 unknown/occupied sides, and that
vetoing the "fallback" and "exit seen empty" lifts into such exits (leaving clean-pinged lifts
alone) cut crossing deaths without hurting gathering.

## What changed from the parents, and why

Parent gather-split-trade-45 (namespace gst45 -> gst48, GST45_* -> GST48_*).

- Isolated gst46's change by diffing it against its own parent gst42 (both `strategy.cpp` and
  `strategy.hpp`), then ported only that diff onto gst45's tree (leaving out gst46's debug tally,
  which was `#ifdef`-gated and only ever used to produce the numbers already in gst46's NOTES.md).
- Added params `exit_risk_max = 2` and `exit_risk_ping = 0` (same defaults gst46 committed).
- In the known-portal lift block (`strategy.cpp`, the `if (blind && p_.known_portal)` section):
  when a step would be lifted, count the *exit* tile's other 3 neighbours as currently known
  (enemy head seen within 4 rounds / body seen within 2 / free seen within 2 / stale-or-unknown).
  If an enemy head is known beside the exit, or heads+bodies+unknown > `exit_risk_max`, withdraw
  the lift (unless it came from a clean ping, since `exit_risk_ping = 0`).
- Left the debug tally out entirely (compiled out via `#ifdef GST48_DEBUG`, off by default, same
  as the base).

## What I measured

`check gather-split-trade-48 maps/official`: all checks passed, 0 avoidable deaths. "Slowest
turn" readings (30-140 ms on default/schooltime/stronghold) are machine-load noise, not the
variant: gst45's and gst46's own measurements note the same under shared load.

vs gather-split-trade-45, both seats:

| maps | W-D-L | longest margin | total margin |
|---|---|---|---|
| official (11, both seats = 22) | 12-0-10 | +0.5 | +2.5 |
| worldgen batch of 20, both seats = 40 | 20-2-18 | -0.8 | -4.7 |
| combined | 32-2-28 | | |

Schooltime (the map gst46's veto targeted): gst48 won both seats (as A: 88-230 total but longer
dragon 54-47; as B: 200-0), consistent with gst46's report that gst42 lost schooltime 2-0 and the
veto flips it.

## What should improve

Fewer deaths right after a blind portal-crossing lift on maps with open (3-unknown-side) portal
exits, per gst46's isolated tally (crossing deaths 22 -> 12 on 4 official maps in that report).
Overall record here is at least even with the parent (32-2-28); margins are mixed (slightly up
on official, slightly down on worldgen), within noise for a single 20-map batch.

## What didn't work / next

Didn't retune here (budget: port the one change, measure, submit). Gst46's own notes found that
also vetoing clean-pinged lifts (`exit_risk_ping = 1`) further cuts crossing deaths but brings
back portal detours and costs gathering (pearls eaten -1.5%), so I kept `exit_risk_ping = 0` as
committed. Next: gst46 flagged that "pinged clear" crossings into a 1-unknown-side corridor exit
die later (trapped) 8 of 23 times on official maps — worth checking escape room past the exit,
not just the exit's immediate neighbours.
