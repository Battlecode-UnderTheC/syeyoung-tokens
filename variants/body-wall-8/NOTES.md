# body-wall-8

## Idea

body-wall-7 (renamed bw7 -> bw8) with gather-split-trade-38's champion-knowledge fix, ported
from the gst34->gst38 diff (originally from swarm-hunt-13). bw7's champion code descends from
the same gather-split-trade-23 line and had the identical flaw, confirmed before porting.

## What changed from the parents, and why

Confirmed the flaw: in `Turn()`'s report-building loop (the ally-in-view segment), an ally
whose body ran past the 7x7 window was reported at its visible length (`n`) whenever there was
no fresh existing report (`fresh` false), and stamped with the current round via
`have.round = round_` — identical to gst34's code, byte for byte. That partial, shorter length
then beat the champion's own exact self-report by round freshness in `Champion()`, so mid-size
dragons could elect themselves champion and the team's mass never consolidated onto the true
longest.

Ported (namespace bw7 -> bw8, macros BW7_* -> BW8_*, applied before the fix):

- `Report` gains `bool lower`: a report built from a partial view (`cut[id] || !trust_visible`)
  sets it; `lower_bound_reports` gates the new behaviour (kept on, no `_OFF` escape hatch since
  gst38 already validated it and the brief asks to just port the fix).
- Ally-in-view loop: under `lower_bound_reports`, a partial report never lowers a known
  length (`have.length = part && have.length > n ? have.length : n`).
- `Encode`/`Decode`: bit 0 of the sonar word now carries `lower`; `Decode` checks bits 1-9 are
  zero (was bits 0-9), so layout for magic/team/id/x/y/length/round is unchanged.
- `Note`: a newer lower-bound relayed report advances the tile/round on a known ally but never
  shrinks its length; exact reports still fully replace (trap-split stubs still correct
  themselves).
- `Champion`: from round `late_report_from` (330) reports are kept `late_report_memory` (80)
  rounds and fade at `late_report_fade` (0.25/round), instead of the base `report_memory` (40)
  / `report_fade` (0.5) — relayed reports from a far champion arrive old late in the game, and
  used to fall out of memory or get outscored by fresher, shorter self-reports.

No debug/trace scaffolding ported (gst38's `_TRACE`/`_OFF` were tuning aids; the brief is a
short loop, not a sweep).

## Measured

sim vs body-wall-7 (both seats, --threads 2):

| maps | W-D-L | longest margin | total margin |
|---|---|---|---|
| official (22 matches) | 11-0-11 | +0.0 | +5.7 |
| worldgen batch (20 maps, 40 matches, fresh seed) | 19-0-21 | -0.4 | +32.6 |

`check body-wall-8 maps/official`: all checks passed, slowest turn 16.7 ms on `big_empty`
(same map is 42.8 ms for body-wall-7 unmodified — a pre-existing body-wall-7 characteristic,
not introduced by this change, and not this task's focus).

## What should improve

Same mechanism gst38 measured: fewer self-elected mid-size champions, more team mass
consolidating onto the true longest late-game, seen here as flat W-L (within noise) but a
consistent positive total-length margin on both map sets, matching gst38's own parent-relative
result shape (its W-L was near-even on official, clearly positive on worldgen totals).

## Didn't try / next

Did not port gst38's `_TRACE` champion-naming instrumentation, so I could not directly measure
body-wall-8's "names true champion at r460" share the way gst38's NOTES.md did for its own
family; the sim W-D-L/margins stand in for it here given the time budget. Next: if a later
task wants to chase gst38's other open idea (isolated clusters with 0 dragons knowing the
champion), it applies equally to this line.
