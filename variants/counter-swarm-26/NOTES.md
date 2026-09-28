# counter-swarm-26

## Idea

Crossover of counter-swarm-25 (base, namespace `cs25`->`cs26`) and gather-split-trade-49.
gst47/gst49's finding (ported here via gst49's diff against gst45): at r400 most feeders
have no known path to the champion, and most of those have a path through unexplored
ground — the team leaves large parts of the map unseen while it fights over known beds.
gst49 fixed this in its own codebase by, when a dragon sees more than 3 allies (crowded —
beds nearby are probably already being worked), boosting long-unseen frontier tiles
(`explore_value * min(4, rounds_since_seen / 50)`) and folding patrol (toward the
longest-unseen tile) into the same goal scan so it competes with weak bed/pearl candidates
directly, instead of only running as a fallback once nothing else scores at all.

## What changed from the parents, and why

Ported the same diff into counter-swarm-25's `ChooseGoal` (`cs26.cpp`), which already has
the bed/cluster scoring and known-portal crossing (kept unchanged, per the brief — no extra
board pass):

1. Added a `crowd`/`crowded` signal before the main per-tile scan: count of allied heads
   seen this round via `seen_` (excluding self), `crowded = crowd > 3`, same threshold as
   gst49/swh17.
2. In the existing per-tile loop's "unexplored edge" term, when crowded the score is
   `mult * explore_value / (d + 1)` where `mult = clamp(age/50, 1, 4)`, `age` = rounds since
   that tile was last seen (`board_.at(t).seen_round`).
3. In the same loop, when crowded, folded in the patrol score `(round_ - seen_round) /
   (d + 4)` for any tile that's open and not banned, competing via the existing
   `std::max(score, ...)`.

Avoided gst49's own performance trap (their first draft called `Occupied(t)`, which walks
the whole body via `Mine()`, inside the per-tile loop — turned an O(board) scan into
O(board*length) and blew up big_empty's slowest turn 6x): cs26 instead builds a
`mine_mask` (`vector<uint8_t>`, one pass over `body_`) only when `crowded` is true, and
uses a local `occ(t)` lambda backed by that mask plus the existing `board_.occupant`
check — no extra board pass, no per-tile body walk.

## What I measured

- `check counter-swarm-26 maps/official`: all checks passed, 0 avoidable deaths on every
  map. Slowest turn vs parent (`check counter-swarm-25 maps/official`): big_empty 8.6ms
  (parent 3.2ms), devil 3.9ms (parent 0.3ms), all other official maps comparable
  (<=0.4ms). On 20 fresh worldgen maps the picture reverses per-map (the crowded branch
  costs more only on the maps/turns where it actually fires): cs26's worst 5 were
  0.1-21.4ms, parent's worst 5 were 0.1-26.2ms — same order of magnitude, not a
  consistent regression, and `check` passed on every map in both sets.
- `sim --a counter-swarm-26 --b counter-swarm-25 --maps maps/official --threads 2`, both
  seats (22 matches): **13-0-9**, mean rounds 373, +4.2 longest, +19.6 total length at
  game end.
- `sim --a counter-swarm-26 --b counter-swarm-25` on 20 fresh worldgen maps, both seats
  (40 matches): **24-0-16**, mean rounds 410, +3.0 longest, +36.6 total length.

Clears the brief's "at least even" bar on both sets, consistent with gst49's own result
against its parent (also a net-positive W-D-L and length margin from the same mechanism).

## What I'd try next

- gst49's own follow-ups still apply here: instrument (compiled out) the fraction of
  reachable tiles seen at r150/r250/r350 to directly confirm the mechanism rather than
  infer it from W-D-L; profile whether the crowded branch's cost on big_empty/devil can be
  cut further (e.g. gating patrol's extra scoring on top candidates only, like the cluster
  bonus already does, instead of every open tile in the full scan).
- Didn't sweep the `crowd > 3` threshold or `age/50` divisor — ported as literal reads of
  the brief, same as gst49 did. Worth tuning in a follow-up task.
- Didn't have budget to run a `--trace` on any of the losses; worth checking whether they
  cluster on tight/contested maps (Colosseum, arena, default_small, stronghold, trophy as
  B, devil as B) where crowding fires differently than on open maps.
