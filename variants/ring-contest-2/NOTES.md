# ring-contest-2 (g42-12): contest without the side effects

Base: ring-contest-1 (arena ring-sched-29). Fixes the two blunders the g42 replay review found
(league/generations/g42.md), as one change. Everything else is ring-contest-1 (notes below).

## Change
1. **Long dragons split as the base does.** The no-growth-split-while-racing gate (Decide step 4)
   now applies only to dragons shorter than `kRaceSplitExempt` = 5. Observed cause: on
   fixed-maze-wide (158994) the 6-long starter id4 raced r4-12 unsplit, 8 v 11 dragons at r40;
   a long dragon's split makes two racers (split_min_length 4 makes 2+2, so 5+ is the first length
   whose split leaves a racer of 3+).
2. **Explorer-eats only for contested or near food.** The explorer still takes ChooseGoal's food
   goal, but only if a rival head (same memory/drift as the contest boost) could reach it within
   kSchedHorizon (whoever is first), or it is within `kExplorerNear` = 3 steps (its own view
   radius, a detour of at most 3 turns). Otherwise it walks the frontier exactly as the base.
   Observed cause: big_empty 159150, explorers farmed the home corner to r30 (-34 total at r60).

## Behaviour (same maps and seeds, duel-85b9a002 vs ring-sched-17: 10-2)
- fixed-maze-wide s1592614637 seat A (vs 158994): id4 (6-long) now splits at r6 (-29: r11,
  -17: r8). r40 14 dragons/35 total (-29: 11/28; -17 in the same seat: 16/42). Won (was lost by
  elimination r276).
- big_empty s1592614637 seat B (vs 159150): explorers 3 and 5 leave the home corner by r15 and
  cross the board; r40 34 dragons/81 total (-29: 27/67; -17: 32/80); r60 172 (-29 151, -17 177).
- devil s1380...: the explorer id4 now goes for the top frontier at r8 rather than the
  uncontested centre pearls (first divergence); the centre is still reached by r24 by id6. That
  game was lost (the -29 one won); the other devil seed won both seats. r<30 splits unchanged.

## Local
- check maps/official: all passed.
- Fresh mapgen seed 51842297 (44 maps), sim seeds 1,2 vs ring-contest-1: 86-4-86.
  Per family (16 each): anti-centre 10-6, crowded 9-7, mixed 9-7, open 9-7, off-axis 8-8,
  ring 8-8, sealed-spawn 8-8, sparse 8-8, maze 5-4-7, rich-near-home 6-10, rooms 6-10.

## Arena duels (eph-d36bee79)
- vs ring-sched-29, server random:2: 38-30 (3+ starters 23-17, <=2 15-13; elimination 13-11,
  queen 18-14). Official random:2: 10-10.
- vs ring-sched-17, server random:2: 40-28 (3+ 24-16, <=2 16-12; -29 had 39-29 server).
  dragons@100 25.8 v 24.0, queen alive at end 63% v 57%. CPU max 33M.

## Arena verdict: ring-sched-36
- vs parent ring-sched-29: 55-0-57 (49.1%), loses to parent by the rule, within noise.
  Server 32-36, custom 15-19, official 8-2. Decided: elimination 26-24, queen 19-20, longest 10-13.
  Eval P(win) vs parent at r100/200/300: 0.484 / 0.495 / 0.486.
- vs ring-sched-17: 67-45 (59.8%; -29 had 63-49). vs ring-rush-1 74-38, vs cs-queen-split-tail-risk-v1-7 67-45.
- CPU: worst p99 23.8M, max 37.7M.

---

# ring-contest-1 (g42-6): contest food in the opening

Base: ring-sched-m3 (arena ring-sched-17). One change, the opening food contest, behind
`CONTEST_MODE` (strategy.hpp; 0 restores the base).

## Evidence it answers
- arena-r16 cluster A (554 early collapses, median turning point r20). Deaths are level, so the
  food is the problem. In r30-59 we ate 6 middle pearls to their 28, and splits in r0-59 were
  10.5 v 23.6.
- server-r16: on Trophy the explorer walks away from food it can see and splits on the way
  (r5-18). Total length at r100 was 18 v 88.

## Change (r < 60, workers only, never the queen)
1. **Contest boost (ChooseGoal).** Enemy heads seen in the last 20 rounds count as rivals, placed
   at their last-seen tile and allowed up to 3 steps nearer (`RivalDist`). A food tile (pearl, or
   a bed by its exact tick) that we reach no later than every rival scores x2, when a rival could
   reach it within 12 (kSchedHorizon). The x2 is the pearl we win plus the pearl they don't eat.
   - Why 20 rounds: in the A losses the enemy takes a food patch by about r16 and stays on it.
   - Why 3 steps: about a farmer's drift around its patch.
2. **Explorer eats.** An explorer (rush state 1) whose best ChooseGoal goal is food within 12
   rounds takes that food instead of the frontier. Food seen at spawn counts too. With no such
   food it explores the frontier exactly as before. No geometric-centre target.
3. **Splits.**
   - An explorer keeps its one free split, and may split again whenever dense food is in its
     7x7 view (ClusterScore >= 2).
   - No worker growth-splits while racing to food it contests (an explorer's food goal, or
     contest-won food), while that food is more than 3 steps away or a rival would arrive within
     1 round after us. A split is a turn not walked.
- New labels: "contest: race", "contest: explorer food".

## Local
- `check ring-contest-1 maps/official`: all passed.
- Smoke run, maps/server seed 1 vs ring-sched-m3: 16-18 (v1). The contest fires on about 40% of
  worker turns that have a rival in memory.
- Fresh mapgen, seeds 77042611 and 90311457, 44 maps each, sim seeds 1,2, vs ring-sched-m3:
  171-181 in total (86-90 and 85-91).
  - Per family (32 games each):
    - rich-near-home 23-9;
    - open 17-15, rooms 17-15;
    - mixed 16-16, sparse 16-16;
    - crowded 15-17, off-axis 15-17, ring 15-17;
    - anti-centre 13-19, sealed-spawn 13-19;
    - maze 11-21 (4 of the losses are one map, maze-s77042611-002, lost by queen in both seats).

## Arena duels vs ring-sched-17
- v1 (rival memory 3 rounds, explorer splits only at dense food), eph-c50437d6, server suite: 33-35.
  - Middle pearls in r30-59 were 6.1 v 7.9: worse.
- v2 (this code), eph-96a28b89:

  | suite | W-L | split | by elimination |
  |---|---|---|---|
  | server | 35-33 | 3+ starters 19-21, ≤2 starters 16-12 | 16-9 |
  | official | 11-9 | | |

  CPU max was 32.3M.
- Replay metrics for the server duel (us/them; from arena-r16's game.py on the duel replays):

  | metric | all | ≤2 starters | boards under 700 tiles |
  |---|---|---|---|
  | middle pearls, r30-59 | 8.2 / 7.5 | 6.9 / 3.5 | |
  | pearls eaten, r30-59 | 36.2 / 35.1 | 19.7 / 17.6 | 22.5 / 18.9 |
  | splits, r30-59 | 15.2 / 14.7 | | |

  Dragons at r100 were 24.1 v 24.0.
- **Replays.**
  - Trophy, duel-0c44a40a games 6 and 7: same seed and seat A, ours vs the base.
    - The base's second starter (id 2) stays on the top board edge from r28 to r44.
    - Ours (id 2) comes back into the cup by r36 (2:3 at the cup's top).
    - Our side holds the cup with 5-6 heads in r40-50. At r60 we had 9 dragons where the base
      in the same seat had 3.
    - The explorer still goes to the edge at r10-20: it hasn't seen the cup yet, and the
      dragons share no map knowledge.
  - Queen of Spades, games 62 and 63: the two games are identical up to r50, so the change
    doesn't fire there.

## Arena verdict
(pending; see the report)
