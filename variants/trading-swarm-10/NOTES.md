# trading-swarm-10

## Idea

This is trading-swarm-9, the clone of the real server opponent, with one fix to how it is
measured and one change for the late game.

## Measurement fix: the server shuffles spawns

The judge's maps are not always the bundled files. In 27 of the 45 server replays, the
replay's map text has the team codes of the DRAGON lines swapped compared with
`maps/official`: seat A starts where the file puts team B. In 3 Prisoners Dilemma games
the server map has 10 dragons, not 6. So replaying "the same map, seed and seats" from the
bundled file put our bot on the wrong spawns in more than half the games.
`clone-eval/gamemaps.py` builds one map per game: the official file with the replay's DRAGON
lines. Bed ranges come from the file, because the replays zero them; the edges are identical
in all 45. `cmp.py` and `fatal.py` use these per-game maps.

**This affects the whole league.** `sim`/`match` reproduce the judge on a *given* map file, but
server games may use a mirrored spawn assignment or a different dragon set. Seat effects on
small maps (KNOWLEDGE: "Small official maps are decided by the seat") may therefore look
different on the server.

## Change from trading-swarm-9

**No splitting from r400** (`kNoSplitRound`). The real opponent's splits fall to about 23 per
century in r400-499. The r400 mass feed then turns corpses into long dragons instead of new
2-long ones. Clone longest at r450: 14.1 -> 17.2 (real 21.4). Final longest: 10.5 -> 15.0
(real 21.6).

Tried and dropped: **split only when both halves can move** (a head step with room and a free
tail neighbour). The commonest fatal chain was split -> step -> stuck, but the guard cut
splits more than deaths: r200 count went 22.7 -> 18.9. The deaths moved to reverse-split ->
stuck.

## Fidelity (45 server games, per-game maps, exact seed and seats)

| | ts8 | ts9 | ts10 | real |
|---|---|---|---|---|
| result agreement | 14/45 | 22/45 | 21/45 | |
| opponent W-L | 12-33 | 14-31 | 15-30 | 31-14 |
| r50 count/longest | 10.8/5.4 | 12.8/3.5 | 12.8/3.5 | 13.1/3.7 |
| r200 count/longest | 16.8/3.5 | 22.7/3.6 | 22.7/3.6 | 31.9/5.7 |
| r300 count/longest | 17.4/3.5 | 22.6/3.6 | 22.6/3.6 | 38.9/7.5 |
| r450 count/longest | 13.5/12.6 | 9.0/14.1 | 5.7/17.2 | 11.0/21.4 |
| final longest/total | 9.2/41.0 | 10.5/34.3 | 15.0/52.5 | 21.6/95.1 |

The per-game tables are `clone-eval/ts8-vs-server.txt`, `ts9-vs-server.txt` and
`ts10-vs-server.txt`.

## What is still wrong

**The mid-game count** is 23 against the real 32-39 at r200-300. The clone eats *more* than
the real opponent (172/252/249 per century vs 120/197/207) and splits more (73/103/102 vs
51/76/76), but it also dies much more (58/100/101 vs 34/67/71).

On Schooltime M588437 the real opponent reached 47 dragons by r100 with 1,626 dragon-turns
in r50-99, against the clone's 707. The clone eats more per turn (0.13 vs 0.11) and has a
pearl in view more often (79% vs 56%). The real opponent wins by having more dragons, not by
finding pearls better. Its lead starts at r20-40: 6 pearls by r30 against the clone's 2, then
a snowball. Its early dragons spread across the whole top edge; the clone's cluster.

The clone's fatal chains, per game:

| chain | per game |
|---|---|
| pearl -> pearl -> stuck | ~40 |
| split -> pearl -> stuck | ~48 |
| pearl -> split -> stuck | ~38 |
| newborn child stuck on its first turn | ~33 |

## Measurements

- `sim --seed 1,2`, 15 official maps, vs trading-swarm-9: 34-6-20. 20 worldgen maps (seed
  26600) vs trading-swarm-9: 24-0-16.
- gst96 vs clone, 15 official maps, seeds 1 and 2: vs ts10 35-2-23, vs ts9 39-2-19. The real
  server series was 14-31.
- `check`: all checks passed. Slowest turns in `check` read 2-18 ms, but the machine's load
  average was 60-150 (ts9 read the same). Re-time on an idle machine.
