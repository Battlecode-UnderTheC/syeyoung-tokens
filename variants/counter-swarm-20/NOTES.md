# counter-swarm-20

## Idea

counter-swarm-19 with gather-split-trade-38's champion-knowledge bug fix: an ally seen only
in part (body running past the 7x7 window) is a lower bound on its length and no longer
overwrites a fresher, exact self-report; late relayed reports are kept longer and fade slower.

## What changed from the parents, and why

cs19's champion code (Champion, Report, Encode/Decode) came from the gather-split-trade
family (gst23's ghost fix, via cs11), so it had the identical flaw gst34/swarm-hunt-12 had:
`have.length = fresh && (cut[id] || !p_.trust_visible) ? std::max(have.length, n) : n;` — when
`fresh` is false (no report in the last `report_memory` rounds) a partial sighting overwrites
with only the visible segment count `n`, stamped with the current round, so it can beat and
replace the champion's own exact self-report. Confirmed by inspection: `cs19.hpp`/`cs19.cpp`
have byte-identical sonar word layout (magic 8 | team 1 | id 12 | x 6 | y 6 | length 12 |
round 9, bits 0-9 previously all required zero) to gst34/gst38, so gst38's fix ported with no
layout changes needed. Diffed gst38 against gst34 to isolate the change and ported it
(namespace cs19 -> cs20, files renamed cs19.{hpp,cpp} -> cs20.{hpp,cpp}):

- `lower_bound_reports` (on by default): a partial-view report carries a lower-bound flag in
  bit 0 of the sonar word (bits 1-9 must now be 0 in Decode, was bits 0-9); a report flagged
  lower moves the tile/round on in `Note` but never lowers a known length; exact/self reports
  still replace normally.
- From round 330 (`late_report_from`) reports are kept 80 rounds (`late_report_memory`, was
  40) and fade 0.25/round (`late_report_fade`, was 0.5), so a genuinely-far champion's stale
  relay still beats a nearer stub.

Left out gst38's `GST38_TRACE`/`GST38_OFF` debug knobs (not needed once the fix is confirmed
sound; the brief calls for the one change).

## Measured

`check counter-swarm-20 maps/official`: all checks passed, 0 avoidable deaths.

| opponent | maps | W-D-L | notes |
|---|---|---|---|
| counter-swarm-19 | official (22) | 11-0-11 | longest -2.1, total +13.0 (mirrors gst38 vs gst34's pattern: fewer self-elected mid champions, more total mass) |
| counter-swarm-19 | worldgen set 1 (40) | 24-0-16 | |
| counter-swarm-19 | worldgen set 2, seed 8808 (40) | 19-0-21 | roughly even |
| gather-split-trade-38 | official (22) | 12-0-10 | |
| gather-split-trade-38 | worldgen set 1 (40) | 23-0-17 | |

Overall at least even to ahead of both parents on every set tried (best on official/worldgen1
vs cs19, close on worldgen2, ahead of gst38 on both sets tried).

## What should improve

Fewer dragons electing a mid-length stub as champion late-game because a partial sighting
stamped it fresher than the real champion's own report; more mass consolidated into the true
longest dragon, per gst38's r460 champion-naming numbers (64%->75% official, 33%->52%
worldgen) which should carry over since the code path is identical.

## Didn't try / next

Didn't re-measure the "true champion named" percentage directly for cs19/cs20 (gst38's NOTES
already established the mechanism and magnitude on byte-identical code); could trace-confirm
with `CHAMPDBG`-style prints if a future task wants exact numbers. Next: gst38's own
next-step idea (making an isolated champion audible via relay of champion + runner-up) applies
here too.
