# ring-shield-memory (g53-6)

Base: ring-sealed-shield (arena ring-shield-1). Iteration 2 of the big-dragon lunge shield (see
variants/ring-sealed-shield/NOTES.md).

## Idea
ring-shield-1 cut late deaths of our 15+ workers to killers in view and in reach (33 v 63 per 112 games
v ring-sealed-2) but deaths to killers OUT of view and in reach rose (21 v 8): the shield's escape runs
picked the end furthest from what is in view, often next to heads that had just left the view or on
the view's edge with unseen ground beyond.

## Change (one: how LungeShield scores its candidate ends; trigger and tests unchanged)
- Remembered untradeable enemy heads (seen_ entries with round_-3 <= w.round < round_; 3 is
  FleeDirection's memory) count as threats: distance = torus distance from the remembered head less
  age * ceil(Le/4) (the free steps a dragon moves per round without paying segments), at least 1;
  Le = max(seen+partial, kShieldPartialLen=8). An end's score is min(in-view field, remembered).
- Scores are capped at kClear = reach(8)+1 = 9: every end beyond an 8-long's lunge is equally clear.
- Among equal scores, an end inside the view (Chebyshev < 3 from our head) beats one on the view's
  edge; an equal-score inner end also replaces an edge end of the chosen move.
- Interpretation note: the brief said "grown by their reach for the rounds since"; growing by the full
  lunge reach (8+/round for partial enemies) makes every tile in view distance 1, i.e. inert, so the
  per-round growth uses the free-step move and the next-turn lunge is in the distance scale itself.

## Local (fresh mapgen --family all --seed 53611, 44 maps x seeds 1,2, v ring-sealed-shield)
90-2-84; decided elimination 42-42, queen 14-13, longest 33-29. Shield moves 54 v 59 total.
Per family (16 each): anti-centre 6-10 (longest 6-9), crowded 8-8, maze 7-2-7, mixed 9-7, off-axis 8-8,
open 10-6, rich-near-home 9-7, ring 8-8, rooms 9-7, sealed-spawn 8-8, sparse 8-8.
check maps/official: pass.

## Arena duels v ring-shield-1 (eph-03dd6181)
- server random:2: 36-0-32 (52.9%), elim 13-14, queen 16-15, longest 7-3; 3+ starters 22-18, <=2 14-14.
  queen_length@end 11.8 v 10.9, longest@400 11.9 v 11.7. CPU max 31.1M.
- official random:2: 10-0-10; longest@300 13.3 v 10.7. CPU max 43.4M (parent 37.4M).

## Submit
Blocked by the permission system for the worker; handed back as ready (bot dir /tmp/g53-6/bot).
