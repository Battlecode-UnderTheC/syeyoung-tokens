# late-surge-5

Mutation of late-surge-1 (task g7-4): diagnose why it loses on the official maps (big_empty
especially), fix the biggest cause. **Result: no fix beat the parent; not for submission.** The
committed defaults are late-surge-1 plus one bug fix (`trust_visible`), which measured even.

## Diagnosis (late-surge-1 vs gather-split-trade-27, counter-swarm-12, swarm-hunt-8, 11 official maps, both sides)

Baseline: 8-14 vs gst27, 11-11 vs cs12, 13-9 vs sh8. The losses are of two kinds:
small maps (arena, Colosseum, devil, trophy as one side) wiped out by round 30–320, which depend
on the side; and round-499 longest-dragon losses (stronghold, default, queen_of_spades,
schooltime, big_empty).

**big_empty (64×64, open): the surge never happens.** Traced vs counter-swarm-12 (lost 86-104
longest with **885-104 total**; 54 dragons alive at 499). Per-dragon dumps at rounds
100/250/380/420/460/499: every dragon knows the champion (id 261; reports 1–3 rounds old, length
30 → 86), `feeding` is on for all of them from 380, but their torus distance to it stays 20–55
all the way to 499. Cause: the champion's reported tile is somewhere the feeder never explored,
so `Search` over the feeder's known board can't reach it (`dist = -1`), and the code falls back to
`ChooseGoal`. The feeder just keeps gathering. Only ~5 feed deaths a match on big_empty.
Champion 30 at 380: no dragon is big at surge time (split at 4 until 400, 64-unit cap).

**Fix tried: `seek_champion`.** With no known path, walk to the known tile minimising 2·torus(t,
champion) + dist(t), which heads into the unknown towards the champion. On the traced game the
feeders did arrive (64 feed deaths vs ~5), champion 86 → 106, loss turned to win. But:

1. **The champion gets lunged.** Once feeders gather round it, enemy short dragons lunge it
   (they trade at strictly longer enemies): traced deaths of champions at L≈30 (round 403, vs
   late-surge-1) and L≈58 (round ~470, vs gst27). KNOWLEDGE's "the champion is almost never
   killed" does not hold on big_empty vs lunging opponents.
2. **Ghost stub, local form (a real bug in late-surge-1 and its line).** Champion 500 (L55)
   made a trapped split: heir 502 (L53) and the stub 500 (L2). The heir and nearby allies
   hold the fresh report "500 is L55". `Remember` keeps `max(reported, visible)` for allies in
   view, so they keep ranking the 2-long stub as champion (55 > 53). The **heir resigns next to
   the stub to "feed" it**, and the team's mass is gone. The `report_fade` fix for ghosts
   (gst23) doesn't cover this because the stub is alive and seen fresh every turn.
3. Walking feeders stop gathering and die en route or at a dead champion: on worldgen, seek
   costs ~50 total length per match and gains no longest.

**Fixes for 1 and 2.** `lunge_guard_min`: from `champ_freeze`, the champion treats tiles within
max(visible length − 1, 3) of an enemy head in view as exposed. `trust_visible`: an ally
whose last visible segment is off the window border has its whole body in view, so its visible
length replaces the report.

## Measurements (sim, both sides; worldgen = 20 fresh maps, 40 matches)

| config | vs late-surge-1 worldgen | vs ls1 official | vs gst27 worldgen | vs gst27 official |
|---|---|---|---|---|
| late-surge-1 (all off) | 20-0-20 (self) | 11-0-11 | 27-0-13 | 8-0-14 |
| seek only | 20-0-20 (−51 total) | 9-0-13 | | |
| trust_visible only (**committed**) | 20-0-20 (+1.9 longest) | 11-0-11 | 24-0-16 | 9-0-13 |
| lunge guard only | 17-0-23 | 10-0-12 | | |
| seek + trust | 16-0-24 | 13-0-9 | 23-0-17 | 10-0-12 |
| seek + trust + guard | 14-0-26 | 9-0-13 | | 9-0-13 |

Nothing clearly beats the parent. trust_visible is even with it (a correctness fix that rarely
fires). The lunge guard hurts: a champion that shies from every enemy head in view stops
collecting.

## What should improve / what I'd try next

- big_empty is lost to an economy problem, not reach alone. Feeding pays ~half a feeder's
  length (pearls on every second segment) and only if the champion eats them before enemies
  do. With 800+ total and a 30-long champion at 380 on a 64×64 map, a surge that must cross
  the map arrives too late. Try a **map-size-dependent surge**: on big maps, several regional
  champions (feed the nearest known ally longer than you) and then one final merge. Or start
  the surge earlier where W+H is large.
- Champion safety should target the actual threat: enemy heads whose lunge path is free and
  who are shorter (they trade on strictly longer), not all heads in view.
- `seek_champion` stays in the code (off). It's the right mechanism for big maps, but it
  needs the above to pay.
