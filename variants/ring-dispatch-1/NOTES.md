# ring-dispatch-1 (g43-2): the team task board

Base: ring-contest-m1 (arena ring-contest-1, live copy ring-contest-4). Line ring-dispatch.
One change: the second scheduling step after ring-sched-1's arrival-time matching
(strategist A r15 #1). Code: `dispatch.cpp` plus hooks in `strategy.cpp` (ChooseGoal, Decide, Act).

## Evidence it answers
- arena-r17: early collapse A didn't shrink (13.83 → 14.58 per 100 games). Middle eats r0-59 were 11.5 v 16.0,
  and workers bunched at home in 167601, 167062 and 167425.
- Server losses, r0-60 eats: 23.7 v 36.7 (middle 1.7 v 7.1, far 3.5 v 8.3).
- devtest1: spread at r50 was 7.0 v 10.5.

## Change
1. **TASK sonar word, 0xBB.** Fields: team | id (11 bits) | role (HARVEST, EXPLORE, CONTEST, HOLD) | target x,y |
   head x,y | round mod 16 | home kind and x,y.
   - Workers time-share sonar slots: the ring word goes on two opposite directions (alternating each round) and the
     TASK word on the other two.
   - The queen, and workers within 4 of her or her ring, keep the ring word on all four.
   - Only before r150. Ring-word bits are untouched (56-63 not used).
2. **Matching.** Allies out of view whose food TASK we heard in the last 3 rounds join ring-sched-1's greedy matching.
   Each one adds a single pair: its announced food, arriving at its reported distance less the rounds since.
3. **Sectors.**
   - Unseen ground is split into 8 directions from home. Home is the queen's ring; failing that, a ring home heard in a
     TASK word; failing that, our queen as last seen.
   - Unmatched workers (the explorers) are matched greedily to sectors by distance, at most one per sector.
   - A sector named in an ally's fresh EXPLORE word stays that ally's.
   - Frontier in a sector an ally holds scores x0.5. Our own sector gets no boost (see iterations).
4. **Hysteresis (about 30%).**
   - Last turn's food goal scores x1.3 in our own goal choice.
   - Last turn's sector competes at distance x10/13.
   - A worker that harvested last turn stays a harvester up to 10 rounds out (instead of 8).

`TASK_MODE 0` gives back the parent (checked: identical games in sim). `TASK_ABL` (default 0) holds the ablation
switches. `DISPATCH_DBG` prints a per-turn `TDBG` line before r60.

## Iterations (all local runs are ring-dispatch-1 vs ring-contest-m1)

| ver | what | arena server random:2 | arena official | fresh mapgen (44 maps, seeds 1,2) |
|---|---|---|---|---|
| v1 | as above; hysteresis inside the matching (own and heard prior target at arrival x10/13); own sector x2 | 36-32 | 10-10 | 93-83 (open 4-12) |
| v2 | v1 + rush explorers honour claimed sectors | 26-42 | 10-10 | 89-87 (open 2-14) |
| v3 | hysteresis moved out of the matching into our own goal score; v2's rush change dropped | 33-35 | 9-11 | 92-84 (anti-centre 4-12; 13-27 over seeds 1-5) |
| **v4 = submitted** | v3, but our own sector is not boosted (x1) | **41-27** | 9-11 | 81-95 (no family below 5-11; anti-centre 19-21 and open 17-23 over seeds 1-5) |

- **Why v1 lost open maps.** Ablation on open fresh maps, 16 games each:
  - no hysteresis: 8-8, with r50 dragons 23.3 v 19.4;
  - no matching priors only: 7-9;
  - no role hysteresis only: 5-11.

  The prior discount inside the matching broke its determinism. Only allies that heard a dragon's TASK word know its
  prior, so different dragons computed different matchings (r50 dragons 16 v 25).
- **Why the own-sector boost went.** Anti-centre fresh maps, seeds 1-5 (40 games):
  - v3: 13-27;
  - v3 without sectors: 20-20;
  - v4: 19-21.

  Explorers already carry the hunger and unseen multipliers, so a x2 sector boost over-explored.

## Behaviour (decoded)
**TDBG counters**, sim on maps/server, seed 1, worker turns before r60, base (`TASK_MODE 0`) first:
- task kept from last turn: 0.563 → v1 0.615, v3 0.608, v4 0.563;
- food/explore flips: 0.157 → v1 0.135, v3 0.135, v4 0.151;
- distinct head sectors by r30: 5.91 → v1 6.06, v3 6.18, v4 6.09;
- 50% of worker turns hear at least one TASK word; 26% of worker turns before r60 know no home.

**Eats in r0-59 by zone** (own/mid/far by BFS from the starters; arena duel replays, ours v ring-contest):

| version | duel | mid | far | spread@50 |
|---|---|---|---|---|
| v1 | server | 5.66 v 4.12 | 4.69 v 3.87 | 11.4 v 10.5 |
| v1 | official | 10.6 v 7.55 | 6.95 v 6.8 | |
| v3 | server | 4.40 v 4.46 | 3.79 v 3.65 | 11.6 v 10.1 |
| v4 | server | 4.81 v 3.76 | 3.74 v 4.75 | 10.2 v 10.4 |

The fan-out signal was clearest in v1 (the version that lost open maps). In v4 it's weak: more middle eats, fewer
far eats, the same spread.

**Replayed losses** (v1, same map, seed and seat as arena-r17):
- **167062** (fixed-crowded v codex-queen-value-1): was an r86 elimination loss, now a WIN. r0-59 eats own/mid/far
  went 15/12/18 → 25/31/26, and heads in mid/far at r20 went 6 → 5 of 11.
- **167601** (wg39001 v queen-ring14-portal-safe-1): still a loss. Every worker is a harvester or contester r10-r30,
  because home food matches them all, so none explore. Heads stay in our zone, and the enemy's far-zone heads eat
  on our side. The task board doesn't touch this; their economy is far bigger (61 v 31 dragons at r100).
- **167425** (mg39509): still lost (r54 elimination). Mid eats 24 → 32, and heads reach the far zone by r40.

## Arena: ring-dispatch-1 (parent ring-contest-4), draft; beats_parent False
- **vs ring-contest-4: 47-0-65** (42.0%, CI 33.2-51.2, within noise). Decided by elimination 23-26, queen 18-18,
  longest dragon 6-20.
  - By suite: server 31-37, official 4-6, custom 12-22.
- **Other opponents:** ring-sprint-1 53-59, ring-sched-17 61-51, ring-rush-1 65-47. Overall 226-222.
- **Seat gap:** seat A 132-92, seat B 94-130.
- **CPU:** p99 24.2M, max 41.1M.
- **Eval P(win) v the parent** (game.eval): r50 0.503, r100 0.497, r200 0.515, r300 0.510.
- **Board control and eats v the parent:**
  - board control r60 0.466 v 0.482, r100 0.466 v 0.486;
  - pearls eaten r60 61.1 v 59.7, r100 134.5 v 134.3.

**Weak-spot check on the draft's own games v the parent** (111 replays decoded, r0-59 eats by zone, us v them):

| games | mid | far | spread@50 |
|---|---|---|---|
| all | 10.46 v 10.05 | 7.01 v 6.86 | 9.89 v 10.55 |
| server | 4.27 v 4.97 | 3.42 v 4.01 | 10.6 v 11.25 |
| our losses | 12.0 v 14.2 | 5.2 v 7.5 | |

**Not fixed: the bunching remains.**
- **184319, devil, seat B (L).** We ate 0 middle or far pearls in r0-59 against their 30 middle and 14 far. Our
  heads stay in our zone through r60, while the parent has 4-5 heads mid/far from r40.
  - Reproduced locally with DISPATCH_DBG: r20-50 workers are mostly HARVEST, because home food matches them all.
  - The few explorers do take distinct sectors (6, 2, 7) but don't reach the middle.
- **183968 and 183974, fixed-crowded (L, r89/r93 eliminations).** Middle eats 22 v 33 and 22 v 34.
- **The pattern** is the same as 167601. A board that dispatches only the workers the food can't use doesn't move
  harvesters off a rich home.

## Next
- An explicit explorer quota from the board: for example one EXPLORE task per k harvesters, or per unclaimed
  sector with no ally in it, even when home food matches everyone.
- The quota should be set by the frontier's value (unseen share x distance), not by a fixed count.
- v1's in-matching hysteresis gave the clearest fan-out but broke matching determinism. A version that carries each
  dragon's prior target in its TASK word, and applies it only for dragons whose word we heard, would keep both.

## ring-dispatch-2: the explorer quota (follow-up, same variant directory)
The orchestrator asked for the fix: a minimum number of explorers set by the value of unexplored ground, even when
home food could feed everyone. Code: `strategy.cpp` ChooseGoal, marked g43-2b.
- **Quota.** Before r60, n workers in view, at least round(n x 0.5 x unseen share) explore. Never a lone worker,
  never all of them.
- **Who goes.** The harvesters with the latest food match go first, ties to the higher id, then the food is matched
  again without them. Every dragon that sees the same allies picks the same explorers.
- **Stickiness.**
  - A quota explorer keeps the role for 10 rounds.
  - An ally in view whose last TASK word says EXPLORE is left out of the food matching. This is the previous task
    carried in the sonar word.
- **What a quota explorer values.**
  - Its own sector's frontier scores x2.
  - Food scores x0.3 unless it is contested, or within 3 steps and further from home than the explorer's head.

**Trials** (arena duels vs ring-contest-4, server suite, random:2). Spread is the mean BFS distance of worker heads
from our starters, at r30 and r50.

| version | W-L | mid+far eats r0-59, us v them | spread@30 | spread@50 |
|---|---|---|---|---|
| quota share 1.0 | 32-36 | 8.7 v 9.0 | 8.36 v 8.89 | 10.24 v 10.62 |
| quota share 0.5 | 36-32 | 7.3 v 9.9 | 9.19 v 9.65 | 10.12 v 11.29 |
| 0.5 + 10-round hold + explorer words | 37-30 | 8.8 v 10.6 | 8.65 v 8.66 | 9.9 v 10.7 |

- **Quota explorers.** In the devil seed-13802921253180459702 sim they rose from about 10% of worker turns to about
  30% with the hold. About half of those turns still took food.
- **Fresh mapgen**, before the last food tweak: 78-98. Open 4-12, crowded 5-11, sealed-spawn 5-11.
- **Local maps/server**, seeds 1,2: 37-31.

### Arena verdict: ring-dispatch-2 (parent ring-contest-4), beats_parent TRUE (within noise)
- **vs ring-contest-4: 58-0-54** (51.8%, CI 42.6-60.8). Server maps 35-33.
  - Decided by: elimination 25-23, queen 22-20, longest dragon 11-10.
- **Other opponents:** ring-sprint-1 50-62, ring-sched-17 58-54, ring-rush-1 71-41.
- **Seat gap:** seat A 139-85, seat B 98-126.
- **CPU:** p99 24.1M, max 47.9M.
- **Eval P(win) v the parent:** r100 0.504, r200 0.539, r300 0.537.

**Weak-spot check, paired** (the draft's own 111 games v the parent; same games, so same seeds; r0-59, us v them):

| games | middle eats | far eats | spread@30 | spread@50 |
|---|---|---|---|---|
| all | 10.5 v 9.8 | 6.4 v 7.1 | 8.59 v 8.58 | 10.03 v 10.36 |
| server | 4.0 v 5.3 | 3.7 v 4.4 | 9.04 v 9.05 | 10.89 v 11.34 |
| our 53 losses | 5.0 v 9.9 | 6.0 v 10.0 | 7.7 v 8.0 | 9.25 v 10.35 |

**NOT fixed.**
- **187477** (devil, the same seed as 184319): we ate 0 middle or far pearls, against their 31 middle and 12 far.
  Spread@50 was 7.3 v 13.8.
- **187444** (devil): middle eats 3 v 33.
- **187354 and 187351** (fixed-crowded): far eats 10 v 27 and 12 v 37.
- **The win came from later in the game,** not the opening: eval P(win) is 0.50 at r100 and 0.54 at r200.
- **Why the quota doesn't move it.** Explorers carry the hunger and unseen multipliers but still eat home food on
  the way; seat B collapses. In losses the gap is the parent's fan-out, not ours.

**Next:** make the quota explorer's goal the sector frontier itself (as RushGoal does for the starters), not a score
multiplier that home food can outbid. Measure it by mid/far eats in seat B losses.
