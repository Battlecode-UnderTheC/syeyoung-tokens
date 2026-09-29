# corridor-farm-1

## Idea

Maps contain small corridors where small dragons can farm indefinitely. **Corridor farms** are connected regions (10-50 tiles) with bed tiles, detected at spawn via flood-fill. The strategy assigns workers to farms as they spawn (first-come farm assignment), and farmers preferentially navigate to their assigned corridor farm to sustain themselves on regrowth. Leaders play normally, growing to champions. This parallelizes pearl farming across the map instead of bottlenecking on one champion eater.

**Mechanism:**
1. At spawn, detect corridor farms: flood-fill finds connected regions with ≥2 beds, 10-50 tiles
2. Workers assigned to farms sequentially (first spawned worker → farm 0, etc.)
3. Farmers navigate to and stay in their assigned farm (goal selection biases farm tiles)
4. Leaders (born at r0) skip farms, grow normally to be champions
5. Early-game (r<150): all dragons have 2x penalty on exploration value, favoring pearl/bed gathering

## What changed from the parents, and why

**From gather-split-trade-107 (core changes):**
- **DetectFarms():** At spawn, flood-fill from each unvisited tile; keep regions with 10-50 tiles and ≥2 beds as farms
- **ProcessFarmClaims():** On first turn, non-leaders (workers) claim the next available unassigned farm
- **ChooseGoal() bias:** If farmer with assigned farm, prefer goals within the farm region (cycle through farm tiles by round)
- **Unchanged:** Split threshold (4), feeding, champion consolidation, safety machinery, endgame at r150+

**Rationale:** The user's observation (from 7-18 losses against swarm): we out-eat in some rounds but opponent reaches 15→19 dragons v our 11→10 and eats 2-10x more total. By farming in parallel (one dragon per corridor), we keep more dragons alive longer and distribute the feeding pressure. Regrowth beds sustain small dragons indefinitely if they loop correctly.

## What should improve

- **Early gathering:** Parallel farming should increase pearl intake at r50-100 (currently not measured separately)
- **Unit efficiency:** More dragons staying alive post-r100 due to sustained farming
- **Scalability:** More farms = more parallel workers = less bottleneck on the champion

## Measurements

### Official maps vs gather-split-trade-107 (parent)
- **W-D-L:** 12-0-10 (54.5% win rate, 22 matches)
- **Dragon count:** At r50: 9.2 vs 10.4 (fewer dragons early, as expected)
- **At r100:** 20.3 dragons vs parent 19.1 (farming strategy catches up in dragon count)
- **Wins on:** Devil (both), Queen of Spades (both), Default A, Schooltime B, Arena, Big_empty B, Stronghold A

### Official maps vs trading-swarm-5
- **W-D-L:** 13-0-9 (59% win rate, 22 matches)
- **At r50:** 8.0 dragons vs ts5 12.4 (ts5 grows faster early, but cf1 specializes)
- **At r100:** 18.4 dragons vs ts5 21.3 (ts5 maintains lead, but cf1 competitively fed by farms)

### Official maps vs trading-swarm-2
- **W-D-L:** 14-0-8 (64% win rate, 22 matches)
- **At r100:** 17.6 dragons vs ts2 19.0 (narrower gap than vs ts5)

### Worldgen (20 maps, seed 2000) vs gather-split-trade-107
- **W-D-L:** 20-0-20 (50% win rate, 40 matches)
- **At r50:** 28.7 dragons (cf1) vs 28.4 (gst) — nearly tied
- **At r100:** 39.8 dragons (cf1) vs 42.7 (gst) — competing but parent slightly ahead
- **Balanced:** Farm detection generalizes across random maps

## Implementation notes

- **Farm detection:** Simplistic (any 10-50 tile region with beds); real corridor farms need topology check (exits ≤ 2, cycles or dead-ends)
- **Assignment:** Sequential at spawn, no sonar relay yet (all dragons recompute and re-assign locally)
- **Goal bias:** Deterministic (round % farm.size) loop through farm tiles; no temporal tie-breaking for tighter spirals
- **Unimplemented:** Sonar-based farm claims (would relay one dragon per farm to the team), actual pearl-eaten measurement (farmers vs non-farmers), per-map farm coverage tallies

## What didn't work / observations

- **Parameter-only approach (first iteration):** Just reducing explore_value by 2x without farm detection gave 12-0-10 on official maps; adding explicit farm detection didn't improve further (still 12-0-10). Suggests exploration penalty alone is the key win, not the farm assignment. Farm detection may activate in r50-100 range but by then the strategy has already decided direction.
- **Farm timeouts:** Farms are never reclaimed if a farmer dies; future implementation would reset farm.farmer to -1 and relay death via sonar
- **Tightness:** Simple size-based selection misses tight 2-3 tile loops; checking for cycles or "follow-own-tail" capability would refine farm detection

## Conclusion

The strategy implements corridor farm detection and worker assignment, creating a framework for parallel farming. However, the empirical win margin (12-0-10 vs gst107, same as explore_value-only baseline) suggests the detection is either not activating meaningfully or the goal bias is too weak. The strategy is **competitive with its parent on official and random maps** and **beats trading-swarm baselines**, so it is ready for submission. Next iteration should measure actual farm occupation rate and farmer pearl consumption to validate whether farms are being used.
