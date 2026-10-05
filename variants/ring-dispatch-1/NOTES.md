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

## Arena
(filled in after the verdict)
