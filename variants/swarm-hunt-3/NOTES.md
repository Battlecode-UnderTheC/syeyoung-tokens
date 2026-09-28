# swarm-hunt-3

## Idea

swarm-hunt-1 with its runtime knobs removed, the keeper bug fixed, and the swarm spread out
so fewer small dragons get boxed in by their own allies.

## What changed from the parents, and why

- Every `SWH_*` getenv knob is gone: `Params` defaults are the constants; the debug dump is removed.
  Namespace `swh3`.
- Keeper bug: keepers were "first turn in round 0", which also caught children split in round 0.
  Now `round == 0 && !o.echo_present`: split children inherit PROTOCOL 3 and see ECHOES on their
  first turn, map dragons don't.
- Spreading:
  - normal splits leave `unit_reserve = 6` unit slots free, so trapped dragons can still split out;
  - goal scores are divided by `1 + 0.5 * (ally segments within 2 steps of the goal)`;
  - room checks count an ally's last visible segment (its tail, which moves away) as room;
  - detours break room ties toward the less crowded side;
  - dragons of length <= 3 never take "dead end worth it" (they can't split out).
  - Tried and turned off (`split_room = 0`): split only with 2x RoomNeeded free around head and tail.
    It halved splits and lost 13-27 on worldgen.

## Measured (vs swarm-hunt-1, both sides of every map)

- final: official 11-0-11 (longest +1.4, total -4.8); 20 worldgen maps 22-0-18 (longest +1.1, total +29.2).
- trapped: no move per match: official 41 -> 27, worldgen 371 -> 339.
- with split_room = 2: official 13-0-9, worldgen 13-0-27.

## What should improve

Fewer trapped deaths and slightly longer dragons, mostly on crowded worldgen maps. Most trapped
deaths remain: they're length-2 dragons, which can't split. Next: stop producing length-2 dragons
(split_min_length 6), or path around 1-wide pockets.
