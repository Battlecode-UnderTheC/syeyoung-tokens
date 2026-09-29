# counter-swarm-38

## Idea

Tweak of counter-swarm-37 (cs37 -> cs38 namespace/macro rename only, no other change beyond
what's below), applying strategist-a's two proposals from
`strategy/strategist-a.md` against the 7-18 loss record (M547322/M561834/M572141/M578650):

1. **P1 — sweep the champion-consolidation rounds earlier.** cs37 only starts feeding the
   champion and freezing its own splits at round 380/330; strategist-a's diagnosis (backed by
   M578651: we had more dragons and total length but lost on longest, 41 vs 47) is that this
   starts too late to win the round-limit longest-dragon comparison.
2. **P2 — skip routine growth splits when locally outnumbered.** Added a 7x7-window density
   check (`LocallyOutnumbered()`): a dragon skips a routine growth split (not a trapped/escape
   split) when it currently sees >= 3 enemy heads and <= 1 ally head within Torus distance 3.
   Targets the teamEliminated losses (M561837: opponent count 6->40 while ours goes 11->0):
   splitting into a fight we can't win locally just donates more heads to trade against.

## What changed from the parents, and why

- `cs38.hpp`/`cs38.cpp`: renamed from `cs37.*`, all `cs37`/`CS37` -> `cs38`/`CS38`.
- New params in `Params` (cs38.hpp): `local_outnumber_min_enemies = 3`,
  `local_outnumber_max_allies = 1` (0 disables the check, for reference).
- New method `LocallyOutnumbered()` (cs38.cpp, after `EnemiesSeen()`): counts enemy/ally heads
  seen *this turn* within Torus distance 3 (a 7x7 window) using the existing `seen_` sightings
  map. Wired into the growth-split condition (`Decide()`, step 4) as an extra `&&` guard.
- `feed_from_round`: 380 -> 340, `champ_freeze`: 330 -> 320, `leader_feed_from`: 380 -> 340.
  **Not** swept all the way to 300 as strategist-a's P1 first suggested (matching
  `leader_split_until=300`): a full 380->300 sweep measured *worse* on official maps than this
  smaller 40-round sweep (see measurements) -- feeding/freezing that early apparently starts
  giving up growth time before the team has enough length/numbers to make the trade-off pay,
  so I picked the smaller, still-net-positive-on-official sweep instead. Flagging this for the
  next task: a finer sweep between 300 and 380 (e.g. 350, 360) is worth trying rather than
  re-doing the full 380->300 jump.

## What I measured

All sims: `build/tools/sim`, official = `maps/official` (11 maps x 2 seats = 22 games),
worldgen = 20 fresh maps seed 5000 (`worldgen.py --batch 20 --seed 5000`), 40 games (2 seats).

**vs parent (counter-swarm-37), sanity check (the required "check it is sound" step):**
- official: **13-0-9** (W-D-L)
- worldgen (seed 5000): **18-0-22**
- combined: 31-31, roughly even -- meets the "at least even with parent" submit bar.
- I tried three configs before settling: P1-full(300)+P2 gave 8-14/21-19; P1-full(300) alone
  gave 9-13/19-21; P1-moderate(340)+P2 (this variant) gave 13-9/18-22, the best official
  result of the three and roughly the same combined total. P2 alone (P1 untouched) gave
  10-12/20-20. None of P1 alone, P2 alone, or the full P1 sweep clearly beat the parent on
  both map sets at once; the moderate P1 + P2 combination was the best of what I tried within
  budget.

**vs the three named opponents, official maps (task's headline ask):**
- vs gather-split-trade-96 (live v7): **11-0-11** — even. Leads by 6.9 longest / 16.7 total at
  the end on average; wins concentrated on Colosseum/stronghold/default_small/devil(B)/
  schooltime(B)/trophy(B), losses on arena(A)/big_empty/default/devil(A)/queen_of_spades(B)/
  schooltime(A)/trophy(A) -- still seat-dependent, not a clean map-type split.
- vs trading-swarm-5: **16-0-6** — clear win.
- vs trading-swarm-2: **16-1-5** — clear win.
- trading-swarm-8 (the opponent clone, task g20-1) was not tagged yet when I measured
  (checked `variants/`; not present) -- not included.

**vs gather-split-trade-96, worldgen (seed 5000, 40 games):** **14-0-26** — a clear loss, the
one result that gives me pause. Combined with the official 11-0-11 that's 25-37 overall
against the live opponent across both sets, worse than cs38's even record against its own
parent. I did not have budget left to dig into *why* (trace a worldgen loss) -- flagging this
as the top thing for the next task to check before trusting cs38 too far against gst96
specifically; it's possible the P1 sweep interacts worse with gst96's own timing than with
cs37's.

**Per-map dragon count / longest / pearls eaten (r50/r100), vs the real-opponent replay
numbers the brief asked for:** I did not get to instrument this inside the 30-minute budget --
`sim`'s summary output gives end-of-game longest/total, not round-50/100 snapshots or a pearl
counter, and adding that instrumentation (or a `--trace`-based script to extract it) would have
eaten most of the remaining time. Flagging as unfinished: the next task should either add a
lightweight round-N snapshot dump to `sim`/`check`, or budget a `--trace` pass explicitly for
this comparison.

## What should improve

- Fewer teamEliminated-style losses on crowded maps (P2): dragons hold their length together
  instead of feeding a numbers fight they're already losing.
- Better longest-dragon outcomes at round-limit endings (P1): the champion gets ~40 more
  rounds of dedicated feeding/no-more-splitting than cs37's 380/330, without going all the way
  to 300 (which measured worse).
- Clear improvement vs both trading-swarm-5 and trading-swarm-2; roughly even vs
  gather-split-trade-96 on official maps but behind it on worldgen -- see the flag above.

## What I tried that didn't work

- Full P1 sweep to round 300 (matching `leader_split_until`, as strategist-a's P1 literally
  proposed): measured worse than the smaller 340/320 sweep on official maps, in both P2-on and
  P2-off configurations. Backed off to 340/320.
- P2 in isolation (local-outnumber split-skip, P1 left at cs37's defaults): roughly neutral to
  parent (10-12 official, 20-20 worldgen) -- weaker than the combined P1(340)+P2 result.

## Submission

`build/tools/check counter-swarm-38 maps/official` passes ("all checks passed"). Ready to
submit: at least even with parent counter-swarm-37 in my measurements (31-31 combined across
official+worldgen), with clear wins over trading-swarm-5 and trading-swarm-2 and an even
record vs gather-split-trade-96 on official maps (though behind it on worldgen -- flagged
above for follow-up).
