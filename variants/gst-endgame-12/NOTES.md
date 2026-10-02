# gst-endgame-12 (arena round 9, team endgame): the queen plays safe

## Change (one behaviour: minimum queen safety, gst's economy untouched)
gst-endgame-9 (= gst-search-7) where our queen (dragon 0 or 1):
- never feeds, kamikazes, lunges, hunts a trade or gives up her slot; the judge may not lunge her
  and its single-step overrides, like the late lookahead's, need a queen-safe step;
- never takes a dead-end goal or step (no pockets, no hot-bed farming), needs 20 tiles of room;
- treats as exposed any tile a visible enemy head can reach next turn (1.2.3 reach L-2+ceil(L/4),
  any length: a trade for our queen is good for anyone) and any corridor tile (kelp on 2+ sides);
- hides: an enemy head seen in the last 3 rounds within 6 sends her the roomiest safe way away;
- allies keep off her: a non-queen step ending within 2 of her head takes another free step.
Her growth splits stay gst's (an ablation where she kept 4 and split the rest off cut our dragons
at r50 from 14.2 to 11.3: the queen's early lineage is a big share of the opening).
No escape/harvest runs for the queen.

## Why
gst-search-7's queen dies in 97% of arena games (median r80); queen-decided games go 8-223. Traced
vs qg-board-1 (queen-guard-7) locally: her deaths were trapped 2-long stubs (corridors/pockets, an
ally coming head-on in a 1-wide lane on devil), lunges and kamikazes on a 2-4-long queen, and the
three doomed-spawn maps (slithery, dilemma, autarky: head already in a dead end at r1; unchanged).

## Measured (sim, both seats)
| matchup | maps, seeds | gst-endgame-12 | gst-endgame-9 (base) |
|---|---|---|---|
| vs gst-endgame-9 | server 4-8 | 51-49 (queen 5-1; queen dead 92% v 98%) | - |
| vs gst-endgame-9 | sprint 1-3 | 22-20 (queen 6-0; dead 83% v 98%) | - |
| vs qg-board-1 | server 4-8 | 49-1-50 | 47-53 |
| vs qg-board-1 | sprint 1-3 | 11-31 | 18-24 |
r100 economy vs base: server 21.1 v 21.7 dragons, sprint 25.7 v 26.2. `check` passes.

(gst-endgame-9's notes follow.)

# gst-endgame-9 (arena round 9, team endgame)

## Change
gst-endgame-4 with a fix: escape and harvest runs only cross tiles inside this turn's 7x7 view.
A 5-step run could leave the window, where search7 knows nothing is free. Seen in sim vs gst-queen-3
(default, seed 1): our 45-long champion, ahead 45 to 26 at r490, ran head-first into one of our own
3-long dragons out of view and the game was lost.

## The queen-3 matchup (arena: gst-search-5 and gst-search-6 both 36-42)
- On the same 78 arena games (map, seed, seat), gst-search-8 went 42-36 and gst-search-6 36-42:
  19 flipped to losses, 13 to wins. The flips include eliminations before r200 (prisoners_dilemma,
  trophy), before any of our changes act (r250). They follow the seat both ways, so the arena runs
  are not deterministic; the 46% is within noise.
- Local sim does not show a queen-3 problem:

  | maps, seeds | gst-endgame-9 vs gst-queen-3 | gst-search-8 vs gst-queen-3 |
  |---|---|---|
  | server 1-10 | 105-0-95 (longest 58-49) | 110-0-90 |
  | tournament-sprint 1-5 | 42-0-28 (longest 25-18) | 35-0-35 (longest 18-24) |

- Arena losses to queen-3 by longest dragon mostly start behind on material at r300-400 (total
  78 v 89 at r300); that is economy, not the endgame. Three were "longer queen" (schooltime).
- Escape and harvest runs are fatal less than 0.05 times a match.

vs gst-endgame-4, server seeds 1-10: 102-0-98. `check` passes.

(gst-endgame-4's notes follow.)

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
