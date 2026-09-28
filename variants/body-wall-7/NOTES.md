# body-wall-7

## Idea

body-wall-6 (wall squeeze, cap wallers, gst23-derived economy) with gather-split-trade-33's portal
fix: dragons cross unknown portals when purely exploring (after a clean one-ping sonar check),
stop chasing explore value they would never collect, and break idle loops.

## What changed from the parents, and why

- Namespace/macros renamed bw6 -> bw7.
- The gst31 -> gst33 diff applied almost verbatim (body-wall-6's goal chooser is gst31's plus the
  waller chase): per-tile portal edges (`portal_`), a single sonar ping through an unknown portal
  beside the new head after a one-step move, next-turn crossing ("portal: explore") when the echo
  is clean, the whole body is known and in view, no enemy head in view, the goal has TileValue 0
  and is not a trap, round < 480; explore value for an unknown portal side only when we'd cross;
  loop breaker (12 heads on <= 6 tiles, nothing eaten for 12 rounds -> ban current + previous goal
  50 rounds; not while waiting at a counting-down bed, not the feeding target).
- body-wall-specific: a waller/cap-waller chasing an enemy head seen in the last few rounds is not
  exploring, so it doesn't cross (the enemy check also counts an enemy whose last-seen head is the
  goal). Wall squeeze / seal and cap wallers are untouched (the wall step runs before the goal).

## Measured (sim, 2 threads)

"Before" = this code with portal_cross=0, portal_explore=0, loop_break=0 (== body-wall-6), via a
tuning/debug build; idle = turns whose last 12 heads cover <= 6 tiles with nothing eaten (debug
counter, compiled out in the committed build). Eaten = our length gains.

| maps | before idle / eaten | after idle / eaten | after vs bw6 | after vs gst33 |
|---|---|---|---|---|
| official (11, 22 games) | 3.46% / 10652 | 0.78% / 10787 | 13-0-9 (before: 11-0-11) | 14-0-8 |
| portal official (7: Colosseum default queen_of_spades schooltime stronghold trauma trophy) | 4.52% / 5152 | 0.98% / 5112 | 9-0-5 (before: 7-0-7) | 8-0-6 |
| worldgen set 1 (20, random seed) | 1.59% / 58086 | 0.42% / 63183 | 22-0-18 (before 20-0-20) | 21-0-19 |
| worldgen set 2 (20, seed 8808) | 1.59% / 51033 | 0.38% / 51454 | 19-0-21 (before 20-0-20) | 24-0-16 |

Totals: 54-48 vs body-wall-6, 59-43 vs gather-split-trade-33. Idle loops fell ~4x (body-wall-6
orbited portals more than gst31 did: 3.5% idle on official vs gst31's 2.9%). About 350-1900
crossings per set, ~1000-1800 goal bans per 40 games. check: all checks passed (slowest-turn
numbers were noisy under machine load, same range as body-wall-6's).

## What should improve

Real server maps with portal-only rooms (Default, Trauma): dragons reach the beds and pearls
behind portals instead of orbiting. Local sims understate this (the server opponent farms
portal stores; see KNOWLEDGE.md).

## Not done / next

- Pearl gain in sims is small except worldgen set 1 (+9%); the win is mostly in time not wasted.
- Bans are frequent; a softer ban (score penalty) may be better.
- Wallers could also be allowed to cross when their chase target is stale.
