# counter-swarm-13

## Idea

counter-swarm-12, plus a guard for the big dragons in the endgame. From the feed round (340),
the champion and any dragon at least 60% of the champion's reported length (and at least 8 long)
keep their heads out of the lunge reach of every enemy head in view, as long as a free step
outside that reach exists. Everything else is unchanged.

## What changed from the parents, and why

- Namespace and macros renamed from cs12 to cs13; the files are now cs13.hpp and cs13.cpp.
- `MarkThreats()` runs in step 5, before a step is chosen. For every enemy head seen this
  round it floods free tiles up to `length − 1` steps (the seen length, a lower bound). When
  part of that enemy lies on the window's edge its body probably runs out of view, so it is
  longer than seen, and its reach gets `guard_cut_extra = 3` more. `SafeStep` then skips
  threatened steps when any free, unthreatened step exists. If every step is threatened it
  behaves as before, so the dragon never freezes. Among the safe steps the usual key
  (room, then liberties) picks, which favours open areas. The reason tag is
  `champion: out of reach`.
- Kamikaze and strike are unchanged.
- Why guard by a 60% share and not only the champion: in one trace the team's longest
  dragon (51) died to a lunge while `Champion()` named another ally (reports lag), so that
  dragon was unguarded.

## Evidence for the problem

I used a debug build (CS13_DEBUG, compiled out of the committed code), with our variant as
team A only. I counted deaths of the team's actual longest dragon (at least 10 long, round
340 or later), by cause. Most of these deaths are the longest dragon resigning to feed a
champion report of equal or greater length. Those are merges, not losses.

| opponent, maps | cs12: feed / trapped / killed by enemy | cs13: same |
|---|---|---|
| cs12, official (11) | 6 / 1 / 0 | 6 / 1 / 0 |
| cs12, worldgen (20) | 10 / 2 / 4 strike | 10 / 2 / 1 strike |
| gst23, official (11) | 3 / 0 / 3 lunge + 1 kamikaze | 5 / 0 / 1 lunge |
| gst23, worldgen (20) | 11 / 3 / 2 lunge + 1 kamikaze | 9 / 4 / 2 lunge |

Enemy kills of the champion fell from 11 to 4 over 62 games, about 0.18 to 0.06 a game. A
first version guarded only the self-declared champion and used only the seen length; it
cut the kills to 6.

## Measured

`sim` W-D-L from cs13's side, playing both sides of every map:

| opponent | official (11 maps) | worldgen batch 1 (20) | worldgen batch 2 (20) |
|---|---|---|---|
| counter-swarm-12 | 11-0-11 | 22-0-18 | 23-0-17 |
| gather-split-trade-23 | 9-0-13 | 22-0-18 | 16-0-24 |

For comparison, counter-swarm-12 against gather-split-trade-23 on the same maps went
10-0-12, 19-0-21 and 16-0-24. `check` says "all checks passed", slowest turn 0.1 ms.

## What should improve

The longest-dragon margin at round 499 in games where the enemy lunges (the gst and
swarm-hunt lines). The effect is small, about 0.1 champions saved a game, so expect a few
Elo at most.

## What didn't work, and what wasn't tried

- Guarding only the self-declared champion missed the dragon that reports named as someone
  else.
- Not tried: using the previous round's sightings for enemy heads just out of view.
