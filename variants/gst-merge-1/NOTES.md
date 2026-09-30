# gst-merge-1 (arena round 7, team swarm: the merge)

## What it is
mass-swarm-8 (gst131 line + value-judged trades) plus, one piece at a time:

1. **gst-hunter-1's hot-bed farming (kept).** Code credit: the hunter team. Strategist w's P1/P2.
   - A bed seen to reset its countdown to <= 3 is hot, and is worth a full pearl whenever due.
   - A dead-end pocket gains +1 per hot bed.
   - A 2-3 long dragon may enter a pocket that grows it to 4, then tip-splits out with gst's
     trapped split.
   - A farmer of length <= 5 heading for a hot bed within 8 claims it over sonar (0xA8). Allies
     skip hot beds within 2 of a claim for 30 rounds.
   - Ported verbatim. One fix: the claim tile is cleared at the start of every turn, so an early
     return (feed, split, trade) no longer rebroadcasts last turn's claim.
2. **gst-free-1's P3a opening hold (in the code, OFF: kEarlyHoldRound = 0).** Before r20 a
   dragon of length >= 8 doesn't split with an enemy head within 6. Code credit: the free team.
   It lost vs piece 1 alone, so it is off.
   - gst-free-1's map-type switch was NOT taken: it keys on a size table, which the RULE forbids.

**No map-keyed logic.** Nothing branches on the map's name, exact dimensions or a per-map table.
- Hot beds are detected from observed countdowns.
- The one size-dependent rule is inherited unchanged from v8: gst's opening centre bonus
  applies when width x height <= 1024. It is a generic area threshold, not a table.

Endgame left alone (search is folding in search-7's). mass-swarm-6's consolidation stays as it
was.

## Piece by piece (faithful sim, both seats)
| step | server maps (5), seeds 1-10 | official, seeds 1-10 |
|---|---|---|
| hot-bed farming vs mass-swarm-8 | 61-0-39 | 160-0-140 |
| + opening hold vs hot-bed farming alone | 44-0-56; on all 10 server maps 94-0-106 | 154-0-146 |

For the hot-bed farming step vs mass-swarm-8 on server maps: r50 16.4 v 14.6 dragons, r100 24.7 v
20.9, total at r100 63.3 v 53.5.

## Final build vs the targets
**Official maps, held-out seeds 21-30** (300 games per opponent):

| opponent | W-D-L | win % | final margin (longest / total) | r50 dragons | r100 total |
|---|---|---|---|---|---|
| gather-split-trade-131 (v8) | 185-0-115 | 62% | +2.8 / +16.9 | 13.3 v 12.1 | 59.1 v 52.5 |
| gst-hunter-1 | 182-1-117 | 61% | | | 59.1 v 54.9 |
| gst-search-1 | 150-0-150 | 50% | | | 55.7 v 52.4 |
| mass-swarm-8 | 159-1-140 | 53% | | | 55.8 v 52.5 |
| arena-search-7 (v9) | 144-0-156 | 48% | -2.0 / -7.5 | 13.3 v 13.3 | 55.2 v 58.4 |

**All 10 server maps, seeds 1-10** (the first five maps plus default, devil, portals, trauma and
trophy; 200 games per opponent):

| opponent | W-D-L (first 5 + new 5 = total) | win % |
|---|---|---|
| v8 | 67-33 + 64-36 = 131-69 | 66% |
| gst-hunter-1 | 64-36 + 60-40 = 124-76 | 62% |
| mass-swarm-8 | 61-39 + 54-46 = 115-85 | 58% |
| gst-search-1 | 54-46 + 45-55 = 99-101 | 50% |
| arena-search-7 | 34-66 + 42-1-57 = 76-1-123 | 38% |

Economy on the first five server maps vs v8: r50 17.2 v 13.7 dragons, r100 26.3 v 19.7 (total
67.9 v 50.4). Against arena-search-7 we lead at r50 (17.5 v 13.5) and r100 (25.9 v 20.6, total
66.9 v 50.0) and still lose 34-66, ending -6.9 longest and -40 total. Its endgame beats ours.

**The real M665147 seeds on maps/server** (seats A / B):

| map | vs v8 | vs arena-search-7 |
|---|---|---|
| slithery | W / L | L / L |
| schooltime | L / L | L / L |
| autarky | W / W | L / L |
| dilemma | W / W | W / W |
| queen_of_spades | L / W | W / W |

**Hot-bed pearls** (judge sandbox via unswbc, server seeds, vs gst131, strategist w's hot.py):

| map | seat | gst-merge-1 | gst131 | result | mass-swarm-8 (same games) |
|---|---|---|---|---|---|
| autarky | A | 220 | 6 | won | 8 v 4 |
| autarky | B | 192 | 4 | won | 4 v 4 |
| slithery | A | 1184 | 524 | lost on length | 335 v 433 |
| slithery | B | 789 | 527 | lost on length | 479 v 296 |

- The hot-bed farming works as intended.
- The sandbox's Slithery seat-A result (a loss) disagrees with sim's (a win, 40-31). Still to
  check, e.g. whether the unswbc run seeds the bots' RNG.

**CPU** (judge sandbox): p50 4.4M, p99 5.5M, max 6.1M points per turn. `check`: all checks
passed.

## Schooltime at the server seed (trace, gst-merge-1 vs v8, seat A; mass-swarm-8 is the same)
- **Even to r80:** 18 v 17 dragons, total 48 v 48.
- **From r100 we stall and v8 grows:**

  | round | dragons, us v v8 | total, us v v8 |
  |---|---|---|
  | 100 | 22 v 25 | |
  | 140 | 26 v 47 | 59 v 125 |
  | 220 | 25 v 64 (the cap) | |
  | 320 | 25 v 64 | 67 v 162 |

  From r320 v8 eliminates us, by r386.
- **Whole game:**
  - v8 splits 438 times to our 188, and enters dead ends "worth it" 561 times to our 208.
  - v8 lunges 36 of our short dragons (134 length) against our 20 (54 length). Our value rule
    lets our dragons stand beside heads it rates as fair trades; v8 then trades its 2-3-long
    dragons into our longer ones.
- **In r80-320 deaths:** trades are even (about 26 lunges each way). v8 loses 211 dragons
  "giving up a slot" at the cap: it is so far ahead it can throw them away.
- **Conclusion:** the loss is an economy stall from r100 on this map and seed: fewer splits and
  fewer dead-end entries. Hot-bed farming doesn't reach it: the result is identical with and
  without farming.
- **Next here:** find why our dragons stop splitting from r100. Likely candidates:
  - the lunge losses of 4-5-long dragons, which are the ones that would split;
  - Exposed() under the value rule (making it cautious again for short dragons beside longer
    enemy heads is worth a test).

## Wrapping it with gst-giant-1's rollout judge (gst-merge-2)
Easy. gst-giant-1's plugin.cpp wraps any gst StrategyAgent: it calls gst's Turn(), and when an
enemy head is in view and the action is a non-feeding move, it lets rollout.hpp's Judge review
it. Wrapping this merge needs only:
- rollout.hpp copied in (namespace renamed);
- a plugin.cpp that holds gm1::StrategyAgent instead of gstg1::StrategyAgent.

Its feeding exclusion matches on "feed" / "champion" in the reason, which covers our "feed
champion" and "feed: chop". Our hot-bed reasons need no exclusion. One caution: the judge might
override a farmer's dead-end step when an enemy head is in view.
