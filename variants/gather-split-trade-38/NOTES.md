# gather-split-trade-38

## Idea

gather-split-trade-34 with swarm-hunt-13's champion-knowledge fix: an ally seen only in part
is a lower bound on its length and no longer shrinks the champion's report; late relayed
reports are kept longer and fade slower.

## What changed from the parents, and why

gst34 had the same flaw as swarm-hunt-12 (identical line): an ally whose body ran past the 7x7
window was reported with only its visible segments whenever there was no fresh report, stamped
with the current round, so it beat the champion's exact self-report and mid-size dragons
elected themselves. `trust_visible` only covers the opposite case (whole body in view).
Ported from swh13 (namespace gst34 -> gst38):

- `lower_bound_reports`: a report built from a partial view carries a lower-bound flag in bit 0
  of the sonar word (bits 0-9 were all zero and checked; now bits 1-9 must be 0, so
  magic/team/id/x/y/length/round layout is unchanged). A newer lower-bound report moves the
  tile/round on but never lowers a known length; exact reports still replace (trap-split stubs
  still correct themselves).
- From round 330 reports are kept 80 rounds (was 40) and fade 0.25/round (was 0.5).
- Temporary debug: `GST38_TRACE` prints each dragon's champion at r400/r460; `GST38_OFF`
  reverts both changes (== gst34 behaviour). Both compiled out by default.

## Measured (sim vs gather-split-trade-34, both seats)

| maps | W-D-L | longest margin | total margin |
|---|---|---|---|
| official (22) | 11-0-11 | -2.0 | +8.3 |
| worldgen set 1 (20 maps, 40) | 20-0-20 | +0.2 | +55.9 |
| worldgen set 2 (seed 8808, 40) | 22-0-18 | +2.0 | +10.7 |

Dragons naming the true champion (the team's longest) at r460 (trace build, our side only):

| maps | before (GST38_OFF) | after |
|---|---|---|
| official | 103/160 = 64% | 128/171 = 75% |
| worldgen set 1 | 165/498 = 33% | 356/681 = 52% |

Still, several worldgen matches have 0-6 of 20-60 dragons knowing (isolated clusters).
check: all checks passed (slowest turn up to 7 ms under load average ~156; noisy).

## What should improve

Fewer self-elected mid-size champions; more mass fed into the real longest. On W-L it is
within noise of the parent; totals are up.

## Didn't try / next

Make an isolated champion audible: relay champion + runner-up, or pass reports to a
trapped-split child at birth (it knows nothing).
