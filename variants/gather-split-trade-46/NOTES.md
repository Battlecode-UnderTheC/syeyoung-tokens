# gather-split-trade-46

## Idea

gst42 lifts the blind-step cap on a known portal (clean ping / exit seen empty / 2 refusals).
I measured which lifted crossings kill, then vetoed the lifts that have no ping behind them when
the exit has 3 unknown (or occupied) sides.

## What changed from the parents, and why

Parent gather-split-trade-42 (namespace gst42 -> gst46, GST42_* -> GST46_*).

- Debug tally (`GST46_DEBUG`, compiled out; prints `GST46LIFT` lines at exit): every crossing a
  lift allowed, keyed by condition, length (long = 6+), and the exit's other neighbours as the
  dragon knows them (enemy head seen within 4 rounds / body seen within 2 / free seen within 2 /
  stale or unknown; walls skipped). It also counts deaths within 2 rounds, split into: died on the
  crossing move itself, died on another dragon's turn (a head moved onto ours), died on a later
  turn of its own. It hooks the agent destructor (sim destroys an agent when its dragon dies) and
  a thread_local "last agent to act".
- Fix: `exit_risk_max = 2`, `exit_risk_ping = 0`: a lift by "exit seen empty" or "fallback" is
  withdrawn when an enemy head was seen beside the exit or heads+bodies+unknown sides > 2. Lifts
  by a clean ping are not vetoed (`exit_risk_ping = 1` vetoes them too; see below).
  `exit_risk_max = 4` turns the veto off (plays exactly like gst42).

## Tally (gst42's rule; gst46 debug build vs gst42, both seats)

schooltime+default+trauma+queen_of_spades (8 matches): 143 lifted crossings, 36 died within 2
rounds: **21 on the crossing move itself** (in traces these were head-to-head: a head beside the
exit stepped into it after our ping), 1 on another's turn, 14 on a later turn (mostly trapped
after a "pinged clear" crossing into a corridor exit with 1 unknown side: 8 of 23).

| exit sides unknown | pinged clear: cross / die on cross | fallback | seen empty |
|---|---|---|---|
| 3 (open) | official 52 / 7, worldgen 44 / 12 | official 23 / 9, worldgen 4 / 1 | few |
| 2 | 23 / 2, 34 / 0 | 3 / 0 | 2 / 1 |
| 1 | 23 / 0 (but 8 later deaths), 4 / 0 | 3 / 0 | 9 / 1 |

Worldgen (20 maps, both seats): 105 lifted crossings: 21 died on the crossing, 4 on another's
turn, 1 later. Enemy heads/bodies beside the exit were almost never *known* (heads>0 in 2 of 248
lifts). That's why gst43's "remembered head within 1" never fired: the exits are simply unseen.
The open exit (3 unknown sides) is what kills; corridor exits (2 unknown) are nearly safe.
Long dragons hardly ever use the lift (5 of 248).

After the fix (veto on fallback and seen-empty only): official-4 has 125 crossings, 12 deaths on
or at the crossing and 9 later (was 143 / 22 / 14). Worldgen is unchanged (103 / 23 / 3) because
fallback rarely fires there. With the veto on pings too (`exit_risk_ping=1`): official-4 75 / 7 / 8,
worldgen 77 / 11 / 2. But portal detours rose 826 -> 1465 and pearls eaten fell 1.5% (the doorstep
waltz came back).

## Measurements vs gather-split-trade-42, both seats

| config | official (11) | worldgen seed 4646 (20) | worldgen seed 91919 (20) |
|---|---|---|---|
| exit_risk_ping=0 (committed) | 12-0-10, longest +1.3, total +1.6 | 19-0-21, -0.9, -2.0 | 20-0-20, 0.0, 0.0 (identical games) |
| exit_risk_ping=1 | 12-0-10, +0.4, +0.5 | 22-0-18, +1.6, -16.2 | 18-0-22, -1.4, -6.3 |

Schooltime: 2-0 in both configs. Overall even with the parent (51-51 / 52-50), within noise.
check: all checks passed (machine load ~113, so the timings are noise: the parent measured 37 ms too).

## What should improve

Fewer workers wasted by blind fallback crossings on open-exit portal maps (schooltime, default).

## What didn't work / next

- Vetoing open exits even after a clean ping halves crossing deaths, but it brings back the
  detours and costs gathering. Many crossing deaths are head-to-head *trades* (both die), so they
  cost less than they look.
- Next: a "pinged clear" crossing into a corridor exit with 1 unknown side dies later (trapped)
  8 of 23 times on the official maps, so check escape room past the exit. No ping can see
  sideways from the exit, so the head-beside-exit risk can't be measured directly.
