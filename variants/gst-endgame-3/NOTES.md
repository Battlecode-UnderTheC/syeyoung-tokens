# gst-endgame-3 (arena round 9, team endgame)

## Idea
gst-search-8 with one behaviour added: **a big dragon (length >= 10) that a shorter enemy could
lunge next turn runs, using the free steps of a 1.2.3 move** (ceil(L/4), at most 5).

- search7's lookahead picks its one step as before. If that step's tile can be reached next turn
  by a visible enemy head shorter than us (BFS over free tiles, lunge reach), `Escape()` searches
  every 2..ceil(L/4)-step path of free tiles (our body moving as it goes, pearls eaten) and takes
  the one that ends out of every shorter enemy's reach with room (RoomPenalty 0), preferring pearls.
- It sees the threat: for a big dragon, an enemy whose body runs out of view gets lunge reach
  max(seen - 1, 6) (gst's lunge cap) instead of seen length - 1.
- plugin.cpp takes the run from r250 (the lookahead's start) unless gst is feeding, trapped,
  farming or walking to an ally, and only if gst's memory says the path is free.
- gst's body_ is rebuilt from the view after a multi-step move (code from gst-sprint-1).

No map keys, no env vars, no static state.

## Why (traces, sim 1.2.3, server maps seeds 1-2, vs gst-queen-guard-2 and gst-giant-2, 80 games)
Late (r350+) deaths of our champion-class dragons (>= 0.8 of our longest): 19 in 80 games, 459
length, all head-to-heads where an enemy *lunged* us. The victim's last move was the late
lookahead's step in 14, the judge's override in 5. Debug of search7 at those turns:
- enemies seen with 3-4 segments (rest out of view) were 8-13 long, so their reach was
  underestimated (2-3 instead of 6-7);
- a champion chased by a short enemy had every one-step move in reach (reply cost on all 4);
  with 5 free steps it could outrun it but gst never takes more than one.
Enemy champions died this way about half as often.

## Measured
Same 80 games: champion lunge deaths 19 (459 length) -> 14 (244). The run fires about once a match.
Sim, server maps seeds 1-5, both seats (100 games each): vs gst-search-8 49-0-51; vs
gst-queen-guard-2 42-58 (gst-search-8: 45-55); vs gst-giant-2 58-42 (gst-search-8: 57-43). Neutral
within noise: the change is narrow. `check` passes on maps/server.
