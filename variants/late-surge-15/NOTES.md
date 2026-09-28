# late-surge-15

## Idea

Crossover late-surge-14 x gather-split-trade-39, done as a wholesale movement-layer swap, the way
giant-8, counter-swarm-19 and swarm-hunt-14 did it. The source base is now gst39's `strategy.*`
(renamed `ls15`, macros `LS15_*`). late-surge's identity is ported on top of it.

## What changed from the parents, and why

- **From gst39, whole:** the movement and goal layer. That is SafeStep with escape levels and
  TurnOut, dead ends worth entering, contest/share goal scoring, patrol, split safety with cap
  safety and the reserve, sonar-checked crossing of unknown portals (`cross_until` 480), the loop
  breaker (ban 50, not while waiting on a bed), heard_split, and champion reports with the
  lower-bound fix plus obituaries. ls14's own probe/portal-ban/loop code is dropped.
- **Kept from late-surge-14:**
  - Quiet trading: `trade_margin = 1` (gst39 has 0), so strikes and lunges go only at strictly longer enemies.
  - The trader switch: WatchTrades, 5 approaches or 1 trade, trader hunt 0, trader margin 0.
  - The early-outnumbered mode (om_*): no even trades, the trader switch off, and a 2-step
    dodge through ExposedNear in SafeStep.
  - The small-map centre pull in ChooseGoal.
  - The late converge walk (from 440, length 10 or more).
- **Endgame:** both parents already had the same one (split until 400, leaders until 300, freeze
  330, everyone feeds from 380 within 3 if below 80% of the champion).
- **Sonar bits:** bit 0 is the lower-bound flag (as in gst39), bit 1 is the trader flag, and bits
  2-9 must be zero. Obituary words and portal pings go through Encode, so they carry the trader
  flag too. A bare portal ping (word 1, no champion) fails the magic check and is ignored.

## Measured (sim, both seats; official = 22 matches, worldgen = 40 each)

| opponent | official | worldgen set 1 (random) | worldgen set 2 (seed 2121) | total |
|---|---|---|---|---|
| late-surge-14 | 12-0-10 (+6.8 longest, +4.3 total) | 21-0-19 (+2.8, +17.9) | 24-0-16 (+2.1, +23.6) | 57-45 |
| gather-split-trade-39 | 12-0-10 (-3.4, +18.4) | 25-0-15 (+1.8, +30.2) | 23-0-17 (+6.2, +5.3) | 60-42 |

Mean team total length at r50 / r100 / r340, ls15 first (traced, ls15 as A, over the games still
running at that round):

| maps, opponent | r50 | r100 | r340 |
|---|---|---|---|
| official, ls14 | 27-30 | 53-58 | 100-117 (n7) |
| wg1, ls14 | 82-74 | 134-104 | 160-116 |
| wg2, ls14 | 83-75 | 128-104 | 178-141 |
| official, gst39 | 25-26 | 59-48 | 123-76 (n8) |
| wg1, gst39 | 82-70 | 136-97 | 161-111 |
| wg2, gst39 | 83-70 | 128-99 | 166-147 |

On worldgen the swap closes the growth gap with ls14: +8 by r50 and +24-30 by r100. On official
the samples are small, and it is slightly behind ls14 there.
check: all checks passed. The slowest turn was 31 ms on schooltime, under heavy machine load;
gst39 showed 16-23 ms under similar load.

## What should improve

More growth by r100 on open and worldgen maps, from gst39's movement. The trade and endgame
behaviour stays late-surge's.

## Next

The official r340 total against ls14 is behind on 7 traced games. Find out which maps, and whether
the cause is the trader switch or outnumbered mode interacting with gst39's contest scoring.
