# ring-symearly-1

g42-2. Base: ring-sched-17 (code: ring-sched-m3).

## Idea

Commit to the map symmetry much earlier than r57-77, without ever committing wrongly, and
share what the team has ruled out over sonar.

## What changed from the parents, and why (sym.hpp, strategy.cpp, ring.cpp)

1. **Commit when one candidate is left.** Every map declares x, y or xy, and a candidate is
   only dropped on a contradiction, so the last one standing is the map's. The extra
   12-confirming-pairs gate (ring-sym-1) guarded nothing and held commits back.
2. **Share the surviving set, not just a commit.** Bits 1-3 of the champion word (0xA7) now
   carry the sender's surviving candidates (it used to carry only a usable singleton), and so do bits 1-3 of
   hot-bed claims (0xA8) and bits 26-28 of ring words (0xBA). The receiver ANDs them in. Ring
   words matter most: the ring queen mostly hears ring words, so before this she often never
   learned the symmetry.
3. **Starting layout.** Dragon 0 acts first in round 0, so it sees the spawn exactly. In its
   window, a tile and its mirror must both be empty or held by opposite teams, and the enemy
   queen (id 1) must be on its own head's mirror. This holds on all 22 contest maps, 30
   worldgen and 56 mapgen maps checked (starters mirror under the declared symmetry, with
   dragon 2k+1 the mirror of 2k).
4. **Iteration (after the first duel).** Under the shared commit every dragon, children
   included, knew the far side's bed timers in a few rounds. On Prisoners Dilemma and Autarky
   (a contested central cluster) that lost 0-8 on the arena and 4-20 locally: replays showed
   small dragons going into the central cluster among enemies by r30 and the team being
   ground down by r60. So: **edges** (kelp, empty) are mirrored on the shared commit, but
   **tiles** (beds, timers, plain tiles) still wait for the dragon's own evidence, as before.

## Correctness (SYM_TRACE build, every dragon's commit compared with the declared symmetry)

- 22 contest maps × seeds 1, 2 × both seats (92 team-games): 0 wrong commits; the declared
  candidate was never eliminated.
- 130 fresh maps (88 mapgen `--family all` seeds 424242 and 777001, 12 `trick`, 30 worldgen;
  28 x, 28 y, 74 xy) × 2 seeds × both seats (520 team-games): 0 wrong commits.

## Commit round (medians over team-games, contest maps)

| | base | new |
|---|---|---|
| first dragon of the team | r14 | r1 |
| queen | r88 (16 of 92 never) | r5.5 (1 never) |
| median starter | r65 | r7.5 |

On the fresh maps: first r5, queen r12, median starter r11 (x r15.5, y r10, xy r11.5 for the
queen). The slow ones are big maps (unsw, australia) and weakhold, where y and xy agree on
everything but the countdowns.

## Arena

- First version (beds on the shared commit): server 35-33, official 10-10; but
  prisoners_dilemma 0-4 and autarky 0-4; queen alive at end 47% vs 65%.
- Final (beds on own evidence): server 37-31 (dilemma 1-3, autarky 2-2, default 0-4,
  portals 4-0), official 9-11. Queen alive 56% vs 60%. CPU max 35M.
- Local fresh mapgen (first version): 257-4-259 v ring-sched-m3, no family clearly lost.

## Arena verdict (ring-sched-35)

- Against the parent ring-sched-17: 47-0-65 (42.0%, CI 33-51), so it loses to the parent. By suite: server 28-40,
  official 5-5, custom 14-20. Eval P(win) against the parent: r100 0.485, r200 0.457, r300 0.454.
- Against ring-rush-1: 67-45. Against cs-queen-split-tail-risk-v1-7: 65-47. Overall 226-222.
- Worst maps: trophy 3-13, autarky 3-13, custom home-clusters 1-7, dilemma/portals/devil 5-11.
- CPU p99 27.3M, max 39.5M.
- Replays (dilemma seed 8191469905430442526, portals same seed): the games match the base
  move for move until r20-45 and then diverge. No change you can see is clearly the intended
  one: the earlier symmetry only changes memory (mirrored kelp), and the knock-on effects
  (different routes and splits) look like noise, not better play.
