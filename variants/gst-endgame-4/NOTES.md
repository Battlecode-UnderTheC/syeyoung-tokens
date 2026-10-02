# gst-endgame-4 (arena round 9, team endgame)

## Idea
gst-endgame-3 (escape runs) plus one behaviour: **a big dragon (length >= 10) takes a free-step
harvest run** when its free steps (2..ceil(L/4), at most 5) can eat more pearls in view than the
lookahead's one step. The run must end on a pearl it eats, out of every shorter enemy's reach, with
room (search7's RoomPenalty 0); among equal pearl counts the shorter run wins. Same gates in
plugin.cpp as the escape (from r250; not when feeding, trapped, farming or walking to an ally;
gst's memory must say the path is free). It reuses gst-endgame-3's run search and body rebuild.

## Why
The late game is decided by the longest dragon. Our longest has a pearl in view on most late
turns but gst and the lookahead move one step a turn, while under 1.2.3 a 20-long dragon has 5 free
steps. gst-sprint-1 (an earlier sprint) only sprinted gst's "toward goal" moves, which a late
dragon of the gst-search line almost never makes (the lookahead steers them), so it never tested this.

## Measured (sim 1.2.3, both seats)
| opponent | maps, seeds | gst-endgame-4 | gst-search-8 as A |
|---|---|---|---|
| gst-search-8 | server 1-5 | 54-0-46 (longest 36-25, +1.3 longest at end) | - |
| gst-search-8 | tournament-sprint 6-8 (held out) | 22-0-20 (+6.2 longest) | - |
| gst-search-8 | official 6-8 (held out) | 53-0-37 (longest 28-14, +6.0 longest) | - |
| gst-giant-2 | server 1-5 | 62-0-38 (longest 41-16) | 57-0-43 |
| gst-queen-guard-2 | server 1-5 | 56-0-44 (longest 39-16, queen 0-8) | 45-0-55 |

The harvest run fires about 30 times a match. `check` passes on maps/server.

(gst-endgame-3's notes follow.)

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
