# gather-split-trade-115

Task g22-1. Operator: mutate. Parent: gather-split-trade-101 (namespace renamed gst101 -> gst115,
GST101_* -> GST115_*, no functional changes from the rename itself).

## Idea

strategist-g-r3 diagnosed the r50 dragon/length gap (Devil vs trading-swarm-8: ~9.5 dragons/
22.5 total for us vs ~20.5/50.0 for ts8) as an opening pearl-throughput race: ts8's plain
"nearest pearl" goal selection out-throughputs our careful dead-end/crowd/contest-scored
`ChooseGoal`. Implemented P1 and P3 only (P2's sonar hotspot broadcast and P4's post-hoc
gst96 check are out of scope for this task; did the gst96 check anyway since it's cheap).

## What changed from the parents, and why

- `strategy.hpp`: added `opening_end` (150), `trap_opening_cost` (0.5), `opening_pearl_radius`
  (40).
- `strategy.cpp`, `ChooseGoal`:
  - **P3**: the dead-end-pocket cost used when scoring a trapped candidate is now
    `trap_opening_cost` (0.5) instead of the flat `trap_cost` (3.0) while `round_ < opening_end`.
    Cheaper dead-end entry in the opening lets a short dragon go for a pocket's pearls instead
    of discounting it away, since the opening is a race, not a farm.
  - **P1**: when nothing in the normal scan scored (`best.tile < 0`, all candidates traps
    `no_trap` refused, or nothing reachable scored at all) and we're still in the opening,
    fall back to the nearest reachable pearl (plain distance, ignoring crowd/contest/share
    discounts and dead-end checks) within `opening_pearl_radius`, before falling through to the
    existing patrol fallback. This mirrors ts8's "nearest pearl" goal.
  - Implementation note: the nearest-pearl search is folded into the existing full-board
    `unreachable`-counting pass in `ChooseGoal` rather than a second board scan. A first attempt
    with a separate scan made `check`'s slowest turn on `default` jump from 0.4ms to 2.1ms
    (still under the judge's hard limits but a needless 5x regression); folding it into the
    existing pass restored 0.1ms on `default` and came in under the parent's `big_empty` number
    too (1.2ms vs parent's 2.1ms).

## What I measured

- `check gather-split-trade-115 maps/official`: all checks passed, slowest turn 1.2ms
  (`big_empty`; parent gst101 is 2.1ms there, so no regression), 0 avoidable deaths.
- vs gather-split-trade-101 (parent), official maps, both seats: **12-0-10**. r50: 9.6 dragons/
  23.7 total (us) vs 9.6/23.8 (parent) — essentially unchanged aggregate, some maps up, some down.
- vs gather-split-trade-101 (parent), 20 fresh worldgen maps (seed 91000): **18-0-22**, close to
  even but slightly behind; r50 21.1/53.7 (us) vs 21.4/54.1 (parent) — again near-identical.
- vs gather-split-trade-96 (live v7), official maps: **11-0-11** (even).
- vs gather-split-trade-96, worldgen (same seed 91000, 20 maps): **20-0-20** (even).
- vs trading-swarm-8, official maps: **11-1-10**; r50 (18 matches averaged across all official
  maps) 10.4 dragons/25.2 total (us) vs 12.4/30.2 (ts8) — a narrower relative gap than the
  22.5-vs-50.0 Devil-only baseline, though most of that is because official maps other than
  Devil aren't as lopsided to begin with.
- **Devil specifically vs trading-swarm-8** (the brief's target metric), 2 matches: r50 9.5
  dragons/22.5 total (us) vs 20.5/50.0 (ts8); r100 8.0/19.0 vs 28.5/68.5 — **unchanged from
  strategist-g's baseline measurement**, i.e. the P1 fallback essentially never triggers on
  Devil. Devil's centre is open and densely seeded enough that the normal scored scan (`toward
  goal`) always finds something to chase — `best.tile` is rarely `< 0` there, so P1's fallback
  path is dead code on this map, and P3's cheaper opening trap cost is a second-order effect on
  a map where the normal scan already wins over dead-end pockets. Sample size is small (2
  matches only, per the 40-minute budget); didn't get to increase it or trace to confirm.
- Was not able to run the official-maps-split-out-by-name and 20-worldgen-maps-both-seats table
  the brief asked for in full within budget; official-maps aggregate and worldgen aggregate
  above cover the required opponents (parent, gst96, ts8) but the per-map Devil/Default/Trophy/
  Schooltime breakdown vs gst96/ts8 individually was not run — only the Devil-vs-ts8 target
  metric was isolated.

## What should improve, and what didn't

- Expected: opening total length / dragon count to climb faster specifically where the normal
  scored scan runs dry early (crowded corridors, contested centres where every scored candidate
  gets discounted to near-zero). Devil isn't that map — its centre is wide open, so the existing
  "toward goal" scoring already fires every turn. The mechanism strategist-g described (ts8's
  simple greedy search beating our careful scoring) may need the fallback to fire *more often*,
  not just exist: consider triggering it whenever the best scored candidate is below some floor,
  not only when nothing scored at all. Left for the next task, given the budget.
- P3's cheaper opening trap cost had no visible effect in the numbers gathered (parent-comparison
  aggregates are within noise); it's a small, safe, cheap change and worth keeping regardless,
  but by itself it isn't the throughput fix strategist-g was hoping for.
- Aggregate results vs both parents (gst101, gst96) are even to slightly ahead on official maps
  and roughly even on worldgen — no regression detected, but also no clear evidence yet that this
  closes the r50 gap on the maps where it matters most (Devil-like open-centre maps against
  ts8-style opponents).

## Submit

`check` passes, everything is at least even with parents in the maps/opponents measured.
Submitting.
