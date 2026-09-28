# giant-1

## Idea

A few long dragons that never trade, fed by a swarm of short-lived helpers.

- **Roles.** A dragon alive at round 0 is a *giant*. Until round 100 it still splits off
  helpers (it splits in half from length 4), but it never trades and avoids every enemy head.
  After round 100 it never splits by choice. Every split child is a *helper*. Helpers gather,
  split from length 4 until round 300, trade head-to-head like strategy-v1's workers (only
  for an enemy at least as long, or twice as long when outnumbered) and hunt enemy heads
  within 6 steps.
- **The giant's safety.** Every enemy head is a threat. The giant never steps onto a tile an
  enemy head can reach next turn. Within 3 steps of an enemy head it evades (maximises its
  distance) instead of heading for its goal. The room check is a flood fill that knows the
  giant's own tail moves away: segment i counts as free after (length − i) × 1.5 steps,
  since eating stalls the tail. An unexplored edge counts as 4 tiles, not as a sure way out.
  With no roomy step, the giant takes the roomiest step no enemy head can reach. Only when
  there is no such step at all does it split off all but 2 segments: the child walks out
  from the tail and carries on as a giant (children born at length ≥ 8 are giants), and the
  2-segment stub stays behind as bait or a trade.
- **Gossip (sonar, PROTOCOL 3).** Every turn, every dragon pings N/E/S/W with the biggest
  giant it knows of. The 64-bit word holds a magic number, team, id, length, round and head
  x/y. Reports are compared by length − 0.5 × age. Giants report themselves and everybody
  relays.
- **Feeding.** From round 300 a helper walks to the king: to its last reported head, or to
  the reached tile nearest it. It resigns once the king's head is in view within 3 steps. A
  death leaves a pearl on every other segment, and the king eats them.
- **Size classes that sort themselves out.** A giant less than half the king's length steps
  down to a helper that never splits and feeds late. A helper of length ≥ 10 after round 300
  becomes a giant. Until feeding starts, helpers keep off the heads of giants (a length ≥ 6
  in view, or the king), because our own helpers boxing giants in was a main cause of giant
  traps.
- **Portal steps.** When there is a safe step in view, the giant won't step through a portal
  onto a tile it can't see: an out-of-view exit may hold part of its own body it doesn't
  know about. The worldgen `check` caught these deaths.

## What changed from the parents, and why

This is a fresh variant with no parents. It uses `shared/lib/board_memory` (and the
`kUnknown`/`kBlocked` codes). Everything else is new, in namespace `giant`. `GIANT_TUNING`
(commented out) reads parameters from `$GIANT` and prints debugging output. It is for
development only.

## What I measured (native sim, `build/tools/sim`)

Worldgen means 80 maps from `worldgen.py --batch 40`, seeds 1234 and 777, with both sides
played: 160 matches.

Against strategy-v1:

| version | official (22) | worldgen (160) |
|---|---|---|
| pure giant (never splits, no helpers) | 1-0-21 | ≈ 31-49 of 80 |
| + helpers (giant splits until 100) | 7-0-15 | 31-49 of 80 |
| + gossip/feeding, feed from 300, round-0 bug fixed, tuned room check | 10-0-12 | 91-69 |
| + helpers avoid giant heads, promotion at length 10 | 12-0-10 | 98-62 |
| + helpers split until 300 | 14-0-8 | 102-58 |
| **submitted** (+ portal-step guard) | **14-0-8** | **98-62** |

Against the anchors (submitted build): anchor-greedy 21-0-1 official and 160-0 worldgen;
anchor-random 22-0-0 official and 160-0 worldgen.

`check` passes on the official maps and on all 80 worldgen maps. The slowest turn is ≤ 0.4 ms.

Where it wins, at round 499: our longest ≈ 66 vs their ≈ 37. When the giant survives with
helpers around, it wins big. Where it loses at round 499: our longest ≈ 30 vs their ≈ 45.
Usually our helpers lost the attrition war mid-game, and the lone giant couldn't grow while
strategy-v1's swarm fed its leaders. Eliminations are about even (17 won, 12 lost), and the
losses are mostly on the tiny official maps (arena, default_small).

## What should improve

It should beat strategy-v1 on worldgen-like maps (≈ 62%) and be about even or better on
the official maps. It crushes the anchors.

## Tried and didn't work

- **Pure giant** (no helpers): it grows fast early (30+ by round 75), but strategy-v1's swarm
  crowds and kamikazes it. It gets eliminated on small maps, and where it survives it loses
  on longest (strategy-v1 feeds its leaders from round 400).
- **Giant stops splitting at round 1 or 30:** worse than stopping at 100. Stopping at 150
  to 300 was about the same.
- **Giant trades like a leader while still splitting** (strict only after round 100): worse
  (84-76 vs 96-64 worldgen).
- **Heir by sonar only** (a child is a giant only if its giant parent pinged it): far worse
  (62-98). The "pseudo-giants", the long children of long helpers, turned out to be useful
  feeding targets and long dragons.
- **King anchoring** (late, the king keeps its goals near one spot so helpers find it), a
  longer gossip TTL, a larger feed distance, intercepting ahead of the king: no measurable
  change. Feeding stays low (≈ 10 feeds a match): sonar lines stop at kelp and at the first
  dragon hit, so many helpers never learn where the king is.
- **Splitting when the best room is under half of what's needed** (vs only when no
  non-lethal step exists): worse.
- `unknown_room` 1 or 10 (vs 4), feed from 250/350/450, `evade_distance` 2/4/5,
  `outnumbered_factor` 0: all within noise (±5 of 160) or worse.

## Surprises

- Almost all giant "trapped" splits come from heirs (children of trapped splits). An heir
  doesn't know its own body beyond the 7×7 window, so the flood fill overestimates room, and
  the traps chain.
- Our own helpers are the main thing boxing a giant in: about half the traps have an ally
  segment next to the head, mostly after round 300.
- A split child acting in round 0 also sees `round == 0`. "Alive at round 0" is only a
  reliable original-dragon test if nothing splits in round 0.

## Next

- Send the body path to the heir over sonar (the stub can ping it on the split turn).
- Make gossip reach further (rendezvous points, relaying with direction hints).
- Improve the helpers' economy (strategy-v1's dead-end evaluation), since the swarm war
  decides most of the losses.
