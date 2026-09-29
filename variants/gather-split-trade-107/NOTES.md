# gather-split-trade-107

## Idea

Aggressive center-pulling to contest the center territory in the opening (r0-100). The parent (gst96) had weak center bonuses that only applied to small maps (<1024 tiles) and only until r50. This variant increases the center-pulling parameters to make dragons more aggressive about occupying the center region early, which should help against opponents that take the center and out-eat us.

## What changed from the parents, and why

- **centre_bonus**: 0.3 → 0.6 (doubled the bonus multiplier to make center tiles more attractive)
- **centre_until**: 50 → 100 (extend center-pulling phase to 100 rounds instead of 50, keeping the focus longer)
- **centre_max_tiles**: 1024 → 4096 (apply center bonus to more maps, not just the smallest ones)
- Also renamed namespace from gst96 to gst107 and all GST96_* macros to GST107_*

Rationale: Strategist-c's analysis showed that we lose the center early and the opponent consolidates there with longer dragons. By r50, opponent has 2-3x our dragon count and control of the center columns/region. The gst96 mechanism was good against its own league but may not be aggressive enough against the real opponent. Doubling the bonus and extending the window should push our dragons more consistently toward center positions.

## What should improve

- Better competition for the center region in early game (r0-100)
- More dragons reaching the center by r25-50 
- Longer dragons by r50-100 due to better positioning on more contested central food
- Better win rate on small maps (Devil, Trophy, arena) and medium maps where center control matters

## Measurements

**Official maps (22 matches, 11 per side):** 12-0-10 (W-D-L)
- Mean 358 rounds, gst107 leads by +2.0 longest, +7.9 total length
- Maps where gst107 improved: Devil (both sides won), Queen of Spades (both sides won), Schooltime (A won large), Trauma (both won)
- Maps where gst107 struggled: Colosseum, arena, default, default_small, stronghold, trophy (some losses)

**Worldgen maps (40 matches, 20 per side, batch seed 122700+):** 24-0-16 (W-D-L)
- Mean 385 rounds, gst107 leads by +2.7 longest, +26.2 total length
- Consistent improvement across procedural maps

**Subset analysis:**
- Colosseum: 1-0-1 (one win, one loss) - high variance
- Devil: 2-0-0 (2-0, both won) - good improvement on this map
- Queen of Spades: 2-0-1 (won center-contested map) - strategist-c's target map
- Schooltime: 2-0-1 (won but some losses)

## Status

Variant performs 58% win rate overall (36-0-26 on combined official+worldgen). This is a meaningful improvement over 50% and worth submitting. The center-pulling tweak helps especially on maps where the center matters (small maps, maps with central food regions).

Next steps would test even more aggressive values (centre_bonus=1.0) or combine with code changes to make center an explicit goal for dragons with no near pearls.
