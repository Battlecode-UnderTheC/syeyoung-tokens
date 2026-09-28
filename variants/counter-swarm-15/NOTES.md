# counter-swarm-15

## Idea

counter-swarm-14 with hunting (workers chasing enemy heads within 6 steps) given its own
cutoff, `hunt_until = 300`, instead of stopping only when feeding starts (380).

## What changed from the parents, and why

- Namespace cs14 -> cs15 (files, macros).
- New param `hunt_until` (300). In cs14 the hunt gate was `round_ < feed_from_round`, so
  porting late-surge's endgame (feed from 380) accidentally extended hunting from 340 to 380.
  Late trades cost bodies that the endgame would rather feed to the champion. Feeding stays at 380.

## What I measured (sim, both sides swapped per map)

Official maps (11) = 22 matches; P1, P2 = two fresh batches of 20 worldgen maps (40 matches each).

| hunt_until | vs cs14 official | vs cs14 P1 | vs cs14 P2 | vs gst31 official | vs gst31 P1 | vs gst31 P2 |
|---|---|---|---|---|---|---|
| 340 | 9-0-13 | 19-0-21 | 19-0-21 | 10-0-12 | 15-0-25 | 15-0-25 |
| 300 (kept) | 11-0-11 | 25-0-15 | 19-0-21 | 11-0-11 | 19-0-21 | 12-0-28 |
| cs14 itself | | | | 10-0-12 | 15-0-25 | 16-0-24 |

Totals: 300 vs cs14 55-47; vs gst31 42-60 (cs14: 41-61; 340: 40-62).
Mean total-length margin vs gst31 (300 / cs14): official -79.7 / -73.5, P1 -70.9 / -93.8, P2 -95.0 / -83.2.

## What should improve

Slightly fewer late worker losses, more body left for the 380 feed. The effect is small,
within noise on 102 matches; the gap to gst31 (total length ~-70..-90) is NOT explained
by late hunting. Look elsewhere (gst31 has more "detour", fewer "trapped: no move";
the gap is in total length, mid game growth).

## Tried, didn't work

- hunt_until = 340: slightly worse than 300 vs cs14 (47-55) and no better vs gst31.
