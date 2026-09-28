# trading-swarm-5

## Idea

A more faithful clone of the strongest real server opponent (M491281). trading-swarm-4 is
that clone, but it barely used portal-only stores: the real one lives behind its portals
(Default: 203 crossings and 241 pearls eaten in portal-only regions; Trauma: 141 crossings,
709 portal-only pearls vs 81 open, from `replays/M491281/analysis/regions.txt`). ts5 keeps the
clone's character (SPLIT 2, sonar champion, even trades, lunges, pincers, late feeding) and
adds portal-room farming and readier crossings.

## What changed from the parent (trading-swarm-4), and why

Namespace `ts4` -> `ts5`, file `swarm5.cpp`. All changes sit behind `constexpr kNew = true`
(false gives ts4's behaviour, used for the debug baseline).

- **Home component**: each dragon floods from its first head tile over remembered plain
  edges (no portal side, no unseen edge, no kelp). Tiles outside are "room" (portal-only as
  far as this dragon knows). A child born in a room treats that room as home.
- **Room stores**: room pearls are remembered for 60 rounds (25 open); room beds are goals
  from up to 30 steps (10 open) and 20 rounds before ripening (8 open) ("to portal bed").
- **Farm room**: a dragon inside a room with no pearl goal goes to the room bed ripening
  soonest (if it ripens within its distance + 12), instead of drifting out ("farm room").
- **Crossing gates relaxed**: known adjacent portals are crossed always when outside a room
  (2/3 inside; parent 2/3 everywhere); the unknown-portal sonar check now only vetoes on a
  *head* echo (ally or enemy), not on body hits down the ping's long line; "to unknown
  portal" reaches 30 steps (16); new "to portal": walk to a known portal within 10 steps
  (not the one just used) and take it (always outside a room, half the time inside).
- **Debug counters** (`TS5_DEBUG`, compiled out; not thread-safe, use `--threads 1`): per
  team crossings and pearls eaten in true portal-only tiles (flood from the team's round-0
  home over everything the team has seen), printed at exit.

## Measurements

Faithfulness (debug build, per match, vs trading-swarm-4; "seat A ; seat B";
crossings / portal-only pearls / open pearls):

| map | real opponent | ts4 behaviour (kNew=false) | ts5 |
|---|---|---|---|
| Default | 203 / 241 / 188 | 15 / 6 / 19 ; 35 / 16 / 5 | 269 / 287 / 312 ; 227 / 198 / 5 |
| Trauma | 141 / 709 / 81 | 49 / 412 / 21 ; 79 / 935 / 10 | 65 / 799 / 16 ; 85 / 1025 / 18 |
| 20 worldgen (avg) | - | 45 crossings, 94 room pearls | 46 crossings, 70 room pearls |

Default now matches the real one closely (from ~6-16 portal-only pearls to ~200-290).
Trauma was already room-heavy in the clone (its spawns sit in a tiny open region); crossings
there are still about half the real one's.

W-D-L (release build, both seats; official = 11 maps x 2, wg = 20 fresh worldgen maps x 2):

| opponent | ts5 official | ts5 wg | ts4 official | ts4 wg |
|---|---|---|---|---|
| trading-swarm-4 | 8-4-10 | 20-0-20 | | |
| late-surge-21 | 8-0-14 | 10-1-29 | 5-0-17 | 14-1-25 |
| gather-split-trade-48 | 5-0-17 | 18-1-21 | 7-0-15 | 13-1-26 |

Third-party wins 41 vs the parent's 39 on the same maps: even. `check`: all checks passed,
slowest turn 0.2 ms (big_empty).

## What should improve

As a sparring partner: portal-heavy fixes in other lines should now show up in sims on
Default-style maps (portal rooms with beds), where the clone used to ignore those stores.

## What didn't work / open

- Treating unknown tiles as reachable in the home flood made nothing a room (it leaks around
  unexplored gaps); the flood must use only seen edges.
- A first version cut crossings (dragons parked in rooms): farming only when a bed ripens
  soon, plus the "to portal" walk, restored them.
- Relaxing the sonar veto to heads only raised "portal: explore" deaths on worldgen
  (0.6 -> ~2 a match).
- Trauma crossings (65-85) still trail the real one's 141, and worldgen crossings barely rose
  (45 -> 46): "to portal" rarely fires because a pearl or bed goal almost always exists.
  Crossing a portal when the path through is no longer than +2 of the plain one might.
