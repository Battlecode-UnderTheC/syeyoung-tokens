# ring-rush-1

Task g39-4. Parent: ring-sched-1. New line: ring-rush.

## Idea

Win the race for the shared middle (server-r13 #1, 44 losses on centre maps). The opponent
has 2+ heads on the central bed cluster by about r16; we arrive at r26-31 and stop 1-4 tiles
short, because every tile there is in an enemy head's reach.

## What changed from the parent, and why

All in `rush.cpp`, plus hooks in `strategy.cpp` (Decide, ChooseGoal, WorthTrading, Exposed).

- **Rushers.** The two lowest-id non-queen starters (ids q+2 and q+4: map dragons alternate
  teams) rush when the board's centre is 5..26 steps from their spawn. Until r30 their goal
  is the centre: the target itself, or the frontier tile (beside an edge never seen) that
  minimises steps there plus the straight distance on. They don't growth-split on the way.
  With length 5+ they take a second free step when it is in view, free, not exposed and
  roomy.
- **Target refinement.** Pearls and beds due within 30 rounds, seen within 4 of the centre,
  are scored by how many of them lie within 2 tiles. The densest cluster (score 2 or more)
  becomes the target.
- **Ending the rush.** A rusher has arrived within 2 tiles of the target. It gives up after 6
  rounds without getting nearer, or at r30. An arrived rusher holds the middle until r60:
  goals more than 8 tiles from the target count half.
- **Local parity.** This applies to any worker, never the queen, before r60 with its head
  within 7 of the centre.
  - While our heads within 4 match theirs, tiles next to enemy heads don't count as exposed.
  - While ours outnumber theirs, a 1-for-1 head-on trade counts as worth it (kamikaze and
    lunge).
- The queen's logic is unchanged. Everything is gated on observation: the board size, our own
  id and spawn, and the beds seen.

Traced arrival (sim, seed 1):
- Devil: r10 and about r18.
- Trophy: about r21 (one worker; the cluster is 21 steps away).
- Dilemma: about r16.
- The rush gives up early when walled in: Tower Defense r6, Stripes r9.

## Results

Local smoke (server maps, seeds 1-2, vs ring-sched-1): 38-0-30.

Arena duels (ephemeral, vs ring-sched-1):
- Server suite, random:2: 39-29. By starters: 3+ 26-14, 2 or fewer 13-15.
  - Dragons at r100: 25.3 v 20.3.
  - Board control at r200: .51 v .44.
  - Queen alive at the end: .75 v .63.
- Official suite, random:2: 12-8.

Arena verdict (ring-rush-1, draft): 45-0-33 vs ring-sched-1, so it BEATS its parent (within
noise).
- By suite: server 40-28, official 5-5.
- By map type: centre maps 14-8, others 31-25.
- How games were decided: queen 21-13, elimination 18-10, longest dragon 6-9.
- Against the other bots:
  - ring-plus-21: 42-36;
  - cs-queen-split-tail-risk-v1-7: 43-35;
  - ring-bigq-1: 46-32.
- Eval vs the parent: P(win) .54 / .58 / .55 at r100 / r200 / r300.
- CPU: p99 20M, max 31.9M.
