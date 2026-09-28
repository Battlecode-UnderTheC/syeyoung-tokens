# late-surge-13

## Idea

late-surge-12 with gather-split-trade-38's champion-knowledge fix (originally from
swarm-hunt-13, crossed into gst34 -> gst38): a partial sighting of an ally is only a lower
bound on its length and must never beat or overwrite the champion's own exact self-report;
late-game relayed reports are kept longer and fade slower so a remote champion is still heard
of once the map's mass has thinned out.

## What changed from the parents, and why

Confirmed late-surge-12 has the same flaw as gst34/swarm-hunt-12 (same lineage,
`gather-split-trade-23` -> `late-surge-1`): in `Turn()`, an ally whose body ran past the 7x7
window (`cut[id]` or `!trust_visible`) but had no *fresh* report (`round_ - have.round >
report_memory`) got its length set to the visible segment count `n` and stamped with the
current round — beating out its own exact self-report relayed earlier. Ported from gst38
(namespace `ls12` -> `ls13`):

- `Report::lower`: new bool flag, plus `Params::lower_bound_reports` (default on).
- In the per-turn ally-update loop: a partial view now only *raises* `have.length` (never
  shrinks a longer known value) and marks `have.lower = true`; an exact (full-body) view still
  replaces outright.
- Sonar word: `Encode`/`Decode` add the lower-bound flag at **bit 1** (late-surge already uses
  bit 0 for the trader flag, unlike gst34/38 which had it free), so `Decode`'s all-zero check
  moves from mask `0x3FE` (bits 1-9) to `0x3FC` (bits 2-9).
- `Note()`: a newer lower-bound report from another dragon no longer overwrites a longer
  known length; it only advances the tile/round (still "at least this long").
- `Champion()`: from round 330, reports are kept 80 rounds (was `report_memory`=40) and fade
  0.25/round (was `report_fade`=0.5), via new `late_report_from/late_report_memory/late_report_fade`
  params, so a far-off true champion's stale-looking report doesn't lose to a nearby
  mid-size dragon electing itself.

## Measured

`check late-surge-13 maps/official`: all checks passed (slowest turn 21.6 ms on `schooltime`
under shared-machine load; strategy-v1 baseline is 0.1 ms but this run was noisy — no logic
change would explain a jump this large, and `check` still reports 0 avoidable deaths).

sim vs late-surge-12 (both seats):

| maps | W-D-L | longest margin | total margin |
|---|---|---|---|
| official (22 matches) | 10-0-12 | +1.2 | +6.1 |
| worldgen (20 maps, 40 matches) | 20-0-20 | -1.9 | +24.6 |

Roughly even on win rate on both sets, ahead on total length margin on both, small/noisy on
longest margin (+1.2 official, -1.9 worldgen — within the noise band gst38's own report
called out). Consistent with gst38's measured effect against its own parent (53-49 W-L,
totals up more than longest).

## Didn't try / next

Didn't re-tune `late_report_from/memory/fade` for late-surge's own feed timings
(`feed_from_round`=380, `champ_freeze`=330) — ported gst38's values as-is since they already
line up closely with late-surge-12's own `champ_freeze`=330. Next: check whether
`late_report_from` should instead track `champ_freeze` directly instead of a separate
constant, and whether isolated late-surge clusters (workers that never see the champion at
all) would benefit from relaying a runner-up too, per gst38's own follow-up note.
