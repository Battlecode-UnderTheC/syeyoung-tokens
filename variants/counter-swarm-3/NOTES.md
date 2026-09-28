# counter-swarm-3

## Idea

Finish the enemy. A team with no dragons left loses at once, so when our whole team has not
seen an enemy for a while (and we have plenty of dragons), some workers stop gathering and
feeding and search the tiles we have never seen or not seen for a long time, then strike the
heads they find with counter-swarm-1's hunt and multi-step strike.

## What changed from the parents, and why

Parent: counter-swarm-1 (namespace renamed to `cs3`, files `cs3.cpp`/`cs3.hpp`).

- **A team-wide "last enemy seen" round.** Each dragon keeps the latest round any enemy was
  seen, from three sources: its own window, its own sonar echoes (a ping of ours that hit an
  enemy body or head last turn means one is out there), and the value relayed by allies. Every
  dragon pings it in all four directions every turn (32-bit word, tag `0b110` in bits 29..31,
  `round + 1` in the low 9 bits; no word until something was seen). counter-swarm-1 pinged
  nothing, so the pings are free.
- **Finishing mode** (`Finishing()`): a worker (not a leader), round >= 200, at least 6 of our
  dragons, at most 2 distinct enemies seen by itself in the last 100 rounds, the team-wide last
  sighting at least 40 rounds old, and only workers with an even id (half of them). While
  finishing, a worker does not feed; it hunts enemy heads in view even after round 340; and
  every tile on the unknown edge or unseen for 60+ rounds scores `1.0 / (dist + 1)` (pearls
  still compete). Reason tag: `finish: search`.

The first version used only the dragon's own sightings. It fired constantly (a worker in our
half often sees no enemy for 100 rounds in a normal game), took feeders away and lost
longest-dragon games: 28-34 against counter-swarm-1 on official + 20 worldgen maps, and 31-9
against strategy-v1 where the parent goes 35-5. The team-wide signal (sonar relay + echoes)
fixed that: it now fires in a few games only.

## Measured

Native sim, maps: official (11 × 2 sides), practice (20 worldgen × 2), practice2 (20 more
worldgen × 2). The copy of counter-swarm-1 against its parent is 11-11 and 20-20.

| opponent | official | practice | practice2 |
|---|---|---|---|
| counter-swarm-1 | 12-0-10 | 26-0-14 | 20-0-20 |
| strategy-v1 | | 34-0-6 (parent: 35-0-5) | |

Games ending before round 499 (by elimination), against counter-swarm-1: before (the copy)
10/22 official, 18/40 practice; after 8/22 official, 17/40 practice, 14/40 practice2. Against
strategy-v1 on practice: parent 20/40, this 20/40. So the search mode does **not** measurably
increase eliminations. It triggers rarely: about 25 `finish: search` moves per match (vs about
5000 `toward goal`). `check`: all checks passed (slowest turn 1–3 ms under a machine load of
~13–50; the parent measures the same at the same time).

## What should improve

Mostly nothing big: about even with the parent, maybe a few wins where the enemy's last
dragons hide far away. The team-wide enemy-sighting relay is the reusable part: it is a
cheap, accurate "the enemy is quiet" signal that the parent lacked.

## Tried, didn't work

- Own-sightings-only trigger (from round 150/250, units 6/12, one worker in 2/3): fires in
  normal games, costs feeding, longest-dragon losses at 499.

## Next

- Eliminations don't rise: a finishing worker only trades with an enemy at least as long as
  itself (fine, the last enemy dragons are usually long), but searchers rarely find them.
  Aiming the search would help: relay the enemy's last seen position (not just the round), or
  search the mirror of our start first (maps are symmetric, but dragons aren't told the
  SYMMETRY line).
