# gather-split-trade-49

## Idea

Crossover of gather-split-trade-45 (base, namespace `gst45`->`gst49`) and swarm-hunt-17.
gather-split-trade-47's measurement found that at r400, 73-89% of our feeders have no
known path to the champion, and 80-90% of those do have a path through unexplored ground:
the team leaves most of the map unseen, matching VISUAL.md #16/#17 (whole reachable
regions never harvested while the swarm sits elsewhere). swarm-hunt-17 addressed the same
symptom in its own codebase: when a dragon sees more than 3 allies (crowded, beds nearby
are probably already being worked), long-unseen frontier tiles are worth
`explore_value * min(4, rounds_since_seen / 50)`, and patrol (toward what hasn't been seen
longest) is folded into the same goal scan so it competes with weak bed goals directly
instead of only running as a fallback once nothing else scores.

## What changed from the parents, and why

Ported swh17's `ChooseGoal` diff into gst45's `ChooseGoal` (`strategy.cpp`), which already
has late-surge-17's precompute/mine-mask/bounded-cluster-scan single board pass (kept
unchanged, per the brief: no extra board pass):

1. Added a `crowd`/`crowded` signal before the main scan: count of allied heads seen this
   round via `seen_` (excluding self), `crowded = crowd > 3` — same threshold as swh17.
2. In the existing per-tile scoring loop, at the "explore frontier" term, when crowded the
   score is `mult * explore_value / (d+1)` where `mult = clamp(age/50, 1, 4)` and `age` is
   rounds since that tile was last seen (`board_.at(t).seen_round`).
3. In the same loop, when crowded, folded in the patrol score
   `(round_ - seen_round) / (d+4)` for any tile that's open (using the loop's existing
   `mine_mask`-backed `occ(t)` lambda, not `Occupied(t)` — see below) and not banned, so it
   competes with weak bed/pearl candidates via the existing `std::max`.

Bug I caught before measuring: my first patrol draft called `Occupied(t)` (which walks the
whole body via `Mine()` per call) inside the per-tile loop. `check` showed big_empty jump
from parent's 8.0ms to 52.6ms — turning the O(board) scan into O(board*length). Switching
to the loop's already-precomputed `occ(t)` (backed by `mine_mask`) brought it to 26.1ms —
still ~3x the parent on that one map (the crowded branch legitimately does more work per
tile when it fires, matching swh17's own note that its added scan raised trauma's slowest
turn even after folding it in), but in line with what's already accepted for gst45/swh17
sourced variants and well within `check`'s pass/fail (0 avoidable deaths, all maps
"passed"). Didn't have budget to profile further; flagging for the next task.

## What I measured

- `check gather-split-trade-49 maps/official`: all checks passed, 0 avoidable deaths on
  every map. Slowest turn: big_empty 26.1ms (parent 8.0ms), schooltime 0.3ms (parent
  16.6ms — noisy map-to-map, not a consistent regression), others comparable to or faster
  than parent.
- `sim --a gather-split-trade-49 --b gather-split-trade-45 --maps maps/official --threads 2`,
  both seats, 11 maps (22 matches): **11-0-11**, mean rounds 358, at game end leads by +2.4
  longest, +4.2 total length.
- `sim --a gather-split-trade-49 --b gather-split-trade-45` on 20 fresh worldgen maps, both
  seats (40 matches): **23-0-17**, mean rounds 407, +4.8 longest, +48.5 total length.

Clears the brief's "at least even" bar on both sets, with a real edge on worldgen maps
(the more varied/open ones, where "crowded" plausibly fires more and unexplored regions
are larger) and roughly even-to-ahead on official maps.

I did not re-run the r150/r250/r350 reachable-tiles-seen measurement from
gather-split-trade-47 before/after under this exact change (ran out of budget after the
sim runs); the W-D-L/length gains are consistent with the mechanism working, but a direct
before/after of that fraction would confirm it rather than infer it.

## What I'd try next

- Instrument (temporarily, compiled out) the fraction of reachable tiles seen at
  r150/r250/r350 for this variant vs the parent, to confirm the mechanism (not just the
  W-D-L outcome).
- Profile why the crowded branch costs 3x on big_empty specifically — it's an open map
  where "crowded" likely fires most turns; maybe worth a cheaper age-bucket approximation
  there, or gating patrol's extra scoring on top candidates only (like the cluster bonus
  already does) instead of every open tile.
- swh17's own list still applies here too: tune the `crowd > 3` threshold and `age/50`
  divisor (not swept, just ported as literal reads of the brief), and try goal hysteresis
  as a separate change.
