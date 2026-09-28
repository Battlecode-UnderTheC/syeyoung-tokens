# late-surge-16

## Idea

late-surge-15 with one change: the trader switch cannot make a dragon trade evenly before round 60
(`trader_from = 60`). The flag is still detected and spread by sonar as before; it only takes
effect from r60. Namespace renamed ls15 -> ls16.

## Diagnosis (official maps, vs late-surge-14 and gather-split-trade-39)

- Feature firing, counted by tagging action reasons in a temporary build (official, 22 matches):
  **the trader switch is on in about 4,600 of about 8,000 dragon-turns** against both opponents.
  One trade (or 5 approaches) sets it early in nearly every game. Outnumbered mode fires on only
  about 24 turns and the centre pull on about 63. Those two can't explain the gap.
- Ablations on official (22 matches each, parent = 12-0-10 vs ls14 and 12-0-10 vs gst39):
  trader switch off gives 13-9 / 10-12; om off 11-11 / 11-11; centre off 13-9 / 11-11. All
  within noise. Only approaches as a trigger: 13-9 / 10-12. trader_trades 3: 12-10 / 13-9.
- Team total at r50/r100/r200/r340, summed over the official maps (0 for finished games), ls15 as A vs ls14:
  base 269-295 / 532-584 / 604-785 / 698-819; trader off 333-298 / 526-596 / 750-814 / 783-781.
  So the trader switch costs growth against ls14 (which also has it, so both escalate). Against
  gst39, though, it helps: r340 as A 985-611 base vs 971-761 off.
- Trader switch fully off on worldgen: 40-40 vs ls15, 33-47 vs ls14, 39-41 vs gst39. Rejected.
- Most of the official r340 deficit vs ls14 is on a few seat-A maps (devil 23-114, trophy and
  default_small wipes), where the results swing from game to game.

## Measured (sim, both seats; official = 22, worldgen wg1 = 20 random maps x2, wg2 = seed 2424 x2)

| opponent | official | wg1 | wg2 | total | parent ls15 on same wg sets |
|---|---|---|---|---|---|
| late-surge-15 | 11-0-11 | 20-0-20 | 22-0-18 | 53-49 | - |
| late-surge-14 | 12-0-10 | 19-0-21 | 17-0-23 | 48-54 | 12-10, 19-21, 16-24 |
| gather-split-trade-39 | 11-0-11 | 15-0-25 | 21-0-19 | 47-55 | 12-10, 16-24, 20-20 |

Official totals, ls16 as A vs ls14: r50 333-298 (base 269-295). By r100 the games converge back to
the parent's play: from r100 on the traces are the same as the base's.
check: all checks passed, slowest turn 0.1 ms.

## What should improve

A quieter opening (r50 +64 team length as A vs ls14 on official). Otherwise it is about even with
the parent. This is a small, low-risk change, not a fix for the official-map gap.

## Tried, didn't work

Trader switch off (worse on worldgen vs ls14/gst39); approaches trigger off (no change);
trader_trades 3 (noise). Outnumbered mode and centre pull barely fire on official maps.

## Next

The trader flag is sticky for the whole game and spreads team-wide by sonar. Make it decay, or make
it per-region, so a single early trade doesn't turn every dragon into an even trader. Look at
devil and trophy as seat A vs ls14 with `--trace`.
