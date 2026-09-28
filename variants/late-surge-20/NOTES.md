# late-surge-20

## Idea

Crossover of late-surge-18 (base, #1 in the league) and gather-split-trade-46: port gst46's
open-exit veto onto known-portal crossings into ls18, without its debug tally.

## What changed from the parents, and why

Parent late-surge-18 (namespace ls18 -> ls20, LS18_* -> LS20_*).

Both ls18 and gst46 carry gst42's known-portal crossing: a blind step through a known portal is
normally capped by `avoid_blind`, but the cap lifts when last turn's ping through it echoed
clean, the exit was seen empty a round or two ago, or we've refused the same goal's blind step
`blind_fallback` times running (`known_portal`, `blind_fallback` in Params).

gst46 measured that most deaths within 2 rounds of a lifted crossing are head-to-head on the
crossing move itself, into an exit with 3 unknown/occupied sides (a head beside the exit steps
into it as we cross; corridor exits with 1-2 unknown sides are nearly safe). I ported gst46's fix
verbatim: after a lift by "exit seen empty" or "fallback" is decided, count the exit's other
three neighbours (excluding the tile we come from) as heads / bodies / unknown-or-stale, and
withdraw the lift when an enemy head was seen there within 4 rounds, or heads+bodies+unknown > 2
(`exit_risk_max = 2`). Lifts by a clean ping are left alone (`exit_risk_ping = 0`, matching
gst46's committed config — vetoing pinged crossings too cut deaths further in gst46 but brought
back costly portal-doorstep detours). Left out gst46's `GST46_DEBUG` tally entirely (not even
compiled out under an ifdef) since the task said to leave it out; the veto logic itself needed no
debug scaffolding to port.

## What should improve

Fewer worker deaths to blind fallback/seen-empty crossings into open portal exits on official
maps (schooltime, default, similar layouts), for a small win, matching gst46's own measurement
against gst42 (22 -> 12 crossing deaths on 4 official maps, schooltime 2-0).

## What I measured

`check late-surge-20 maps/official`: all checks passed, 0 avoidable deaths (slowest turn timings
noisy under machine load, as gst46 also noted; my own machine showed the same on late-surge-18's
unmodified checks).

vs late-surge-18, both seats:
- official maps (11 maps, 22 matches): **11-0-11**, longest -0.5, total -7.3 (even; the "portal:
  exit seen empty" / "portal: fallback" action counts are far too small a sample on 11 maps to
  show a length trend from this single change).
- worldgen (20 fresh maps, both seats, 40 matches): **20-0-20**, longest +0.3, total -7.5 (even;
  matches gst46's own worldgen result: 0 vs 0 on the fallback path is worldgen's much lower
  fallback-trigger rate, gst46 also found "seen empty" and fallback rarely fire off official
  maps).

Overall: even with the parent within noise on both map sets, same shape as gst46's own result
against gst42. Not a measurable win in this short run (too few games to resolve a change that
only fires on a handful of crossings per match), but it doesn't cost anything either, and it
should be a strict improvement whenever the open-exit head-to-head risk does show up (as gst46
demonstrated at higher sample size on official maps: 22 -> 12 deaths, schooltime 2-0).

## What didn't work / next

- Did not try `exit_risk_ping = 1` here (gst46 found it costs pearls/detours for a bigger death
  cut; out of scope for this focused port).
- Next: gst46 flagged a related risk it didn't fix: a "pinged clear" crossing into a corridor
  exit with 1 unknown side dies later (trapped) fairly often on official maps. Worth checking
  whether ls18's escape-room logic already covers that case or needs the same treatment.
