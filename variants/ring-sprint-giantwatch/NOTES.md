# ring-sprint-giantwatch

## Idea

Strategist a (g45) #2: start late concentration (champion + feeders) when an enemy giant is
observed, not only at the fixed r430 clock. server-r19 C2 (21 longest-tiebreak losses, both
queens dead): the opponent converts material into one dragon ~50 rounds before we do (longest
13 v 18 at r400, 18 v 30 at r450) while totals are level.

## What changed from the parents, and why

Base: ring-sprint-6 (variants/ring-sprint-conveyor). Only `UpdateLateChampion()` and `Remember()`.

- `Remember()`: each dragon keeps, per enemy id, the longest it has seen that enemy (segments in
  view; exact when the whole body is in view, else a running max lower bound), plus a head trail:
  while it has seen the enemy's head every round since the head stood on tile T and a segment of
  that enemy still sits on T, length >= head positions since then + 1 (the body is the last L
  tiles the head stood on).
- `UpdateLateChampion()`: the champion is now computed from kNoBurnFrom (r250) instead of
  kDieNearFrom (r400), under the same "our queen not known alive" gate. A dragon latches
  `giant_latched_` once the longest enemy it saw within p_.enemy_memory (60) rounds is
  >= max(champion's reported length, kFeedMaxLen) + kFeedMaxLen/2. Once latched (and the queen
  gate still passes) die_near_ and late_conc_ are on. Without a latch: unchanged (r400 champion,
  r430 concentration). Everything downstream is unchanged.
- Margin kFeedMaxLen/2 = 4: what one full-size feeder (8) leaves when it resigns. Floor
  kFeedMaxLen: a dragon shorter than that is itself a feeder, so a champion report under 8 only
  says this dragon hears no long ally. Giant bar is therefore >= 12.

Iterations (all arena duels vs ring-sprint-6 on server, random:2 seeds, 68 games):
1. Literal spec (enemy >= champion report + 4): 38-30, longest-decided 8-4. But it latched on 6-10
   long enemies against a 2-5 long "champion" (a dragon's local view) from r250 in most games:
   i.e. feeding on a clock again, not a giant trigger. Rejected on intent.
2. Floor kParallelGrowLen (bar 20): 34-34 with identical metrics: never fired. Own sightings are
   lower bounds from a 7x7 view; locally vs giant-8 (champion 30-57) the most we ever saw was 14.
   Added the head trail; it helps little (we rarely keep an enemy head in view for long).
3. Floor kFeedMaxLen (bar 12), submitted.

## Local / duel numbers (iteration 3)

- check maps/official: all checks passed.
- Local sim vs ring-sprint-conveyor, maps/server seed 1: 17-17 (mirror; 40 latches in 34 games,
  mostly r400-490, a few r270-360). Fresh mapgen (mixed, seed 4505, 12 maps): 11-13
  (longest-decided 3-5, 2 games changed).
- Arena duel vs ring-sprint-6, server random:2: 34-0-34 (3+ 20-20, <=2 14-14); longest@400
  11.42 v 10.88; queen_length@end 11.8 v 10.6. Official random:2: 10-0-10, longest@400 18.6 v 16.5.

## Arena verdict

(pending)
