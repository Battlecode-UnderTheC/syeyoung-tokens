# body-wall-9

## Idea

A real seal for the body-wall line: a multi-step MOVE (up to 4 steps, each after the first
costs a tail segment) that lays our head and new body across a pocket's exit in one turn,
before the enemy head inside moves again. Single steps (body-wall-8) almost never close a
pocket; a 2–4 step move can.

## What changed from the parents, and why

- Namespace/macros renamed bw8 -> bw9.
- `Wall`: when no single safe step seals anyone (best seal not above worst), `MultiSeal`
  runs a DFS over paths of 2..min(seal_steps=4, L-1) free, in-view tiles (never an exposed
  final tile). Our body after the move is simulated exactly as the engine does it (head
  pushed, tail popped unless a pearl is eaten, one more pop per step after the first). For
  each path that touches an enemy's room (flood from its head, other dragons' bodies counted
  passable since they move on), the timed `EnemyReach` checks the seal. The best
  sealed-enemy-length minus seal_cost(1.0) x segments spent wins, then must pass our own
  `Liberties` and `EscapeLevel`. Bounded by seal_evals=64 enemy floods per turn.
- Enemies that every safe single step already seals are excluded (doomed anyway: spending
  segments on them earns nothing). Without this, ~60% of "multi-seals" were such enemies.
- Reason tag `wall: multi-seal`. Under `-DBW9_DEBUG` a tracker counts sealed enemies and
  whether they vanished while their head tile stayed in view (i.e. died) within 4 rounds.

## Measurements (sim, 2 threads)

Real seals (debug tracker): body-wall-8 had ~0 (its single-step seal fires ~2/match on
worldgen, 0 on official). body-wall-9: ~6-8 multi-seals per worldgen match, ~1 per official
match; of those resolved, ~85% confirmed dead (worldgen set 1: 230 sealed, 92 gone / 6 alive
/ 6 lost sight; set 2: 319 sealed, 133 / 22 / 6; the rest unresolved when our sealer moved or
died).

W-D-L, final version:
- vs body-wall-8: official 12-0-10, worldgen set 1 23-0-17, set 2 19-0-21 (54-48 overall)
- vs gather-split-trade-38: official 10-0-12, set 1 21-0-19, set 2 16-0-24 (47-55);
  body-wall-8 on the same: 10-12, 18-22, 17-23 (45-57)

## What should improve

Slightly more enemy trapped deaths and fewer enemy survivors in crowded maze worldgen maps.
The effect is small next to the ~230 trapped deaths per match both sides already suffer.

## Tried

- v1 without the doomed-anyway filter and 60 path evals: 55-47 vs body-wall-8 (similar), but
  most of its seals were enemies already doomed; replaced.
- Capping at 24 full path evaluations (safety + floods for every path) cut seals by 60% and
  lost a little; evaluating enemies first and only checking our safety for a winner fixed it.
- Timing: `check`'s slowest-turn is meaningless right now (load avg ~140; the parent shows
  20-45 ms spikes too). Cost is bounded by 64 enemy floods plus one safety flood per winner.
