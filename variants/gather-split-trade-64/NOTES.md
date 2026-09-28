# gather-split-trade-64

## Idea

gather-split-trade-62 plus a same-turn announcement of trapped splits. When a dragon of length
>= 10 trap-splits (or doom-splits) most of its length off, its pings that very turn carry its own
id at the stub length (this round) instead of its old full-length report, and it starts its
obituary relay right away. Before, the split turn's ping named the parent at full length; the
child (born right behind the parent's tail, it hears that ping first) and the dragons around
took that fresh report as champion, and the team kept feeding the stub's id after it died.

## What changed from the parents, and why

- Namespace `gst62` -> `gst64`, macros `GST62_*` -> `GST64_*`.
- New param `split_announce = 1` (0 under `GST64_OFF`/`GST64_BASE`), in `Turn()` after the
  champion/obituary pings: on a `trapped: split` / `doomed: split` with `2n >= length` and our
  own report >= `obit_champion`, set `reports_[self]` and `obit_` to (self, head, length - n,
  round) and ping it in all 4 directions. Growth splits are excluded (a first version including
  them went 14-26 on worldgen A; obituaries for routine splits eat half the sonar turns).
- Probe `GST64_CDBG` extended (compiled out; `sim --threads 1`): for each dragon naming a dead
  id at r380/r450, why that id died: `split-mass-off:<reason>` (it split >= half its length off
  within 30 rounds of dying) or `died:<last action reason>` (resign = no action at the cap), plus
  the stale report's age and whether the namer had an obituary of it.

## Why the ids went stale (probe, gst62-equivalent build vs gst62, worldgen A, 26 full matches)

Stale namings at r380 (774 of 1635) / r450 (214 of 639):
- champion split its mass off: r380 ~460 (266 growth "split", ~190 trapped split), r450 186 (all trapped split L10+)
- resigned at the unit cap (trapped, no split possible): r380 ~195 (129 of them length >= 20)
- died stepping ("toward goal"/"detour", i.e. head-to-head or body): r380 ~270, r450 ~40
- trapped: trade 62 at r380
Stale reports are young (10 rounds at r380, 21 at r450): it is not the fade; they are fresh
reports of a dragon that just died. Almost none of the stale namers had heard an obituary.

## Measured

Probe (practice A, 26 full matches), base -> this change:
| | r380 live | r450 live | r499 longest share |
|---|---|---|---|
| gst64 | 861/1635 (53%) -> 853/1632 (52%) | 425/639 (67%) -> 435/525 (83%) | 0.320 -> 0.385 |
Trapped-split L10+ ghosts: r380 100 -> 7, r450 186 -> 52.
Official (10 full matches), first version: r450 live 74/161 -> 111/161.

`sim --threads 2`, both seats. practice A/B: two fresh sets of 20 worldgen maps. "base" is this
code with `GST64_BASE` (= gst62) on the same maps, for the noise level.
| opponent | maps | W-D-L | longest | base W-D-L | base longest |
|---|---|---|---|---|---|
| gst62 | official (22) | 12-0-10 | +0.7 | | |
| gst62 | worldgen A (40) | 14-0-26 | -3.5 | 19-0-21 | -2.2 |
| gst62 | worldgen B (40) | 20-0-20 | +3.2 | 19-0-21 | -2.3 |
| body-wall-16 | official (22) | 12-0-10 | +1.6 | | |
| body-wall-16 | worldgen A (40) | 21-0-19 | +3.2 | 17-0-23 | +0.1 |
| body-wall-16 | worldgen B (40) | 17-0-23 | -4.8 | 14-0-26 | -7.2 |
Per-map diffs vs base are chaotic (most maps change result both ways): one changed ping
changes the whole game. Net vs body-wall-16 +7 wins over 80; vs gst62 -4 over 80.

## What should improve

At r450 more of the team names a live champion (67% -> 83%) and the longest ends with a larger
share of team length, so the endgame feed reaches the real champion.

## Open problems / next

- r380 knowledge didn't move: the biggest remaining r380 cause is growth splits (`split`, a third
  of stale namings) by a dragon that was named champion shortly before r330 and then died; and
  cap resignations of long dragons (a resigning dragon casts no pings, so no one learns). Try
  announcing growth splits only from ~r300, or the child inferring its parent (a heard report of
  length own+2/+3 last round) and noting it at the stub length.
