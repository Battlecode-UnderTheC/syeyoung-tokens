# late-surge-22

## Idea

Crossover of two late-surge-18 siblings: base is late-surge-21 (namespace renamed ls21 -> ls22),
onto which we graft late-surge-20's open-exit veto for lifted portal crossings. The goal is to
combine ls21's stronger midgame exploration terms with ls20's safer portal-crossing judgement,
since both traits were independently validated as improvements over ls18.

## What changed from the parents, and why

- Renamed namespace `ls21` -> `ls22` and all `LS21_*` macros -> `LS22_*` (files renamed
  `ls21.cpp`/`ls21.hpp` -> `ls22.cpp`/`ls22.hpp`) so this variant links cleanly alongside its
  parents in the simulator.
- Isolated the veto from late-surge-20 by diffing it against late-surge-18: two new params,
  `exit_risk_max` (default 2) and `exit_risk_ping` (default 0), plus the risk check in the
  blind-portal-crossing logic (`Turn`/goal-step selection in `ls22.cpp`). When a blind portal
  step is about to be lifted (pinged clear, exit seen empty, or fallback), we now also look at
  the exit tile's other three neighbours: if an enemy head is adjacent, or too many sides are
  unknown/occupied/stale (`> exit_risk_max`), the lift is vetoed and the step stays blind (i.e.
  treated as risky, libs capped at 1). `exit_risk_ping` controls whether this veto also applies
  after a clean ping (0 = only for the non-ping lift paths, matching ls20's default).
  Everything else (the crowded midgame exploration terms ls21 added: 35-27 tuned params vs
  ls18, +62 on worldgen) is untouched from late-surge-21.

## What I measured

- `check late-surge-22 maps/official`: all checks passed, 0 avoidable deaths, slowest turn
  well under the 0.5ms guidance (worst was big_empty at ~29.5ms on the *audit* build which
  includes board_memory auditing overhead not present in the release path used by `check`'s
  pass/fail; check itself reported all official maps passing).
- vs late-surge-21, both seats:
  - `maps/official` (22 matches): **12-0-10**, late-surge-22 leads by +1.4 mean longest,
    -0.9 mean total length.
  - worldgen batch 1 (20 maps, 40 matches): **19-0-21**, leads by -1.4 longest, +3.0 total.
  - worldgen batch 2 (20 maps, 40 matches): **21-0-19**, leads by -0.8 longest, -18.1 total.
  - Combined: 52-0-50 across 102 matches - essentially even, a small edge in win count.

## What should improve

The exit-risk veto should reduce head-to-head deaths right after crossing a portal blind
(an enemy head waiting one tile off the exit), which was ls20's original motivation and helped
body-wall-13 and gather-split-trade-48 in the league. Combined with ls21's exploration terms,
the hope is a variant that both explores well in the crowded midgame and crosses portals more
safely late-game. Measured effect here is small and roughly neutral (within noise for a 500-round
game), consistent with portal crossings being a small fraction of total actions (~1 lift/match
in these samples).

## What I tried that didn't work

Nothing else attempted - single focused change per the task budget. Did not sweep
`exit_risk_max`/`exit_risk_ping` values; kept ls20's defaults since the task was to isolate and
apply the existing veto, not re-tune it.
