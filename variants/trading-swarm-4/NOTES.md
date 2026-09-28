# trading-swarm-4

## Idea

trading-swarm-2 (clone of the stronger real opponent, M491281) plus memory routing: each dragon
keeps a `strat::BoardMemory` and finds pearls and ripening beds with a BFS over remembered
tiles and remembered portal pairs, so it reaches portal-only stores and crosses portals whose
far end it has never seen while exploring, after a sonar check like gather-split-trade-33's.
The clone's character is kept: SPLIT 2 whenever possible, even trades, lunges, pincers, sonar
champion, late feeding.

## What changed from the parents, and why

From trading-swarm-2 (namespace `ts2` -> `ts4`, file `swarm4.cpp`):

- **Memory BFS** (`MemSearch`) replaces the 7x7 greedy pearl search: BFS from the head over
  `BoardMemory::Step` (wrap, kelp, known portal pairs), max 40 steps, blocked only by tiles
  occupied *this round*. First steps keep the parent's portal rules (not opposite the facing,
  not the same pair within length+1 rounds). A thread_local scratch buffer keeps per-dragon
  memory to the BoardMemory plus a per-tile portal-side mask.
- **Goals**: nearest remembered pearl seen within 25 rounds ("nearest pearl" / "pearl via
  portal"); else a bed within 10 steps whose estimated countdown (countdown - age) is >= 0 and
  ripens within 8 rounds of arrival ("to bed").
- **Exploring** (no pearl or bed goal, round < 480): cross an adjacent unknown-far-end portal
  when last turn's sonar check was clean ("portal: explore"); else cross a known portal 2/3 of
  the time as before ("portal: random"); else walk to the nearest (<= 16) tile beside an
  unknown portal ("to unknown portal"); else drift to the centre as before.
- **Sonar check**: when a one-step move lands beside an unknown portal, that turn's pings are
  replaced by a single ping (still the champion word) through the portal; next turn it crosses
  only if ECHOES shows no ally/enemy hit. Blind unknown crossings (first version) died on 5.6 of
  26 crossings a match on worldgen; with the check 0.5 of 15.
- **Portal escape**: when no plain step is free, a portal whose far end is in view and free
  ("portal: escape"); needed to keep `check` passing (stuck dragons died with a portal exit).

Crossings (portal: route/explore/random/escape) ~50 a match on official and 56 on worldgen vs
gst-27 (parent ~20-40); plus ~75-117 moves a match on paths that go through a portal later.

## Measurements (final code; official = maps/official 22 matches, wg = 20 fresh worldgen maps, 40)

| opponent | ts4 official | ts4 wg | ts2 official | ts2 wg (same maps) |
|---|---|---|---|---|
| trading-swarm-2 (parent) | 11-3-8 | 20-0-20 | | |
| gather-split-trade-27 | 11-0-11 | 15-0-25 | 13-0-9 | 14-0-26 |
| gather-split-trade-34 | 13-0-9 | 12-0-28 | 12-0-10 | 10-0-30 |
| late-surge-11 | 9-0-13 | 10-0-30 | 10-0-12 | 13-0-27 |

Third-party total 70 wins vs the parent's 72 on the same maps: even, within noise.
Earlier variants: pearl memory 60 rounds + beds anywhere within 40: vs parent 10-3-9 / 23-0-17
but third parties 64 wins; beds <= 10: 46 vs gst27/34. `check`: all checks passed, 0.0 ms
turns (spikes to 20-40 ms seen once were machine load, load average ~150).

## What should improve

Portal-heavy maps (trauma, default-style portal rooms, worldgen portal lattices): dragons now
reach stores behind portals (trauma: wins both sides vs the parent).

## What didn't work / open

- More crossings did not translate into wins against gst/late-surge; the champion side (late
  feeding, stuck deaths ~400/match on wg) is still where it loses.
- Stale pearls/beds pull dragons across the map; limits (25 rounds, beds <= 10) matter.
- "to unknown portal" can loop (walk to a portal, echo not clean, walk to another); a ban on a
  portal after an unclean echo would help.
- Crossings are ~50 vs the real opponent's ~110: its dragons probably cross known portals
  whenever the path through is shorter, and use portals to flee; untried.
