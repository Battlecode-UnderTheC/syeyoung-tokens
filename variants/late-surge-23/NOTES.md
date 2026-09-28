# late-surge-23

## Idea

late-surge-22 plus a lunge guard for the dragons that matter at the end. A MOVE reaches
L-1 free steps, but Exposed() only guarded the tiles next to enemy heads; long dragons and
the champion were being lunged by short heads shadowing them.

## What changed from the parents, and why

- Namespace `ls22` -> `ls23`, macros `LS22_*` -> `LS23_*`.
- `MarkLungeThreat()` (once a turn, after Remember): for a dragon of length >= `guard_length`
  (10) or the frozen champion (round >= champ_freeze and it is the champion), a BFS over free
  tiles from each enemy head seen this round (that we would not trade with) up to
  min(L-1, lunge_reach) steps marks the tiles it can reach in one MOVE. Heads in view are all
  within ~6 steps, so the work is small. Enemies of length <= 2 are skipped (adjacency only).
- SafeStep: a candidate step onto a lunge-reachable tile counts as exposed (the goal step is
  refused, a detour is taken). In the fallback ranking exposure is graded: next to a head (2)
  is worse than lunge-reachable (1), which is worse than safe (0). The room count (`open`) still
  uses only adjacency exposure, so the guard doesn't shrink the room check.
- Short dragons unchanged.

## What was measured

Both seats, `sim --threads 2`.

| opponent | maps | late-surge-23 | late-surge-22 |
|---|---|---|---|
| late-surge-22 | official (22) | 12-0-10 (+3.8 longest, -7.8 total) | - |
| late-surge-22 | worldgen set A (40) | 20-0-20 (+0.2, -11.4) | - |
| late-surge-22 | worldgen set B (40) | 19-0-21 (+0.2, +1.5) | - |
| trading-swarm-4 | official | 17-0-5 (+6.3 longest) | 16-0-6 (+1.9) |
| trading-swarm-4 | worldgen A | 26-0-14 (+6.4) | 26-0-14 (+4.3) |
| trading-swarm-4 | worldgen B | 28-0-12 (+7.5) | 33-0-7 (+12.5) |
| trading-swarm-4 | worldgen C | 29-0-11 (+4.6) | 27-0-13 (+0.2) |

Totals: vs the parent 51-0-51; vs trading-swarm-4 100-42 against the parent's 102-40.
The longest-dragon margin vs trading-swarm-4 improved on 3 of 4 sets. Overall about even.
The per-match count of long-dragon lunge deaths was not measured (the victim can't see who
killed it; that needs a sim-side tally).

## What should improve

Fewer champions lost to shadowing short heads in the endgame against lunging swarms (the
server's split-v7 style opponents, which lunge 2-4 steps at our long dragons). Local opponents
rarely lunge long dragons, so the local signal is small.

## Tried / didn't work

Nothing else tried (short loop).
