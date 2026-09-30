# gst-giant-2

## Idea
gst-giant-1 (gather-split-trade-131 + the rollout judge) with the judge widened. `rollout.hpp` is
self-contained: a Judge with Observe(o) every turn and Review(o, action, &overridden,
with_splits, cautious). It wraps any gst copy the way plugin.cpp does.

1. **Lunge-capable rollout policy.** In rollouts, any dragon of length 3 or more lunges (2-4
   steps) onto an opposing head at least as long, with probability 1/2 when one is in reach.
2. **Escape review.** When gst's action is a trapped one (reason "trapped: ..."), the judge
   reviews it against the single steps, SPLIT 2 and SPLIT L-2.
3. **Contest review.** With no enemy head in view, but another head and a pearl in view, gst's
   move is reviewed against the other steps, excluding steps that end next to another head
   (without that exclusion these overrides killed about 17 dragons a match).

## Measured (vs gst-giant-1, official, seeds 1-5, 150 games)
| change | W-D-L |
|---|---|
| lunge policy alone | 79-0-71 |
| + escape + contest (uncautious) | 74-0-76 |
| + escape only | 77-1-72 |
| + escape + cautious contest (kept) | 81-0-69 (r100: 22.6 v 19.6 dragons, 57.2 v 49.2 total) |

## Portal fixes (shared with gst-giant-1b)
- `check` found self-deaths through portals.
  - The judge's model stepped through portals whose far end it didn't know, as if they were
    open floor. Its lunge candidate then crossed one, and the next turn gst stepped back into
    the body the lunge had left.
  - The judge's body history also missed the middle tiles of multi-step moves.
- Fixes:
  - An unseen edge between two seen tiles (a portal with an unknown far end) is closed in the
    model.
  - `Commit(action)` records the path of every multi-step move, so the next turn's body includes
    it.
  - Every own segment in view counts as our body in the model.
  - An own-body veto: if gst's own step lands on our body (a portal exit, say), the judge picks
    the best other candidate.
- `check` passes on maps/official and maps/server.

## gst-giant-2b
gst-giant-2 plus one line: Lunge() plans its path with mem_.Step (seen edges only) instead of the model table, which treats unseen edges as open floor (check: a wall death on schooltime r499, "rollout: lunge").
