# gather-split-trade-51

## Idea

gst49 added crowded-midgame exploration: when more than `crowd_threshold` allies are in
view, unseen ground gets a staleness multiplier (`min(stale_cap, age / stale_divisor)`) and
patrol scoring is folded into the main candidate scan instead of only running as a fallback.
gst49's report showed this mechanism winning four lines (34-28..37-25 vs parents, mostly on
worldgen) but the literal constants (crowd > 3, divisor 50, cap 4) were never swept. This
task swept them one at a time against gst49 itself.

## What changed from the parents, and why

Exposed `crowd_threshold` (was hardcoded `3`), `stale_divisor` (was `50.0`), and `stale_cap`
(was `4.0`) as `Params` fields (also wired into the `GST51_TUNE`/`GST51_P` override list, same
pattern as the other tunables) and swept:

- `crowd_threshold`: 2, 5 (default 3)
- `stale_divisor`: 30, 80 (default 50)
- `stale_cap`: 2, 6 (default 4)

Each tested one-at-a-time vs gather-split-trade-49 on `maps/official` (both seats, 22
matches) and one 20-map worldgen set (40 matches, both seats). `crowd_threshold=5` was the
only setting that moved the needle at all; `stale_divisor` and `stale_cap` had **zero**
measurable effect on all three of official/worldgen1/worldgen2 (identical W-D-L and 0.0/0.0
margins in every run) at threshold=3. That's consistent with the default maps (5
dragons/side): "more than 3 allies in view" essentially never fires, so the crowded branch,
and hence the divisor/cap inside it, is rarely exercised at all — raising the threshold to 5
made it fire *less* often, and that (not the divisor/cap) is what changed the outcome.
`crowd_threshold=5` was confirmed on a second, fresh 20-map worldgen set as instructed.

Kept `stale_divisor=50.0` and `stale_cap=4.0` at the gst49 defaults (no measured effect).
Changed `crowd_threshold` from `3` to `5`.

## What was measured

vs gather-split-trade-49, both seats, threads=2:

| setting                | official (22)   | worldgen set 1 (40) | worldgen set 2 (40) |
|-------------------------|------------------|----------------------|----------------------|
| default (3/50/4)        | 11-0-11, margin 0.0/0.0  | 20-0-20, margin 0.0/0.0 | (= default, not rerun) |
| crowd_threshold=2        | 11-0-11, margin 0.0/0.0  | 20-0-20, margin 1.9/-10.2 | not tested |
| **crowd_threshold=5**    | 9-0-13, margin -3.0/14.1 | 27-0-13, margin 7.3/19.8 | 22-0-18, margin 4.2/10.7 |
| stale_divisor=30          | 11-0-11, margin 0.0/0.0  | 20-0-20, margin 0.0/0.0 | not tested |
| stale_divisor=80          | 11-0-11, margin 0.0/0.0  | 20-0-20, margin 0.0/0.0 | not tested |
| stale_cap=2               | 11-0-11, margin 0.0/0.0  | 20-0-20, margin 0.0/0.0 | not tested |
| stale_cap=6               | 11-0-11, margin 0.0/0.0  | 20-0-20, margin 0.0/0.0 | not tested |

`crowd_threshold=5` totals across all three sets: 58-0-44 over 102 games (+14 net), but the
sets disagree in direction: official went backwards (9-13), both worldgen sets went forwards
(27-13, then 22-18 on a fresh set). Two independent worldgen samples agreeing is a real signal
(not the single-set mislead the brief warned about — that showed up here between worldgen1
(+14) and worldgen2 (+4), same direction but different magnitude, which I read as noisy-but-
real rather than a flip), but official maps (5 dragons/side, our smallest, noisiest sample at
22 games) disagreeing is a genuine caveat: this setting reads the game as more worldgen-tuned
than official-map-tuned. Net across everything clears the ~8-wins-over-~100-games bar (+14/102),
so I kept the change, but flag the official-map regression for whoever measures this variant
in the full league (mixed map pool, not just worldgen).

`check gather-split-trade-51 maps/official`: all checks passed, slowest turn well under the
0.5ms/9M-point budget.

## What should improve

Fewer dragons burning turns competing for the same nearby bed/pearl once 5+ (not 4+) allies
are visible, since the crowded-exploration/patrol scoring now waits for a denser pileup before
diverting dragons to stale ground — should show up as slightly better length in crowded
worldgen midgames, at a small risk on sparser official maps.

## What didn't work / surprises

`stale_divisor` and `stale_cap` were fully inert at the default `crowd_threshold=3` in every
map set tried — the crowded branch essentially never triggers with only up to 5 dragons/side,
so its internal constants don't matter until the trigger condition itself is loosened. Anyone
tuning this mechanism further should sweep `crowd_threshold` first and reconsider whether
`stale_divisor`/`stale_cap` are worth carrying at all if the branch stays this rare.
