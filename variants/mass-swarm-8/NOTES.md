# mass-swarm-8 (arena round 7, team swarm)

## Approach
The gst line (gather-split-trade-131 = v8, through mass-swarm-6) plus one new behaviour
borrowed from arena-search-6/7 (credit: the search team): **head-to-head trades judged by value,
not by a length rule.**

A head-to-head kills both dragons and leaves remains: a pearl on every second segment,
ceil(L/2) from each corpse, eaten by whoever has heads nearby. So a trade is worth

    their length - our length + remains * (2 * share - 1)
    share = (our heads + 0.5) / (all heads + 1), counting heads within 5 tiles of the collision,
            the two traders excluded

and we take it when that is > 0 (`kTradeValue`, `kShareRadius`, `kTradeMargin`).

- **What it replaces:** gst's WorthTrading ("the enemy is longer, or we outnumber them").
- **Where it applies:** the same function drives kamikaze, the multi-step lunge, hunting, and
  `Exposed()`. A tile beside an enemy head we would gladly trade with is no longer avoided.
- **What it leaves alone:** corridor and dead-end farming, trap pricing, and the rest of SafeStep
  are gst's, untouched.
- **What didn't need porting:** search's other two economy changes already hold in gst. gst never
  feared multi-step lunge reach (Exposed checks only adjacent heads) and never waited for calm
  to split (split_crowd_limit is off in this line).

Everything else is mass-swarm-6: continuous splitting, the team-size consolidation schedule,
chop feed, reserved ground, die-at-body and seek-body delivery.

## Measurements (faithful sim, both seats)
Tuning, seeds 1-10, 15 official maps (300 games per row), vs v8:

| build | W-D-L | margin (longest / total) | r100 dragons, us v them |
|---|---|---|---|
| mass-swarm-6 (no trade value) | 154-146 | | |
| trade value, margin 0 (kept) | 195-105 | +2.8 / +20.9 | 22.0 v 19.6 |
| margin 2 (more cautious) | 134-166 | | |
| margin -1 (bolder) | 165-135 | | |
| share radius 3 | 169-131 | | |
| share radius 8 | 182-118 | | |
| search's timing (feed from 380, freeze 360) | 186-113 | | |

The lesson is the same as the search team's: the value rule helps, and moving the margin either
way hurts.

**Held out, seeds 21-30, 15 official maps (300 games per opponent):**

| opponent | W-D-L | win % | final margin (longest / total) |
|---|---|---|---|
| gather-split-trade-131 (v8) | 178-1-121 | 59% | +1.8 / +8.4 |
| gather-split-trade-96 (v7) | 175-0-125 | 58% | +1.3 / +3.5 |
| trading-swarm-13 | 224-11-65 | 75% | +9.7 / +61.3 |
| mass-swarm-6 | 184-0-116 | 61% | +2.1 / +22.6 |
| arena-free-6 | 158-9-133 | 53% | -0.4 / +49.4 |
| arena-giant-6 | 163-1-136 | 54% | +0.1 / +15.1 |
| arena-hunter-6 | 174-0-126 | 58% | +1.9 / -7.0 |
| arena-territory-7 | 142-0-158 | 47% | -0.3 / -19.4 |
| arena-search-6 | 124-3-173 | 41% | -2.4 / -7.9 |
| arena-search-7 (live v9) | 124-1-175 | 41% | -2.9 / -12.4 |

Economy vs v8 on held-out seeds: 12.6 v 12.0 dragons at r50, 21.7 v 20.5 at r100 (total 55.1 v
51.4). Against arena-search-7 we trail at r50 (12.1 v 13.7) and on total at r100 (52.7 v 61.2).

**Server maps (maps/server, the real server placement), seeds 1-10, both seats, vs v8:**
72-0-28 (100 games).

| map | W-D-L | r100 dragons (us v v8) | r100 total (us v v8) |
|---|---|---|---|
| schooltime | 13-7 | 29.2 v 24.7 | 73.5 v 62.7 |
| autarky | 17-3 | 16.6 v 12.6 | 39.8 v 30.4 |
| dilemma | 20-0 | 11.9 v 1.0 | |
| queen_of_spades | 10-10 | 8.2 v 7.1 | |
| slithery_fight | 12-8 | 49.3 v 49.0 | 127.0 v 125.8 |
| all five | | 24.2 v 20.6 (r50: 15.4 v 13.8) | 61.1 v 52.3 |

**The real M665147 seeds on maps/server** (seats A / B):

| map | vs v8 | vs arena-search-7 |
|---|---|---|
| slithery | W 49-48 / L 35-45 | W / L |
| schooltime | L / L (eliminated at r386 and r450) | L / L |
| autarky | W / W | L / L |
| dilemma | W / W | L / W |
| queen_of_spades | L / W | W / W |

- No starvation in these games. On Queen of Spades (where v9 ate 4 pearls in 299 rounds on the
  server) we finish at T 56-62 or wipe the enemy.
- Schooltime at the server seed is the bad case: we are eliminated by v8 in both seats, while on
  seeds 1-10 we win it 13-7.

**Hot-bed pearls** (strategist w's hot.py; unswbc sandbox, server seeds, vs gst131):

| map | seat | ms8 | gst131 | result |
|---|---|---|---|---|
| slithery | A | 335 | 433 | won on length |
| slithery | B | 479 | 296 | won on length |
| autarky | A | 8 | 4 | won |
| autarky | B | 4 | 4 | won by elimination at r263 |

Corridor farming is not lost. It stays at gst's level, which on Autarky is almost nothing for both
sides (8 hot beds, all 2-exit corridor tiles).

**CPU** (judge sandbox): p50 4.4M, p99 5.5M, max 6.0M points per turn (limit 100M). `check`: all
checks passed.

## What I learned
- Search's economy transfers to the gst line through one function: trade value. gst already
  lacked the other two cautions. It is the first change in this line that clearly beats v8, and it
  also beats v7, mass-swarm-6, hunter-6 and giant-6.
- It is nowhere near 90%.
  - The best result is 75% vs ts13.
  - Territory-7 and search-6/7 still beat it. Against search-7 we trail on count at r50 and on
    total at r100: search's movement (its own search, not a rule) still eats more per turn.
- A note on this line's parameters: `kBigTeam` has been 48, not the 24 I reported, since
  mass-swarm-3. An experiment script left its last edit in the header. 48 measured the same as 24
  (81-69 in round 3), but the earlier notes misstate it.

## Next
- Put gst's corridor farming and the trade-value rule together with search-7's movement, as the
  search team is doing (gst-search-1), and test the tip split from strategist w (P1) on top.
- Schooltime at the server seed: find why we are eliminated there (a trace from r0) before trusting
  the server-map wins.
