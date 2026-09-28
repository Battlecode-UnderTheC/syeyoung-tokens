# late-surge-7

Mutation of late-surge-5 (task g7-6): a size-aware surge for big maps, where late-surge-5's
feeders can't path to a champion standing in territory they never explored.

## Idea

Two-stage surge. Stage 1 (`regional`): a feeder with no known path to the champion feeds the
nearest reachable ally clearly longer than itself (a regional champion) instead of gathering on.
Stage 2 (`converge_from`, `converge_min_length`): late, only the long dragons with no known path
walk towards the champion's reported area (late-surge-5's `seek_champion` mechanism), so the short
ones keep gathering. Also `big_map_advance`: start the surge earlier when W + H >= 96.

**Committed defaults: stage 2 only** (converge from round 440, length >= 10; regional off,
advance 0). Stage 1 measured clearly worse; see below.

## What changed from the parent

- namespace/macros ls5 -> ls7.
- Params `regional`, `regional_margin`, `regional_ratio`, `regional_age`, `regional_reach`,
  `regional_far`, `converge_from`, `converge_min_length`, `big_map_sum`, `big_map_advance`.
- Decide: step 1b (regional feed: "feed regional" reason; the target is the reachable tile next to
  the ally's head, resign when that ally is in view within `feed_distance`), and `converge` enables
  the seek walk for long feeders from `converge_from`.

## Measurements (sim, both sides; worldgen = 20 fresh maps, 40 matches)

| config | vs ls5 worldgen | vs ls5 official | vs gst27 official | vs gst27 worldgen |
|---|---|---|---|---|
| late-surge-5 (parent) | 20-0-20 | 11-0-11 | 9-0-13 | 20-0-20 (+4.5 longest) |
| regional (margin 2) + converge | 17-0-23 (-4.7 longest, -73 total) | 11-0-11 | 11-0-11 | |
| + regional only when champion >= 16 torus away | 15-0-25 | 10-0-12 | | |
| regional only (no converge) | 13-0-27 | 12-0-10 | | |
| regional (>= 2x our length) + converge | 18-0-22 | 12-0-10 | | |
| **converge only (committed)** | **20-0-20 (+1.9 longest, -10 total)** | **12-0-10** | **11-0-11** | **20-0-20 (+1.9 longest)** |
| converge + big_map_advance 30 | 20-0-20 (+2.7 longest) | 12-0-10 | | |

Official vs gst27 per map (committed): big_empty 1-1 (parent 0-2), stronghold 1-1 (parent 1-1),
default 0-2 (parent 0-2). vs ls5: big_empty 1-1, stronghold 2-0, default 1-1.

## What should improve

Big maps (big_empty): the long dragons that can't find a path now walk to the champion late,
where late-surge-5 left them gathering. Small and medium maps are almost unchanged (converge only
fires for dragons >= 10 with no known path).

## What didn't work

- **Regional champions hurt everywhere I measured** (worldgen -4 to -9 longest vs the parent).
  A feed pays only ~half the feeder's length, so a two-hop surge (feeder -> regional -> champion)
  pays about a quarter, and the regional feeds also fire on medium maps where the path to the
  champion is only momentarily blocked (requiring the champion to be >= 16 away made it worse,
  not better, so that's not the only cause). Growing one big regional dragon also doesn't beat the
  enemy's longest unless it reaches the champion, which it often doesn't in time.
- big_map_advance 30: within noise (+0.8 longest on worldgen, same official record).

## Next

- Converge thresholds (440 / 10) are unswept; try earlier converge on big maps only.
- Instead of regional feeding, let a big regional dragon be the champion-of-record for its area
  (it keeps growing and competes for longest) rather than being fed.
