# gather-split-trade-68

## Idea

Crossover of two siblings that both descend from gather-split-trade-56, merging the parts of
each that don't overlap: gst62's champion dead-end skip and gst63's early centre pull. Both
changes touch `ChooseGoal` but at different points (the dead-end filter vs. the per-tile
score), so they compose without interaction.

## What changed from the parents, and why

Base is gst63's sources (namespace/macros renamed `gst63` -> `gst68` throughout), which already
carries the centre-pull change (`centre_max_tiles`/`centre_until`/`centre_bonus` scaling
gather/edge scores near the map centre in the opening on small maps). Diffed gst62 against
gst56 to isolate gst62's *own* change (everything else in that diff was gst62's namespace/macro
rename, identical to gst63's): a new `champ_no_trap` param (default 1) and one extra OR clause
in `ChooseGoal`'s `no_trap` guard:

```cpp
const bool no_trap = (p_.cap_safety && length_ >= 4 && !CanCountOnSplit()) ||
                     (p_.champ_no_trap && round_ >= p_.champ_freeze && Champion().id == spawn_.id);
```

Applied that clause onto gst68 (gst63-based) verbatim, plus the param and its `GST68_SET` tuning
entry. `champ_freeze` and `Champion()` already existed in gst63 (inherited from gst56), so no
other plumbing was needed. The two changes don't interact: the centre pull only changes which
*candidate tiles* get scored highly during the opening (round < 50, small maps); the
`champ_no_trap` clause only changes whether a *dead-end* candidate is skipped, and only for the
frozen champion late in the game (round >= 330). Different rounds, different mechanisms.

## What I measured

`check gather-split-trade-68 maps/official`: all checks passed, 0 avoidable deaths (the usual
slow-turn spikes on big_empty/stronghold/trauma under check's instrumentation match the parents'
own check runs on this machine, unrelated to this change).

Sim, both seats, W-D-L (longest/total margin at match end where noted):

| opponent | maps | W-D-L |
|---|---|---|
| gather-split-trade-62 | official 11 | 11-0-11 (longest +1.2, total +13.2) |
| gather-split-trade-62 | worldgen batch 1 (20 maps, seed 101) | 22-0-18 |
| gather-split-trade-62 | worldgen batch 2 (20 maps, seed 202) | 21-0-19 |
| gather-split-trade-63 | official 11 | 12-0-10 (longest -1.7, total +1.6) |
| gather-split-trade-63 | worldgen batch 1 | 20-0-20 |
| gather-split-trade-63 | worldgen batch 2 | 19-0-21 |
| trading-swarm-5 | official 11 | 14-1-7 |
| trading-swarm-5 | worldgen batch 1 | 29-0-11 |

Against both parents this lands close to even (W-D-L within a couple matches either way across
44+ matches per pairing on official+worldgen combined), with a slight edge against gst62 and a
roughly even edge against gst63 (which itself was already close to even against its own parent
gst56). Against trading-swarm-5 the win rate stays strongly ahead, consistent with both parents'
own reports. No regression in death causes or avoidable deaths turned up in the action/fatal-turn
breakdowns.

## What I'd try next

- A worldgen batch specifically on small maps (<=1024 tiles, where the centre pull is active)
  crossed with late-game states (round >= 330, where champ_no_trap is active) to see the two
  features actually co-occur in the same match rather than just confirming they don't clash on
  average.
- Didn't have time in the 20-minute budget to trace a loss; the close-to-even record against
  both parents suggests neither change is being undermined by the other, but a `--trace` on one
  of the losses to gst63 would confirm nothing subtle broke.
