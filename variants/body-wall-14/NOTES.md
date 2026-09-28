# body-wall-14

## Idea

Crossover of body-wall-13 (base, namespace `bw13`->`bw14`) and gather-split-trade-49.
gst47 found that at r400, 73-89% of our feeders have no known path to the champion, and
80-90% of those do have a path through unexplored ground: most of the map is left unseen.
gst49 addressed this by giving long-unseen frontier tiles a bonus when a dragon sees more
than 3 allies (crowded, beds nearby are probably already worked) and folding patrol
(toward the longest-unseen tile) into the same goal scan so it competes with weak bed
goals directly. This ports that exact `ChooseGoal` change onto body-wall-13, which keeps
its own bed/cluster scoring, known-portal crossing and enemy-head warnings unchanged.

## What changed from the parents, and why

In `ChooseGoal` (`strategy.cpp`), before the main per-tile scan:

1. Added a `crowd`/`crowded` signal: count of allied heads seen this round via `seen_`
   (excluding self), `crowded = crowd > 3` (same threshold as gst49/swh17).
2. Added a `mine_mask` (built once from `body_`) and an `occ(t)` lambda backed by it, so
   the crowded-only patrol term below doesn't call `Occupied(t)` (which walks the whole
   body via `Mine()`, O(length) per call) inside the O(board) scan — gst49 hit exactly this
   trap and had to fix it before measuring; ported the fix straight in rather than
   reproducing the bug.
3. In the existing frontier-explore term of the per-tile loop, when crowded the score is
   `mult * explore_value / (d+1)` where `mult = clamp(age/50, 1, 4)`, `age` = rounds since
   that tile was last seen.
4. In the same loop, when crowded, folded in the patrol score
   `(round_ - seen_round) / (d+4)` for any tile that's open (`occ(t)`) and not banned, so
   it competes with weak bed/pearl candidates via the existing `std::max`.

No extra board pass was added — both terms are evaluated inside body-wall-13's existing
single scan loop (which body-wall-13 already ran as one pass, unlike gst45/49's separate
cluster-pool prepass; nothing else in that structure was touched).

## What I measured

- `check body-wall-14 maps/official`: all checks passed, 0 avoidable deaths on every map.
  Turn timings were noisy from machine load during this run (the tool uses wall-clock, not
  CPU points) — one run showed stronghold at 12-27ms, but re-running `check` on the
  unchanged parent (body-wall-13) in the same conditions showed the same swings (1.5ms to
  42.6ms on repeated runs of the *parent*), confirming it's system noise, not a regression
  from this change. A quiet run put every map back under 2ms except schooltime at 11.8ms,
  again matched by similar noise on other maps for both variants.
- `sim --a body-wall-14 --b body-wall-13 --maps maps/official --threads 2`, both seats, 11
  maps (22 matches): **10-0-12**, mean rounds 338, +0.4 longest, +0.0 total at game end —
  even with the parent on official maps.
- `sim --a body-wall-14 --b body-wall-13` on 20 fresh worldgen maps, both seats (40
  matches): **26-0-14**, mean rounds 388, +5.5 longest, +31.0 total — a real edge on
  worldgen (larger/more varied maps where crowding and unexplored regions are bigger),
  matching gst49's own finding that this change helps more off the official map set.

Clears the brief's "at least even" bar: even on official maps, ahead on worldgen.

## What I'd try next

- gst49's own follow-ups still apply here: tune the `crowd > 3` threshold and `age/50`
  divisor (ported as literal reads of gst49's params, not swept), and confirm the
  mechanism directly (fraction of reachable tiles seen at r150/r250/r350, before/after)
  rather than inferring it from W-D-L.
- body-wall-13's own wall/squeeze logic wasn't touched; worth checking whether waller
  dragons ever end up patrolling into a spot better left to a feeder, now that patrol
  competes for goals more aggressively when crowded.
