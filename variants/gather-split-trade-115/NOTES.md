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

## Round 2 (orchestrator feedback)

Round 1's P1 fallback only fired when `best.tile < 0` (nothing scored at all), which never
happens on Devil's open, densely-seeded centre -- the orchestrator's measurement showed the
Devil-vs-ts8 r50/r100 numbers exactly unchanged from baseline, confirming the fallback was dead
code there.

Changed the trigger: added `opening_pearl_factor` (1.0) and now prefer the nearest reachable
pearl whenever `round_ < opening_end` and either nothing scored, **or** the best scored
candidate's score is weaker than the pearl's own plain (undiscounted -- no crowd/share/contest/
cluster multipliers, matching how ts8's greedy search works) score times `opening_pearl_factor`.
The nearest-pearl distance/score are still tracked inside the existing full-board pass (no
second scan; `check` stayed at 0.1-0.2ms slowest turn everywhere on official maps).

### Devil vs trading-swarm-8 (target metric)

Note: Devil is a single fixed official map and `sim` turned out to be fully deterministic here
(reran the same 2-match batch twice, byte-identical results both times) -- there is no seed
knob exposed by `sim`, and `worldgen.py` has no "devil" style/landmark to generate seeded
variants of it from. So "two seeds" wasn't achievable for Devil specifically; reporting the
only reproducible measurement (2 matches, both seats):

- r50: gst115 9.0 dragons / 20.5 total vs ts8 18.5 dragons / 47.5 total
  (baseline was 9.5 / 22.5 vs 20.5 / 50.0)
- r100: gst115 11.5 dragons / 29.0 total vs ts8 27.0 dragons / 65.5 total
  (baseline was 8.0 / 19.0 vs 28.5 / 68.5)
- Still 0-0-2 (loss both seats). r50 gap is essentially unchanged (still ~2.3x); r100 total-length
  gap narrows a bit (19.0->29.0 for us) but dragon count still falls far behind ts8's.
- ts8's `nearest pearl` (6708 calls/match) still fires ~1.8x more than our `toward goal`
  (3679/match) -- the fallback is no longer dead code (its own action isn't separately tallied,
  it reuses "toward goal"/"into a dead end worth it" reasons downstream), but it isn't enough to
  flip Devil: the deeper issue -- congestion/death-mode asymmetry (strategist-f) and Devil's map
  shape favoring ts8's centre-push timing -- is untouched by P1/P3 alone.

### Aggregate W-D-L (all measured with the round-2 change)

- vs gather-split-trade-101 (parent): official **15-0-7** (up from 12-0-10), r50 11.3/28.5 (us)
  vs 9.2/22.8 (parent); worldgen (seed 91000, 20 maps) **22-0-18** (up from 18-0-22), r50
  20.9/54.4 vs 22.3/57.3.
- vs gather-split-trade-96 (live v7): official **13-0-9** (up from 11-0-11); worldgen **23-0-17**
  (up from 20-0-20).
- vs trading-swarm-8: official **16-1-5** (up from 11-1-10), r50 10.7/26.5 (us) vs 11.2/27.3;
  worldgen **37-0-3** (up from even, a big jump) -- r50 22.5/58.5 (us) vs 16.3/41.9, r100
  41.6/109.0 vs 22.3/56.1. The score-comparison trigger clearly helps broadly across official
  and especially worldgen maps against all three opponents, even though it doesn't close the
  Devil-specific gap yet.

### Takeaway

The "prefer nearest pearl over a weak scored candidate" trigger (not just "nothing scored")
is a much bigger lever than round 1's version, and it's a clean win against parents and ts8 on
official and worldgen maps broadly. But Devil is the one map type where the original diagnosis
was made, and it remains close to unchanged there: ts8 still out-throughputs us by roughly the
same margin. Devil's issue looks less like "our scan misses pearls" and more like a structural
throughput/congestion gap (P2's sonar hotspot broadcast, or strategist-f's congestion proposals)
that P1/P3 alone can't fix. Next step: check whether our per-tile discounts (share/crowd/contest)
are suppressing scores across Devil's whole (open, many-allies-in-view) board, not just at a few
contested tiles -- `opening_pearl_factor` may need to be higher than 1.0, or those discounts
themselves need a lighter touch in the opening, specifically on open-centre maps like Devil.

## Submit

`check` passes ("all checks passed", 0 avoidable deaths). Round 2 measurements show a clear
improvement vs all three opponents on official and worldgen maps (parent 15-0-7 / 22-0-18, gst96
13-0-9 / 23-0-17, ts8 16-1-5 / 37-0-3) -- well above "at least even". Devil-vs-ts8 specifically
is essentially unchanged (still a loss, still ~2.3x behind at r50). Submitting.
