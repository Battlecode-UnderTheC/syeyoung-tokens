# swarm-hunt-7

## Idea

swarm-hunt-6 added cap feeding: a hunter of length <= 3, before round 340, at the unit cap
(64), that sees a longer ally's head within 2 steps resigns next to it, freeing a slot for a
productive split. But swarm-hunt-6's normal splits stop 6 slots below the hard cap
(`units_ < spawn_.unit_limit - unit_reserve`, i.e. it self-limits to 58), so the line almost
never actually reaches 64 and cap feeding rarely triggers. swarm-hunt-7 makes cap feeding
trigger at the line's own effective split limit instead of the hard cap, so it can actually
fire when the roster is full in practice.

## What changed from the parents, and why

One line in `hunt.cpp`, the cap-feed guard:

```cpp
// before (swarm-hunt-6): fires only at the hard unit_limit (64), which the line's own
// splits never reach because of unit_reserve (6)
if (!keeper_ && length_ <= p_.cap_feed_max_length && units_ >= spawn_.unit_limit && round_ < p_.cap_feed_until) {

// after (swarm-hunt-7): fires at the line's own split ceiling (unit_limit - unit_reserve, 58)
if (!keeper_ && length_ <= p_.cap_feed_max_length && units_ >= spawn_.unit_limit - p_.unit_reserve &&
    round_ < p_.cap_feed_until) {
```

Nothing else changed. Namespace renamed `swh6` -> `swh7` (including the `MirrorAgent` type in
`plugin.cpp`) so it can be loaded alongside swarm-hunt-6 in the simulator.

## What should improve

Cap feeding should fire far more often, since the line reaches its own split ceiling
routinely instead of needing to hit the hard 64 cap (which normal splitting logic actively
avoids). That should let more short/expendable hunters convert into feed events for longer
allies earlier in the game, improving swarm-hunt-7's split throughput and average length vs
swarm-hunt-6.

## Measured

- Build: `swarm-hunt-7`, `sim`, `check` all built clean. `build/tools/check swarm-hunt-7 maps/official` -> "all checks passed" (0 avoidable deaths on all 11 official maps).
- Official maps (`maps/official`, both sides, 22 matches): **9-4-9 (W-D-L)** vs swarm-hunt-6.
  At the end, swarm-hunt-7 leads by 0.7 longest, 18.2 total (mean). Cap feeding fired 14 times
  across 22 matches (swarm-hunt-6: 0 times in this run) — roughly even record, clearly more
  cap-feed events.
- 20 fresh worldgen maps (`maps/worldgen/worldgen.py --batch 20`, both sides, 40 matches):
  **22-0-18 (W-D-L)** vs swarm-hunt-6. swarm-hunt-7 leads by 1.4 longest, 14.2 total (mean).
  Cap feeding fired **182 times** across 40 matches (4.6/match) vs swarm-hunt-6's **1 time**
  across 40 matches — confirms the diagnosis: the parent's own split limit almost never let it
  reach the hard cap, so cap feeding was nearly dead code in practice.
- No avoidable deaths, no regression in death causes (wall/self/body/head-to-head counts are
  in line with the parent; "no action" (resign, incl. cap feeds) rose as expected since cap
  feeding now fires much more).

## What I'd try next

- The win margin on official maps is thin (9-4-9); worldgen shows a clearer edge (22-0-18).
  Worth checking whether `cap_feed_max_length` (currently 3) or `unit_reserve` (6) should
  themselves be retuned now that cap feeding is live at the right threshold — a future tweak
  could try loosening `cap_feed_max_length` slightly now that the trigger actually fires.
- Did not try changing `unit_reserve` itself or the feed geometry (2-step lookahead) — kept
  the change to the one guard the brief asked for.
