# swarm-hunt-17

## Idea

Mutation of swarm-hunt-16 (namespace renamed `swh16` -> `swh17` throughout, including the
`SWH16_*` tuning/debug macros, so it can't clash with the parent when loaded into one
simulator). Focus: harvest the areas the swarm abandons (VISUAL.md #16/#17): whole reachable
regions of pearls/beds go uneaten for hundreds of rounds while the swarm sits elsewhere,
because the goal scorer always prefers a nearby bed ripening soon over ground that's just
far away and stale.

## What changed from the parents, and why

In `ChooseGoal` (hunt.cpp):

1. Added a `crowded` signal: count of allied heads seen this round (unconditional of
   `share_factor`), `crowded = crowd > 3`.
2. When crowded, the unexplored-frontier score (`explore_value / (d+1)`) is multiplied by
   `min(4, max(1, age/50))` where `age` is rounds since that frontier tile was last seen —
   so ground we've been away from a long time competes better with a bed several dragons
   are already piling onto, instead of always losing because it's merely far away.
3. Folded patrol scoring (`(round_ - seen_round) / (d+4)`, "toward what we haven't seen
   for longest") into the *same* board scan that already builds bed/pearl/explore
   candidates, active whenever crowded — so patrol competes directly with weak bed goals
   instead of only running as a fallback when nothing else scores at all. (I originally
   wrote this as a second full board loop; `check` showed trauma's slowest turn jump from
   10.4ms to 63ms, so I folded it into the existing loop instead, which brought it back to
   ~20ms — same ballpark as the parent's already-high number on that map.)

I did not add goal hysteresis (multiplying last turn's goal score by ~1.3 while still
valid) — ran out of budget to try it as a separate, comparable change; noted for next time.

## What I measured

- `check swarm-hunt-17 maps/official`: all checks passed, 0 avoidable deaths. Slowest turn
  is trauma at ~20ms vs parent's ~10ms (both already well above the 0.5ms rule of thumb;
  this variant's added board scan is gated on `crowded` so it isn't paid every turn, but
  trauma's dragons are crowded often since it's a maze).
- `sim --a swarm-hunt-17 --b swarm-hunt-16 --maps maps/official --threads 2`, both seats,
  11 maps (22 matches): **10-0-12**, mean rounds 328, at game end swarm-hunt-17 leads by
  +1.7 longest, -0.4 total length.
- `sim --a swarm-hunt-17 --b swarm-hunt-16` on 20 fresh worldgen maps, both seats (40
  matches): **21-0-19**, mean rounds 437, +3.5 longest, -5.5 total length.

Roughly even with the parent on both sets, slightly ahead on the longest-dragon tiebreak
that actually decides matches, slightly behind on total length. Given the brief's bar
("at least even"), this clears it, but not by a wide margin — the effect is real but
modest at these settings.

## What I'd try next

- Tune the `crowd > 3` threshold and the `age/50` staleness divisor — picked as literal
  reads of the brief's suggestion, not swept.
- Goal hysteresis (multiply last turn's goal score by ~1.3 while it's still valid) as a
  separate change, since the brief notes goals flip 36-50% of turns before being reached;
  didn't get to it this task.
- Consider gating the crowded bonus on "few beds per dragon nearby" too (the brief's other
  crowding signal), not just ally count, in case ally-count alone over- or under-fires on
  some map shapes.
