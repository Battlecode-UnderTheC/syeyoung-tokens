# trading-swarm-13

## What this task found

**1. The 45-game agreement is noise at the level we have been tuning.**

The clone draws random numbers from a fixed seed. Changing only that seed (`kSalt` 0-3),
with the logic otherwise identical to ts11, gives agreement 20, 24, 18 and 19 out of 45:
mean 20.2 +- 2.3. ts11's reported 26/45 was a lucky draw. Every earlier "gain" of 1-5 games
(ts9 -> ts10 -> ts11, and the ts12 trials) is inside this noise.

`clone-eval/multi.sh <label>` builds four seeded copies and reports mean +- sd of agreement,
clone wins and counts. Use it for any clone change. The r100-300 dragon counts are steadier
(+- ~1.5) than agreement.

**2. Per-turn behaviour comparison** (new tools in `clone-eval/`):

- `realdump.py` decodes every real opponent turn in r0-199 from the 45 replays: length,
  facing, action, pearls, beds with countdowns, ally and enemy heads, occupied tiles, and
  window room for each of the 4 steps. It writes one JSON line per turn.
- `dump.cpp` writes the clone's turns in the same format, from the same games.
- `feat.py` and `deaths.py` compare the two sets.

**Findings, r50-199 (real / clone ts11):**

- **Deaths per turn:** 0.022 / 0.033. The clone eats more per turn (0.082 / 0.101) but has
  fewer turns: 152k / 121k over the 45 games.
- **Where deaths happen:** ~78% of both sides' deaths come from being fully boxed in (no
  free step). The clone is boxed in 3.1% of its moves, the real one 2.5%.
  - When boxed, the real one tries SPLIT about half the time. At length >= 4 this saves it:
    59% of its boxed splits survive.
  - ts11 only reverse-split from length 5.
- **Adjacent enemy head:**
  - The real one steps into it only ~10% of the time; ts11 did so 25-45% ("trade head").
    ts11 died at 0.36 a turn there, the real one at 0.14.
  - With 2+ enemy heads in view the clone dies at 0.100 a turn, the real one at 0.047.
- **Splitting:** when a dragon first reaches length 4 the real one splits 72% of the time,
  like the clone. The low split odds for longer dragons are dragons held at the unit limit
  on crowded maps (Slithery Fight, Autarky), not a random choice. Copying those odds cut
  splits and the count (r200 15.7), so the switch is off.
- **Moves with no pearl in view:** the real one goes straight 52-63% of the time and shows
  no pull toward the centre (0.49). The clone drifts to the centre (0.54-0.67). But
  wandering instead (`kDisperse = 2`) scored worse on the 4-seed test: agreement 17.8,
  clone wins 9.8.
- **Pearl supply feeds back.** On Schooltime M588437 the beds and pearl seed reproduce
  exactly: the first 12 spawns are identical. Afterwards the server spawned 165/140/153/173
  pearls per 50 rounds and the local game 162/123/119/145, because beds refill only after
  their pearl is eaten. The real opponent's faster eating makes more food. So the count
  gap (real 31 vs clone 21 at r200) comes from ~10 fewer splits by r200: 117 vs 126.5.
  Deaths are equal (~100).

## Changes from trading-swarm-11

1. **Takes an adjacent-head trade only 10% of the time** (`kTradeOdds`; ts11: always).
2. **Boxed in at length 4, splits** (`kReverseMin = 4`; ts11 needed length 5).
3. **Portal self-death fixes** found by `check` once the random numbers changed; both are
   latent in ts11. After crossing a portal within L+1 rounds, the clone does not route or
   explore through a portal whose landing is out of view: its own body may be there.

## Measurements (4 seeds each, 45 server games, per-game maps)

| | agreement | clone wins | count r100/200/300 | final longest |
|---|---|---|---|---|
| ts11 logic | 20.2 +- 2.3 | 15.8 | 16.3/21.0/22.8 | 13.9 |
| trade 10% | 22.5 +- 1.8 | 17.5 | 16.6/20.9/22.6 | 14.8 |
| length-4 split when boxed | 22.2 +- 2.9 | 17.8 | 16.6/21.3/22.1 | 14.1 |
| both | 22.0 +- 1.2 | 17.5 | 16.3/20.9/22.9 | 15.8 |
| both + portal fixes (ts13) | 20.5 +- 1.1 | 16.0 | 16.6/20.8/22.5 | 16.1 |
| wander (no centre drift) | 17.8 +- 1.1 | 9.8 | 14.2/20.6/22.2 | 9.8 |
| split odds (single run) | 22 | 18 | 14.2/15.7/17.6 | |

**Other results:**
- gst96 vs clone, 15 official maps, seeds 1-3: vs ts13 58-2-30, vs ts11 53-3-34. The
  server series was 14-31.
- ts13 vs ts11: 32-4-24 official (seeds 1-2), 17-3-20 on 20 worldgen maps (seed 27000).
- `check`: all checks passed.

**Verdict:** fidelity is unchanged within noise. The r200-300 count gap (~21-23 vs 31-39)
is untouched by any single behaviour switch. The next lever is pearl *coverage*: the real
team eats spawned pearls sooner, which grows the supply. That points to how its dragons
pick targets across the map (sonar-shared bed timers, or keeping near ripening beds). It is
not a matter of local step safety.
