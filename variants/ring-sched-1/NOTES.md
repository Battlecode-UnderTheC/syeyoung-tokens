# ring-sched-1

Task g38-sched. Parent: ring-bud-cpu (the same code as ring-plus-16, the live server bot).
The first step towards scheduling the team's work: which worker takes which food, and which
workers explore.

## 1. Measurement

Two harnesses, kept outside the repo:
- `alloc.py` reads the server replays. It covers the 211 rp9 games in server-r12, both sides.
- `msim` is a native harness that plays local games and logs every action's `reason()` per
  turn.

Every metric is taken engine-side, so ours and the opponent's are measured the same way. It
counts non-queen dragons only, rounds before 300.

- **Pearl in view.** On each worker turn with a pearl in the 7x7 window, take the nearest
  such pearl. The worker *owns* it if no allied worker head (at round start) is nearer, ties
  going to the lower id. Turns are then split into own&go, own&no, dup&go and dup&no, where
  dup means another worker is nearer and go means the worker moved closer or ate.
- **Bed pearls.** At spawn, take each side's nearest head by BFS through kelp, up to 15.
  Count how many of "our" pearls the enemy ate, the latency from spawn to eaten, and the
  excess latency (latency minus the owner's distance).
- **Other measures:** turns in no-eat streaks of 30+, and coverage at r100.

### Server (rp9, 211 games): ours v theirs

| | ours | theirs |
|---|---|---|
| own&go / own&no / dup&go / dup&no (share of pearl-in-view turns) | 0.27 / 0.27 / 0.21 / 0.25 | 0.26 / 0.30 / 0.19 / 0.24 |
| bed pearls we were nearer to but the enemy ate | 2.5% | 2.5% |
| spawn-to-eaten latency (excess over distance) | 6.6 (4.3) rounds | 6.1 (3.9) rounds |
| turns in 30+ no-eat streaks | 19% | 25% |
| coverage at r100 | 0.73 | 0.72 |

- On these measures, allocation is level with the opponents.
- Pearls never expire, so "food lost" can only mean food the enemy ate. That is small: 2.5%
  either way.
- The one gap: our food waits about 0.4-0.5 rounds longer.

### Why the nearest worker skips the nearest food

An instrumented local copy (not committed) logged why. Setup: ring-bud-cpu v csq7, server
maps, seed 101, 201k goal choices.
- In 21% of goal choices a pearl in view could not be reached (kelp, or edges never seen).
- The worker chose its nearest reachable pearl only 43% of the time.
- When it chose something else, the winner was:
  - exploring in 56% of cases;
  - a bed in 20%;
  - another pearl in 17%;
  - nothing in 7%, usually a pearl in a dead end.
- The crowd discount (0.55^allies within 3) was applied to 77% of the skipped pearls, and
  the "an ally is nearer" share discount to 64%.
  - The crowd discount also hits the nearest dragon. So two workers next to one pearl can
    both back off it.
  - The share test compares the ally's Manhattan distance with our path distance. With
    kelp in the way both can yield; with no kelp and equal distances, neither does.
- **Role flips.** A worker's goal switches between food and exploring on 22% of consecutive
  goal choices.

## 2. Design

Treat it as an assignment problem, dragons × food, using only what each dragon sees.

**Matching.** Each worker gathers:
- its own BFS distances;
- a bounded BFS (12 steps over the free tiles it knows) from every allied worker head in
  its view, up to 12 participants.

A bed counts from when it ripens, so its arrival time is max(distance, rounds until due).
All (arrival, id, tile) pairs within 12 rounds are matched greedily in that order, one tile
per dragon.

- Every dragon that sees the same allies computes the same matching. It needs no messages,
  so it costs no sonar bandwidth; the sonar still carries the champion and ring words.
- Views differ at the 7x7 edge. Greedy matching by arrival time is local, though, so the
  nearest pairs agree.

**Use of the matching.**
- **Another worker's tile:** score ×0.1, the old share factor. The opening's nearest-pearl
  fallback skips it too.
- **Our own tile:** no crowd or share discount.
- **Tiles no one is matched to:** scored as before.

**Roles.** A worker matched to food it reaches within 8 rounds is a *harvester* and drops
the hunger and unseen-share explore multiplier (up to ×3). The unmatched workers keep it and
are the explorers. A worker alone gets its earliest food as its match.

This is a role quota that comes out of the board state: the explorers are exactly the
dragons the visible food can't use.

**Not considered:** sonar claims, auctions and Voronoi partitions. Sonar claims and auctions
need bandwidth the champion and ring words already use. Voronoi by arrival time is what the
greedy matching already does locally.

**Cost:** about 12 BFS of radius 12 plus a sort per worker turn. Sandbox, worst turn:
- big_empty: 28.8M v the parent's 29.8M;
- unsw: 21.9M v 23.7M;
- australia: 21.6M v 22.0M.

The queen's code, the safety rules and the endgame are unchanged. The matching runs only
before the feed round, and never for the champion.

## 3. The change

- `strategy.hpp`: `SCHED_MODE 2`, kSchedHorizon 12, kSchedMaxParts 12, kSchedHarvest 8,
  kSchedOther 0.1, and two scratch members.
- `strategy.cpp`: `ChooseGoal`.
  - The matching block comes after the reserved-ground block.
  - The fallback skips food matched to another worker.
  - Matched tiles replace the crowd and share discounts.
  - A harvester's `hunger_mult` is set to 1.
- `SCHED_MODE 0` gives back the parent; 1 is the matching without roles.

## 4. Numbers

**Local sim** (the judge's engine and pearl seeds, both seats, server (17) + official (15)
maps; "2-st" = 2 starters per side):

| test | seeds | W-L | 2-st | 3+-st |
|---|---|---|---|---|
| ring-sched-1 v ring-bud-cpu | 105-108 | **140-116** | 58-54 | 82-62 |
| ring-sched-1 v csq7 | 101-104 | 137-119 | 58-54 | 79-65 |
| ring-sched-1 v csq7 | 105-108 | 140-116 | 61-51 | 79-65 |
| ring-bud-cpu v csq7 (base) | 101-104 | 101-155 | 41-71 | 60-84 |
| ring-bud-cpu v csq7 (base) | 105-108 | 128-128 | 52-60 | 76-68 |
| matching only (mode 1) v csq7 | 101-104 | 111-145 | 48-64 | 63-81 |
| hunger part only v csq7 | 101-104 | 118-138 | 57-55 | 61-83 |

So against csq7 over 512 games it is **277-235 v the base's 229-283**. Both parts help, and
they help more together than the sum of each alone.

Dragons and total length at r50/r100/r200, v csq7, seeds 105-108:

| | dragons r50 / r100 / r200 | total r50 / r100 / r200 |
|---|---|---|
| ring-sched-1 | 12.4 / 20.7 / 27.8 | 30.5 / 51.7 / 73.0 |
| base | 12.6 / 20.9 / 27.4 | 30.9 / 52.8 / 73.7 |

Against ring-bud-cpu:
- dragons: 12.6 / 21.9 / 27.7 v 12.6 / 20.9 / 26.6;
- total: 31.0 / 54.0 v 30.8 / 52.8.

Allocation after the change (server maps, seeds 101-104 v csq7, base first):

| measure | base | ring-sched-1 |
|---|---|---|
| own&no | 0.27 | 0.27 |
| dup&go | 0.20 | 0.21 |
| latency | 6.9 | 6.6 |
| excess | 4.6 | 4.35 |
| coverage at r100 | 0.73 | 0.69 |

Goal choices (seed 101, server maps, base first):

| measure | base | ring-sched-1 |
|---|---|---|
| food as the goal | 42% | 48% |
| food↔explore flips | 22% | 23% |
| goal changes between foods | 11.5% | 15.5% |

The matching is recomputed every turn as dragons move, so commitment didn't improve.

**Arena light duels** (official judge, fresh seeds, 2 per map):

| opponent | suite | W-L |
|---|---|---|
| ring-plus-16 | server | 31-37 |
| ring-plus-16 | official | 8-12 |
| csq7 | server | 35-33 |

All are within noise. Against rp16 our dragons at r100 were 21.9 v 23.8 and tiles visited
at r200 0.44 v 0.51: in the arena the explore cut cost some coverage. The draft's own arena
games are the real test.

`check`: all checks passed.

## 5. Next steps toward fuller scheduling

1. **Commitment.** Keep last turn's match unless a new pair is better by a margin (a
   hysteresis term). The matching now re-solves every turn, and goal changes between foods
   rose from 11.5% to 15.5%.
2. **Allies out of view.** Workers seen in the last 1-3 rounds, and sonar self-reports,
   should join the matching with their position aged. Today a worker 4 tiles away is
   invisible, so far food is double-booked.
3. **Explore as tasks.** Frontier cells should be matched to the unmatched dragons the same
   way, one frontier region per dragon, instead of each picking the best frontier tile.
   This is the fan-out problem, and coverage fell from 0.73 to 0.69.
4. **Starting size.** The queen knows the team's starting size at r0 (FORAGE35 uses it). If
   it were relayed in a sonar word, workers on 2-starter maps could keep more explorers,
   where hunger exploring helped. The arena's 2-starter score is the weak spot (19-25).
5. **Dead-end food.** Pearls in pockets are rejected as traps in about 7% of skips. Matching
   a pocket to the one dragon short enough to farm it would recover them.
