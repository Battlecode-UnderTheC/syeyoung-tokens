# mapgen: procedural maps for dragons

Our bots overfit to the 17 server maps, and the arena's "rotating" suite (xCirno1's
worldgen.py) produces maps that look alike, often with a dense pearl cluster in the
middle. `mapgen.py` makes maps with deliberately different structure, freshly for each
evaluation, so no bot can be tuned to a fixed set.

It is a single file, Python 3 standard library only, and it never reads the environment.
For a given family, seed and index it always writes the same map.

```
tools/mapgen/mapgen.py --family maze --seed 7 --out /tmp/maps --count 5   # 5 maze maps
tools/mapgen/mapgen.py --family all  --seed 7 --out /tmp/maps --count 22  # 2 of every family
tools/mapgen/mapgen.py --seed 7 -o /tmp/one.map       # one map, family = FAMILIES[seed % 11]
tools/mapgen/mapgen.py --describe /tmp/maps maps/server  # feature table (works on any .map)
tools/mapgen/mapgen.py --show /tmp/one.map            # ASCII picture: # kelp, @ portal, o bed
tools/mapgen/fresh.sh 44 $(date +%s) /tmp/fresh       # a fresh set across all families
```

## Families

| family | structure | variants (in MAP_NAME) |
|---|---|---|
| `open` | a few short walls (0-3% of edges), beds on a jittered grid: even everywhere, no central cluster | `even` |
| `rich-near-home` | ~85% of beds within r steps of a side's starters and at least 3 steps nearer them than the enemy; ~8% contested | `r<radius>` |
| `maze` (alias `corridor`) | spanning-tree maze over every tile (1 wide) or 2x2 blocks (2 wide), 0-35% of walls knocked out for loops; or long parallel 1-wide stripes with gaps; or a maze over half the board | `maze`, `maze-wide`, `stripes`, `partial` |
| `rooms` (alias `islands`) | the board cut into a grid of rooms by full kelp lines; rooms joined by 1-tile doors and/or by portals only | `<nx>x<ny>-p<portal links>` |
| `sealed-spawn` | each queen starts in a 3-5 x 3-4 kelp pen with one exit (`exit1`) or none (`exit0`); the other starters are free outside | `exit<0/1>-<w>x<h>` |
| `ring` (alias `donut`) | beds only on an elliptical band; centre empty, fully walled, or walled with 1-3 doors; nothing starts in the core | `empty`, `walled`, `walled-doors` |
| `off-axis` | beds only at least 20-30% of the short side away from the (torus-shortest) segment between the queens | `thr<distance>` |
| `sparse` | 2-10 beds, slow (gap 30-600) or wide (1..100-3849) respawn gaps | `k<beds>` |
| `crowded` | 6-10 starters per side, length 2-4, on an 11-24 board; sometimes a low UNIT_LIMIT | `n<starters>` |
| `anti-centre` | punishes rushing the geometric board centre: no beds in the central half-box in any variant | see below |
| `mixed` | random terrain x random bed layout, sometimes a pen, a crowd, a UNIT_LIMIT, or a central cluster | `<terrain>+<beds>[+pen][+crowd]` |

`anti-centre` variants:
- `walled`: 2-3 concentric kelp boxes round the centre, with doors on alternating sides, so getting in takes a long detour. Nothing is inside.
- `trap`: the centre is a perfect 1-wide maze (a dead-end pocket) with one door (mirrored). Half the time there is one slow bait bed at its deepest point.
- `edges`: food near each side's starters and along the wrap seams (x or y within 1 of the border), never in the middle.
- `portal`: a food cluster far from the spawns, with 1-2 portal pairs from beside the queen straight into it. Plus a few scattered beds.
- `offcentre`: queens spawn near the wrap seam, so the shortest path between them crosses the seam, not the centre. The food cluster sits on the seam between them.

## Parameters varied per map

- **Size**: 11-64 per side, biased towards mid sizes, square about 40% of the time. Families narrow it: crowded is 11-24, maze 13-48, rooms 18-64, anti-centre 21-64.
- **Symmetry**: `xy` (point reflection) half the time, else `x` or `y`. In the engine, `x` flips y: (x, y) -> (x, H-1-y). `y` flips x. The mirrored dimension is at least 14. Team 1 is always the exact mirror of team 0: walls, portals, beds and dragons.
- **Starters per side**: 1-10, weighted towards 2-5. Queen length is from {2,3,4,5,6,8,10,14}, others from 2-8.
- **Spacing**: queens are at least max(4, min(W,H)/4) apart, and further in some families. Other enemy segments are at least 3 apart.
- **Beds**: density from about 1% to 40% of tiles, depending on the family (the server maps run 7-100%). Gap styles:
  - fast: lo 1-6, hi lo+4..30
  - medium: lo 1-25, hi lo+20..120
  - slow: lo 30-150, hi lo+100..450
  - wide: 1..{100..3849}
  - mixed: a palette of 2-4 of these
  
  Half the maps use one gap for every bed, as the contest maps do.
- **Portals**: usually 0-10 pairs (rooms up to about 20), in most families, not just rooms. A starter's body never spans a portal edge.
- **Kelp**: from 0% of edges (open) up to about 65% (maze).
- **UNIT_LIMIT**: usually the default (64). Crowded and mixed sometimes set it lower, but never below starters + 2.

## Validity rules (checked before a map is written; 60 retries per map, else an error)

These follow `shared/engine/dragons/board.cpp`, the port of the engine's MapReader:
- Every bed equals its mirror's bed, gaps included. Beds and their mirrors share a countdown.
- `1 <= lo <= hi` for every bed.
- Edges use the engine's file index: row 2y holds horizontal edges and 2y+1 vertical ones, with stride W+1. Only kelp and portal edges are written; empty is the default.
- Each portal is two edges with the same orientation and the same id. No edge is in two portals, and no portal edge is also kelp.
- `DRAGON` lines alternate teams, starting with team 0, so dragon 0 and dragon 1 are the queens.
- Every body is a chain of distinct, orthogonally adjacent tiles joined by plain edges (no kelp, no portal). Bodies don't overlap, and no body sits on a self-mirror tile.
- Every head has a free first step.
- The queens can reach each other, and every starter can reach a bed or a queen. The exception is the deliberately sealed pens (`exit0`): the generator keeps a list of those tiles and leaves them closed.
- Every component that holds a bed or a starter is joined to the main one. `repair()` opens kelp (with its mirror) until it is; protected pen walls are never opened.
- Beds never go under a starter or inside a pen.

Beyond the Python checks, generated maps were loaded by the repo's engine port
(`build/tools/sim`, `build/tools/check`) and by the official judge
(`unswbc run <map> <bot> <bot> --sandbox`, unswbc 1.2.9).

## Feature report (`--describe`)

There is one row per map. It works on any `.map`, so you can compare against `maps/server`.

| column | meaning |
|---|---|
| `starters` | dragons per team |
| `qlen` | queen start length |
| `len` | team 0's mean start length |
| `beds`, `bed%` | bed count, beds per tile |
| `gap` | mean of (lo+hi)/2 |
| `kelp%` | share of edges that are kelp |
| `portals` | portal pairs |
| `corridor%` | share of open tiles with kelp on 2 or more sides |
| `spawn` | `open`, or `pen1` (cutting one step leaves the queen in 30 tiles or fewer), or `sealed` (the queens can't reach each other) |
| `qdist` | queen-to-queen walking distance |
| `centre%` | share of beds in the central box (W/2 x H/2) |
| `cluster` | `CLUSTER` when `centre%` is at least 45% (with 4 or more beds) |
| `cover` | share of the 4x4 board blocks that hold a bed |
| `contested%` | beds whose walking distance to the two teams' starters differs by 2 or less |
| `axis%` | beds within 15% of the short side of the queen-to-queen segment |
| `ulimit` | UNIT_LIMIT |
| `name` | MAP_NAME, which carries the family and variant: `gen-<family>-<variant>-s<seed>[-i<index>]` |

To group results by map class, use the family directory that `fresh.sh` makes, or the
`name` column.

## Using it with the local tools

```
cmake -S . -B build && cmake --build build -j 2 --target sim check <variants...>
tools/mapgen/fresh.sh 44 $(date +%s) /tmp/fresh
build/tools/sim --a A --b B --maps /tmp/fresh/all --threads 2          # everything at once
for f in /tmp/fresh/*/; do [ "$f" = /tmp/fresh/all/ ] && continue
  echo "== $f"; build/tools/sim --a A --b B --maps "$f" --threads 2 | grep -A1 ' vs '; done
build/tools/check A /tmp/fresh/all 20                                   # the gate, on fresh maps
```

`sim` prints one line per match only when a run has 64 matches or fewer (maps x seeds x 2
seats). That is why `fresh.sh` also writes one folder per family. Use a new seed for every
evaluation. If a set is reused, it becomes one more fixed suite to overfit.

For arena duels, generate a set and run it as a local `sim` suite. Uploading generated maps
to the arena (its `upload_map` / custom suite) is a team decision: **ask the user first**.

## Using it in tokenz-arena

The arena's rotating and holdout suites call `worldgen.py` through `arena/maps.py`, in
`_generate`. There are 4 calls at a time, each with a 300 s timeout:

```
python worldgen.py --seed <int> -o <path.map>
```

`mapgen.py` accepts exactly that. With no `--family`, the family is `FAMILIES[seed % 11]`,
so any run of consecutive seeds covers every family evenly; the rotating suite's
`e*1000 .. e*1000+19` gets about 2 of each. A map takes at most about 0.3 s. Output is
deterministic per seed, and `MAP_NAME` (`gen-<family>-<variant>-s<seed>`) is unique per
seed, which matters because the arena keys maps by name.

Drop-in steps for the maintainer:
1. Copy `tools/mapgen/mapgen.py` from this repo to the arena host as
   `<data_dir>/tools/worldgen.py`, or host it at a URL of your choice.
2. In `arena/maps.py`, set `WORLDGEN_SHA256` to `sha256sum tools/mapgen/mapgen.py`, and
   `WORLDGEN_URL` to wherever the file is hosted. Or have `worldgen()` read the file from
   the repo instead of downloading it. **The pinned sha256 changes with every edit to
   mapgen.py**, so re-pin after each update.
3. Delete the current rotating epoch folder (`data_dir/maps/rotating/e<N>`) to regenerate
   it now. Otherwise it changes at the next epoch. The holdout folder regenerates only if
   it is deleted. Its salt file can stay, because the seeds are just seeds.
4. Old results on worldgen maps are not comparable with results on the new maps. The map
   names differ (`gen-...`), so the two sets stay apart in the arena's tables.

Test it before switching: run `python3 mapgen.py --seed 12345 -o /tmp/t.map` with
`python3 -I` (no site packages, no environment) and load the result with `unswbc run`.
