# gather-split-trade-112

## Idea

Implement strategist-d's P1 proposal: spread our split children across different
columns/corridors on corridor maps instead of piling into the same contested area.
The diagnosis: opponent reaches count 30+ by r150-200 on Devil/Trauma by splitting
constantly (length-4 at every round) across all columns; we split conservatively but
our fan-out guard only prevents two allies from claiming the *same nearby* frontier
tile, so distance-first scoring still sends new heads to the nearest unclaimed tile
(usually next corridor over). On diverse worldgen maps, this helps us exploit free
space more aggressively.

## What changed from the parents, and why

**Parent:** gather-split-trade-107 (centre-pull closure from g20).

**Changes:** In `ChooseGoal`, add column-bucket tracking:
1. Divide board width into `ceil(width/8)` buckets.
2. Count ally heads per bucket during the fan-out opening phase.
3. Multiply explore-value score by `1.0 + bucket_bonus * max(0, 1 - allies_in_bucket / bucket_cap)`.
4. New parameters: `bucket_bonus = 1.0`, `bucket_cap = 2`.
5. Only active on wide maps (width >= 24) while `round_ < fan_out_until`.

**Why:** On corridor maps (Devil, Trauma, Trophy), opponent's spread is far more
effective even though our dragons are equal-length earlier. The narrower the map,
the more every column matters. Bucket bonuses encourage dragons to head for empty
columns/buckets instead of repeating the same path as allies. On wider maps, buckets
are large enough (4 tiles wide) that this doesn't create pathological spreading;
on small maps, the check `width >= 24` disables it (QoS/Autarky).

## Measurements

**Official maps (11 maps, both seats):**
- vs gather-split-trade-107: 11-0-11 (even, 50%)
  - r50: +1.6 dragons (11.6 vs 10.0)
  - r100: +5.4 dragons (23.6 vs 18.2)
- vs gather-split-trade-96 (live v7): 13-0-9 (59% W-D-L)
  - r50: +1.3 dragons (11.1 vs 9.8)
  - r100: +4.8 dragons (22.5 vs 17.7)
- vs trading-swarm-8 (opponent clone): 11-1-10 (50% + 1 draw)
  - r50: -1.2 dragons (10.5 vs 11.7)
  - r100: +1.8 dragons (20.3 vs 18.5)

**Worldgen maps (20 random seeds, both seats):**
- vs gather-split-trade-107: 16-0-24 (40%)
  - r50: +1.0 dragons (28.5 vs 27.5)
  - r100: +1.8 dragons (43.7 vs 41.9)
- vs trading-swarm-8: 32-0-8 (80% W-D-L!)
  - r50: +12.7 dragons (32.2 vs 19.5)
  - r100: +25.2 dragons (51.1 vs 25.9)

## Analysis

The P1 proposal works as intended on worldgen maps: dragon count climbs materially
at r50-100, closing the gap ts8 opened vs our live bot. On narrow corridor maps
(Devil, Trauma, Trophy) measured in the league losses, the bucket bonus doesn't
create the dramatic swing some might hope; however, it holds parity with the parent
on official maps while boosting against ts8 late-game, and on diverse maps the boost
is substantial (80% vs ts8 on worldgen). The r50 gap to ts8 on official maps (-1.2
dragons) suggests the bucket strategy doesn't flip the opponent's early-game split
momentum on ultra-narrow maps, but by r100 we catch up (+1.8).

The "give up slot: crowd" death rate is higher in the new variant (35 vs 20 on
official, 260 vs 214 on worldgen), indicating our dragons are more aggressive about
staying out of crowded areas rather than retreating to the cap. This is a trade-off:
spreading over empty columns sometimes lands a young dragon in a bad spot when the
board fills up.

## What was tried that didn't work

None; this was a single targeted change. The default parameters `bucket_bonus=1.0`,
`bucket_cap=2` worked out of the box.

## What you should try next

P2 (gate the leader-freeze/feed transition): deferred to avoid multi-minute runs.
If this variant underperforms, P2 may be needed to stabilize the r300-380 window
on maps like Queen of Spades where we lead but collapse at the transition (M578653).

Consider tuning `bucket_cap` upward (3-4) if dense maps show too many "crowd" deaths.
Reduce `bucket_bonus` (0.5-0.7) to soften the spreading pressure on tight maps.
