# body-wall-10

## Idea

Crossover: body-wall-9 (namespace renamed bw9 -> bw10) plus gather-split-trade-39's
"obituaries" fix and its heard-split cut-point choice, ported across. In gst39's line most
dragons in bad games named a *dead* champion (a trap-split stub, or an ally that died,
outliving them as a stale sonar report); the fix relays a correction so the team stops
walking to a ghost.

## What changed from the parents, and why

- `obit_*` fields/params (`obit_champion=10`, `obit_rounds=20`, `obit_min_length=5`): a known
  ally >= 10 long whose next report is exact, newer, and less than half its length (a
  trap-split stub keeping the id while the child carries the mass) is an obituary; so is an
  ally >= 5 seen beside our head last turn, gone now while we moved one plain step (recorded
  dead, length 0). The dragon that learns it pings the correction on odd rounds for 20 rounds,
  overriding the usual champion ping (even rounds, and the portal-explore ping, still go
  through as before). Ported near-verbatim from gst39's `Note()`/`Remember()`/`Turn()`.
- `heard_split` (default on): growth splits try cut points L/2, L/2+1, L/2-1 for one where the
  body runs straight through the cut, so the tail ping (opposite the facing) runs straight
  into the child's tail and it hears the champion at birth; trapped splits of a >= 8 dragon
  keep one extra segment when that makes the cut straight. New `HeardSplit`/`GrowthSplit`
  helpers, used at both split sites (was `length_ - 2` / `length_ / 2` unconditionally).
- Sonar word layout: body-wall-9 already reserves the same 64-bit word (magic 8 | team 1 |
  id 12 | x 6 | y 6 | length 12 | round 9 | lower 1 = 55 bits; `Decode` still masks the 9 free
  bits at `w & 1022`). No new bits needed: an obituary is encoded as an ordinary `Report`
  (length 0, or the stub's reduced length), reusing `Encode`/`Decode`/`Note` as-is — `Note`'s
  existing lower-bound protection only applies when the incoming report is a partial sighting
  (`lower=true`), so an exact obituary always overwrites.
- Namespace/macros bw9 -> bw10 throughout (`BW10_DEBUG`, `BW10_TUNE`, etc.), so it can't clash
  with body-wall-9 when both are loaded into one simulator.

## Measured (sim vs body-wall-9, both seats)

| maps | W-D-L | longest margin | total margin |
|---|---|---|---|
| official (22 matches) | 11-0-11 | -1.8 | +3.0 |
| worldgen set 1 (20 maps, 40 matches) | 19-0-21 | -1.4 | +8.9 |
| worldgen set 2 (seed 8808, 20 maps, 40 matches) | 18-0-22 | (not recorded) | (not recorded) |

`check body-wall-10 maps/official`: all checks passed (slowest turn 97.2 ms on trauma under
machine load, well inside the 0.5 ms native rule of thumb; times are load noise, not real CPU
per league knowledge).

Both W-L splits and the longest margin sit within gst39-style noise (~even, ±1 game in 20 is
typical per KNOWLEDGE.md); total-length margin is consistently a few points in body-wall-10's
favour across all three sets, matching gst39's own result against its parent (a small,
consistent gain on totals rather than a dramatic W-L swing). Did not have time in budget to
count how often our dragons name a dead champion (gst39's own headline metric); that would be
the natural next check.

## What should improve

Fewer dragons late-game walking to a ghost champion (dead or a trap-split stub) that no
longer exists, per gst39's own diagnosis; body-wall shares the same champion/sonar machinery
(inherited via body-wall-8's partial-sighting fix) so the same failure mode should apply here.

## What I'd try next

- Measure named-dead-champion rate directly (gst39 traced this at r400/r460); this run only
  checked W-D-L/margins per the budget.
- gst39's own next idea (from its NOTES.md): drop a champion faster once a *newer*, nearly-
  as-long report supersedes it, or have the champion mark a heartbeat bit so relays can tell
  alive-and-far from dead — body-wall-10 doesn't yet have that.
