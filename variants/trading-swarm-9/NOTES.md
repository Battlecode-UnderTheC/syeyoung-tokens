# trading-swarm-9

## Idea

Our sparring clone of the real server opponent, re-fitted on the faithful engine (judge pearl
seed). Every change comes from the 45 real server games in `server-seeds.tsv`, replayed locally
with the same map, seed and seats: gst90 (10729) or gst96 (11400) against the clone. The clone's
per-game numbers are then compared with the real opponent's, decoded from the server replay.

## What changed from trading-swarm-8, and why

1. **SPLIT L-2 at every length from 4** (`kSplitAll`). In 4 server games the split shapes
   (parent length, child length) were (4,2) 904, (5,2) 68, (5,3) 50, (6,4) 24, (7,5) 16,
   (8,6) 18, (10,8) 17, (12,10) 17, and so on up to (44,42). The head keeps 2 segments and
   the long tail walks off reversed, so long starting dragons shrink to 2-4 long within 25
   rounds. ts8 split only at lengths 4-7 (plus a 1/25 roll), so its long starters stayed
   13-21 long at r25 on Autarky, Slithery Fight and Dilemma. Clone r25 longest: 5.1 -> 3.2
   (real 3.9).
2. **No waiting at home beds** (`kNoHomeBeds`; portal-only room beds are still farmed). With
   this, our bot's eating rate against the clone matches the server per century:
   108/163/165/144 vs real 111/161/156/146. ts8 let our bot eat 107/175/197/187.
3. **Mass feed from r400** (`kMassFeed`, `kFeedRound = 400`). In 4 server games the real
   opponent resigned 151 dragons: none before r400, 57 in r400-405, lengths 2-22, always
   shorter than the team's longest. In 104 of the 151 the nearest longer ally head was 1-2
   tiles away. Its count drops from about 50 to about 15 by r420 while its longest climbs
   from 8 to 20-40. The clone now resigns when an ally head that is longer than it is within
   Manhattan distance 2. ts8's L<=5 feed from r350 is kept.
4. **Room guards.** When drifting, the clone prefers steps whose window flood holds at least
   min(L+2, 8) tiles; it falls back to 3 as before. A plain first step toward a pearl or bed
   that leads into a pocket smaller than the body (capped at 8) is swapped for the roomiest
   other free step.

## Fidelity: per-game agreement (clone-eval/, 45 server games, exact map, seed and seats)

| | ts8 | ts9 | real |
|---|---|---|---|
| result agreement | 20/45 | 21/45 | |
| opponent record (W-L) | 12-33 | 15-30 | 31-14 |
| r25 count/longest/total | 6.9/5.1/18.9 | 8.6/3.2/20.6 | 8.0/3.9/20.1 |
| r50 | 11.4/4.8/29.4 | 12.4/3.4/30.2 | 13.1/3.7/31.8 |
| r100 | 16.4/4.2/40.8 | 17.8/3.4/43.7 | 20.5/4.5/52.6 |
| r200 | 19.6/3.9/48.9 | 23.5/3.6/59.3 | 31.6/5.3/84.6 |
| r300 | 19.7/3.6/49.0 | 24.5/3.6/60.6 | 39.1/7.4/115 |
| r400 | 13.8/10.2/45.2 | 18.5/9.5/57.8 | 28.0/14.1/110 |
| r450 | 16.5/12.7/54.0 | 10.2/10.6/37.5 | 10.8/22.7/89.7 |
| final | 13.7/10.6/46.2 | 9.0/7.9/30.9 | 17.4/21.6/95.1 |

Mean absolute error on count: r50 4.7 -> 3.5, r100 9.5 -> 7.5, r200 15.5 -> 15.7.
Full per-game tables: `clone-eval/ts8-vs-server.txt` and `ts9-vs-server.txt`.

The results still disagree on these games: every Queen Of Spades game (5 of 5; our bot wins
in sim but lost on the server), Schooltime (4 of 5), Portals side A, and Devil M572144/M588433.

## What is still wrong (for the next clone task)

- **Mid-game growth.** The real opponent reaches 32 dragons at r200 and 39 at r300; the clone
  reaches 23-24. The clone eats *more* than the real one (168/235/238 vs 120/197/207 per
  century), but it also dies more. Deaths per century: clone 57/93/96, real 33/67/71. Most
  of the clone's deaths are traps: in r0-199, wall + self deaths were 100 for the clone
  against 47 walls/selves plus 18 failed-split deaths (0 free neighbours) for the real one.
  The stronger room guard for pearl steps (min(L+2, 8)) matches the real death and split
  rates per century almost exactly, but it cut the clone's eating to below the real one's
  (112/162/163), so it was not kept. The next lever is "eats like ts9 and dies like the
  guard".
- **The late champion.** The real longest is 14-17 at r400 and 22 by r450; the clone's is
  9.5 and 10.6. Moving the freeze/feed rounds earlier (270/300) didn't help.
- The real opponent's trapped short dragons die by an invalid SPLIT (`split_fail`, 0 free
  neighbours). This is equivalent to the clone's "stuck" move into a wall.

## Measurements (faithful engine)

- `sim --seed 1,2` official maps vs trading-swarm-8: 20-4-20. 20 worldgen maps (seed 26100)
  vs trading-swarm-8: 24-0-16.
- gst96 vs clone, all 15 bundled maps, seeds 1,2: vs ts9 39-2-19, vs ts8 39-1-20 (the same
  strength).
- `check`: all checks passed. Timing was taken at load average ~150: the parent showed the
  same inflated maxima, and p50 per turn is ~18 us for both.

## Harness (clone-eval/)

- `real.py`: decode the 45 server replays into `real.json` (~80 s).
- `snap.cpp`: build against `build/libplay.a` and `build/libshared.a` into `build/tools/snap`.
  It plays one game and prints snapshots, eaten, splits and deaths per century.
- `cmp.py <clone> [-v]`: replay all 45 games against the clone and print the agreement table.
- `fatal.py <clone>`: the clone's fatal reasons.

The official maps missing from `maps/official` (Autarky, Portals, Prisoners Dilemma, Slithery
Fight) are in the unswbc 1.2.2 bundle:
`~/.local/share/uv/tools/unswbc/lib/python3.12/site-packages/unswbc/templates/maps/`
(dilemma.map = Prisoners Dilemma). The paths in the scripts point at this task's worktree
`build/`.
