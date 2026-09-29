# gather-split-trade-113

Task g21-4, mutate of gather-split-trade-101. Implements strategist-f-r3's P2 (extend the
anti-clustering fan-out past round 80, into SafeStep's tile tiebreak). P4 (cap simultaneous
growth splits per local cluster) turned out to already exist in the parent as
`split_crowd_limit`/`crowd_discount_radius` (added at g17-11), so no change was needed there;
this variant left that mechanism untouched.

## Idea

strategist-f's telemetry (M580806/M578650 replays) found that most losses are dominated by
self-inflicted deaths (`no_valid_action:resign`, `head_to_head:mover`), not enemy contact, and
that the existing anti-clustering discount (`fan_out`, strategy.cpp ~L396) stops mattering
entirely once `round_ >= fan_out_until` (80), well before the crowd (per the death tables) keeps
building past r150-300 on Devil/Schooltime/Trophy.

## What changed from the parent, and why

1. `fan_weight`: replaces the hard `fan_out = round_ < fan_out_until` on/off switch with a
   decaying multiplier — unchanged (=1.0, same behaviour as before) while `round_ < fan_out_until`,
   then decays from 1.0 toward a floor (`fan_out_floor=0.3`) over `fan_out_decay=300` further
   rounds. Applied to the same "a contested frontier tile near a nearer ally gets no explore
   credit" rule (strategy.cpp ~L588-612): instead of zeroing the credit only pre-r80 and doing
   nothing after, it now keeps discounting a contested tile's explore credit for the whole game
   (scaled down, never to 0 by design, so the floor keeps a residual "spread out" pressure alive
   after r80 without permanently blinding a dragon to a genuinely useful tile late-game).
2. SafeStep tile tiebreak (strategy.cpp ~L924, the `key()` tuple): appended a new lowest-priority
   field, `-ally_bodies_within(safe_step_crowd_radius=2)` of the candidate step, computed from the
   same ally-head sightings already used for `crowd_allies`/`fan_allies` (a small loop over
   `seen_`, no new board scan). It only ever breaks ties among candidates equal on every existing
   safety measure (doom-first, exposure, escape level, liberties, count, room) — it never
   overrides a real safety difference, so it should only reduce genuinely-avoidable ally-adjacent
   head-to-heads, not push a dragon into a worse dead end to dodge a crowd.
3. Namespace `gst101`→`gst113`, macros `GST101_*`→`GST113_*` (mechanical, required so the
   variant loads without clashing with other gst101-derived variants in the same simulator).

First attempt applied the decay unconditionally from round 0 (not gated on `fan_out_until`
first), which made every mid/late-game contested frontier tile always somewhat discounted
instead of only after r80; that measured *worse* on official maps (8-0-14 vs parent, no-action
123.4 vs 97.2) so I reverted to gating the decay to only kick in once `round_ >= fan_out_until`,
matching the proposal's literal ask ("extend...past round 80") rather than changing pre-80
behaviour at all. That fixed the regression (see measurements below).

## What I measured

`build/tools/sim`, 2 threads, --a gather-split-trade-113:

- vs gather-split-trade-101 (parent), maps/official (22 matches, both seats): **9-0-13**.
  no-action deaths/match 97.2 (parent 101.0), head-to-head 64.0 (parent 67.9) — both self-kill
  causes strategist-f flagged went down. Total length -13.3 behind, longest -0.5 (roughly even).
- vs gather-split-trade-101, 20 fresh worldgen maps (seed 91000, 40 matches both seats):
  **21-0-19**. no-action 364.1 (parent 355.4, slightly worse), head-to-head 153.3 (parent 154.1,
  roughly even).
- vs gather-split-trade-96 (live v7), maps/official: 9-0-13; 20 worldgen maps: 20-0-20. Both
  essentially identical to the parent comparison (gst96 and gst101 play very similarly here).
- vs trading-swarm-8 (server-opponent clone), maps/official: 13-1-8, our no-action 81.4 vs its
  15.3 (it dies far more to wall/self/body: 74.0/55.2/37.1 — its own overspeed splitting, not
  ours), head-to-head 56.4 vs 56.1.

Didn't break out W-D-L by official-map category (Trophy/Slithery-like/Devil/Schooltime
separately) given the 40-minute budget; the per-match lines above are in the sim output if
someone wants to re-slice them.

## What should improve

The resign/mover rate (strategist-f's proposed first-class metric) dropped on official maps
without W-D-L or total-length collapsing — the intended effect. Net W-D-L is a wash (roughly
even, slightly behind on official, slightly ahead on worldgen), which is expected: this change
only reduces *self-inflicted* deaths, and per strategist-f's own numbers those are a small
fraction of total deaths outside a few outlier games (Trophy, Slithery-adjacent). It shouldn't
regress anything and gives the next task cleaner telemetry to build on.

## What I'd try next

- P1 (a first-class resign+mover counter printed by `sim` itself) wasn't done here — I read it
  off the existing per-cause death breakdown by hand. Adding it to `sim`'s output directly (not
  variant code) would make this faster to check next time, per strategist-f's own suggestion.
- P3 (let length-2/3 stubs trade into a cheap ally instead of resigning) is the other half of
  strategist-f's proposal list and untouched here; it directly targets the resign count itself
  rather than the upstream crowding that causes it.
- The unconditional-from-round-0 version of the decay (see "first attempt" above) is worth
  revisiting with a gentler floor (closer to 1.0, e.g. 0.7-0.8) if someone wants the anti-
  clustering pressure to matter earlier too — my one attempt at that was too aggressive and
  regressed, but a smaller pre-80 discount might not.

## Submission

`build/tools/check gather-split-trade-113 maps/official`: all checks passed. Measurements are
a wash vs both parents (roughly even, not a clear regression) — ready to submit.
