# gather-split-trade-42

## Idea

A KNOWN portal was a one-way door: `SafeStep` caps a blind step (landing tile out of view) at
1 liberty and a goal step needs 2, and a portal's far end is always out of view, so every goal
step onto a known portal became "detour (goal step unsafe)" (VISUAL.md #11 "doorstep waltz").
Extend gst33's sonar probe from unknown to known portals.

## What changed from the parents, and why

Parent gather-split-trade-39 (namespace renamed gst39 -> gst42). All new code is behind
`known_portal = 1` / `blind_fallback = 2` (known_portal=0 reproduces the parent exactly).

- When the preferred goal step is refused *only* for being blind (not exposed, >= 2 liberties,
  escape level >= 1), remember (tile, portal direction, goal) and count a streak per goal.
- In `Turn`, when stepping back onto that tile, ping only through that portal (same trick as the
  unknown-portal probe; it replaces the champion broadcast that turn; not possible when the
  portal side is opposite our facing, since that ping starts at the tail).
- In `SafeStep`, a blind step's cap is lifted when: last turn's ping from here through that
  side came back clean ("portal: pinged clear"); or the exit tile was seen empty within 2
  rounds ("portal: exit seen empty"); or the same goal's blind step was already refused
  `blind_fallback` (2) times within 30 rounds and our ping from here did not echo a dragon
  ("portal: fallback").
- Debug counters (GST42_DEBUG, compiled out): portal-steps (moves across a portal edge),
  portal-detours (unsafe-goal detours whose preferred step was a portal edge), lifted.

## Measurements

Portal counts (gst42 dragons, both seats, 2 matches per map vs gst39; before = knobs off):

| map | portal-detours before -> after | portal steps before -> after | pearls eaten |
|---|---|---|---|
| schooltime | 493 -> 264 | 245 -> 288 | 2257 -> 1663 (games differ) |
| default | 412 -> 327 | 68 -> 231 | 492 -> 650 |
| trauma | 116 -> 118 | 120 -> 199 | 643 -> 1019 |
| queen_of_spades | 122 -> 104 | 192 -> 222 | 626 -> 653 |
| stronghold | 37 -> 37 | 90 -> 92 | same |

vs gather-split-trade-39, both seats (machine load ~130, timings noisy):
- official (11 maps): 12-0-10, longest -2.4, total -12.3 (schooltime 0-2: 9-54 and 30-51)
- worldgen batch A (20): 22-0-18, longest -0.9, total -7.7
- worldgen seed 4242.. (20): 19-0-21, longest -2.4, total +2.7
- overall 53-0-49: even within noise.

## What should improve

Portal-heavy maps (Default, Trauma: most beds are portal-only) and the server opponent that farms
portal stores. Crossings of known portals rose 1.2-3.4x, detours on portals fell up to 47%.

## Concerns / what didn't work

- Lifted steps are not free: on schooltime ~28 lifted steps per 2 matches carried ~2.5 fatal turns
  per match (pinged clear 1.5, fallback 0.5, seen empty 0.5). A clean echo only says the straight
  line past the exit is empty; a head beside the exit can still take it. Next: after a lifted
  crossing, require >= 2 liberties counted on the exit's known neighbours, or ping twice
  (straight and a side) before crossing with a long dragon.
- Schooltime lost both seats against the parent; not investigated (budget).
