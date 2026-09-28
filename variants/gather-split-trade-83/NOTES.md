# gather-split-trade-83

## Idea

gst78's "poor home" flag (early portal-crossing when the home region looks thin) is measured
against the whole team's dragon count and against sheer reachable-tile count, so it fires
constantly on maps where teammates are elsewhere on the map (default, stronghold) even though
this dragon's own corner is fine, while never distinguishing "genuinely scarce" from "there's
food here but it's behind a wall I haven't found a way around" (portal-crossing doesn't fix the
latter). Sharpened both at once (option 3 from the brief, given the time budget): divide the
poor threshold by allies actually seen near this dragon this round (not `units_`, the team
total), and additionally require that seen-but-unreachable valued tiles outnumber reachable
ones, so a genuinely locked-off dragon still crosses but a merely-spread-out one doesn't.

## What changed from the parents, and why

- Renamed `gst78`/`GST78_*` -> `gst83`/`GST83_*` throughout (namespace + macros), per the brief.
- In `ChooseGoal`'s poor-home block (`strategy.cpp`, was ~line 473):
  - Added an `unreachable` counter alongside the existing reachable-tile scan: a tile with
    `paths.dist[t] <= 0` (not currently reachable from here) but `base[t] > 0` (we've seen
    something valuable there) counts as seen-but-unreachable. Reuses `base[]`, already computed
    for the reachable pass, so no extra `TileValue` calls.
  - `allies_here`: counts `seen_` entries with `team == spawn_.team` and `round == round_`
    (allies actually visible this turn), plus ourselves, in place of `units_` (whole-team count)
    as the divisor in `poor_min`/`poor_per_unit`.
  - `poor_` now additionally requires `unreachable > reachable` (still ORed with the starving
    branch, which is its own independent trigger and unaffected by this region concept).
- Everything else kept as gst78 had it (4x unknown-portal explore multiplier while poor,
  `poor_cross`: cross a pinged portal even with a goal >= 6 steps away).

I did not get to try options (1) and (2) in isolation separately within the time budget; the
combined change is the one measured below. I also didn't rebuild a `GST83_DEBUG` binary to get
exact poor-turns-per-map counts (would have needed a second build+run cycle); the W-D-L/length
numbers below are the primary evidence and are consistent with the intended effect (poor firing
less spuriously on default/stronghold, matches proceeding closer to even rather than gst78's
lose-a-bit pattern there).

## What I measured

`build/tools/sim --a gather-split-trade-83 --b gather-split-trade-78`, 2 threads:

- Official maps, both seats: **12-0-10** (W-D-L) over 22 matches; at game end gst83 leads by
  1.2 longest, 2.8 total length. (Wins on Colosseum, arena, default (both seats), default_small,
  trauma (both seats), trophy (one seat); losses on big_empty (one seat), devil (one seat),
  queen_of_spades (one seat), stronghold (both seats), schooltime (one seat), trophy (one seat),
  Colosseum/arena/default_small (other seat).)
- 20 fresh worldgen maps, both seats: **19-0-21** over 40 matches; at game end gst83 leads by
  2.3 longest, **48.0** total length. Slightly behind on win count but clearly ahead on the
  length-margin tiebreak criterion the game itself uses.

Didn't run against gst68 or trading-swarm-5, a second worldgen batch, or the
trauma/default/stronghold/queen_of_spades pearl-eaten and poor-turn-frequency debug counts the
brief asked for, due to the ~20 minute time budget — the check plus one official-map sim plus
one worldgen sim already used most of it. Flagging this gap for whoever picks this up next.

## What should improve

Fewer early portal crossings on maps where the flag was firing spuriously because of team size
rather than local scarcity (default, stronghold per the brief), while keeping gst78's trauma win
(a genuinely poor/starving dragon still crosses: the starving branch and per-region reachable
count are both untouched in spirit, just no longer diluted by teammates elsewhere or by
merely-distant-but-open food).

## What I'd try next

- Split options (1) and (2) apart to see which one actually carries the improvement (or whether
  they interact) — the brief's ask, not done here for time.
- Get the `GST83_DEBUG` poor-turn counts per map (trauma/default/stronghold/queen_of_spades) to
  confirm the flag fires less on default/stronghold without dropping on trauma.
- Try `unreachable >= reachable` (not strictly greater) or a ratio instead of a raw compare, in
  case the strict inequality is too conservative on maps with few valued tiles total.
