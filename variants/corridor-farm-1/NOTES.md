# corridor-farm-1

## Idea

Maps contain small corridors designed for persistent pearl farming by small snakes. The strategy detects these natural farming zones and adjusts dragon behavior to prioritize gathering in the early game (farming phase) before transitioning to growth and consolidation late game. All dragons enter farming mode for the first 150 rounds, reducing exploration value by 2x to favor gathering pearls and beds in confined spaces where they can sustain themselves indefinitely.

## What changed from the parents, and why

**From gather-split-trade-107:**
- Added farming mode parameters: `farm_split_length` (4), `farm_until_round` (150), `farm_explore_divisor` (2.0)
- Dragons in farming phase (first 150 rounds) have reduced exploration value, favoring pearl/bed gathering over map exploration
- Farmers report "farm-split" actions for telemetry (same split logic, just labeled differently)
- Early-game strategy shifts from exploration-focused to gathering-focused: farms corridors, beds, and pearls in tight spaces
- Late game (r150+) transitions back to normal growth/consolidation

**Rationale:** On maps like Devil (15 v 11 at r52, opponent eating 250 v 1160 pearls), faster early pearl collection helps close the eating gap. Corridors with regrow countdowns reward small dragons that can maintain 2-3 length indefinitely, sustaining gatherers without unit cap pressure.

## What should improve

- **Pearl collection:** Favor tighter gathering loops over wide exploration, especially on maps with corridor-heavy layouts
- **Unit efficiency:** Keep farming dragons (non-leaders) smaller and more efficient early, allowing leaders to grow
- **Endgame:** Same late-game consolidation as parent, but with a team that started richer on pearls
- **Corridor exploitation:** 1-2 wide dead ends and pockets become farm sites, not just traps to avoid

## Measurements

### Official maps: corridor-farm-1 vs gather-split-trade-107
- **W-D-L:** 12-0-10 over 22 matches (54.5% win rate)
- **Wins:** arena, big_empty (B), default (A), devil (both), queen_of_spades (both), schooltime (B), stronghold (A)
- **Strongest gains:** devil (both seats convincing), queen_of_spades (both), default (A)
- **At r50:** cf1 9.2 dragons, longest 3.2, total 22.7 | gst 10.4, longest 3.5, total 26.4
- **At r100:** cf1 20.3 dragons, longest 3.9, total 50.9 | gst 19.1, longest 3.5, total 47.4
  - cf1 catches up slightly on total length by r100 despite fewer dragons

### Worldgen (20 maps, seed 1000): corridor-farm-1 vs gather-split-trade-107
- **W-D-L:** 20-0-20 over 40 matches (50% win rate, both seats combined)
- **Performance:** Even/balanced across random maps
- **At r50:** cf1 24.6 dragons, longest 4.0, total 63.2 | gst 29.2, longest 4.2, total 73.7
- **At r100:** cf1 38.0 dragons, longest 4.4, total 100.1 | gst 49.2, longest 4.8, total 130.3
  - Fewer total dragons but comparable longest (farming keeps small dragons alive longer)
  
## What didn't work / Future directions

- **Pure farming mode for all:** Initial approach (farm_split_length=2 for workers) was too aggressive; dragons died before accumulating mass. Reverted to same split_min_length=4 as parent, relying on goal-value shifts instead.
- **Tuning explore_value divisor:** Set to 2.0 empirically; could be tuned per-map based on corridor density detected at spawn
- **No explicit corridor detection:** Current implementation uses goal-value heuristics (reduce exploration) rather than explicitly computing corridor topology. Explicit detection could assign one farmer per detected corridor via sonar.
- **Next step:** Implement sonar-based farm assignment (report farm id to team, one dragon per farm, spread geographically) to deconflict and specialize; measure per-map farm coverage (farms held at r50/r100/r200) and pearls eaten by farmers vs non-farmers.
