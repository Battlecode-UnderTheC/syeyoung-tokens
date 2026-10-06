# ring-sprint-stale (g45-3; line ring-sprint, parent ring-sprint-6 = ring-sprint-conveyor)

## Idea
Strategist a g45 #1. On the server (server-r19), 13-17% of our r0-100 moves are circling (opponents
7-10%). The worst runs are 2-long stubs waiting at a bed tick for 79-94 turns, and on Devil we eat
33 of 34 pearls at home before r60. The idea: an idle worker goes to ground it hasn't seen for a
while instead of waiting at a home bed.

## What changed from the parent, and why
`STALE_MODE` (strategy.hpp/.cpp, ChooseGoal):
- **Who is idle:** before kHungerUntil (r150), a non-queen, non-champion, unparked worker that has
  no food to take now. That means the schedule gave it no tile within kSchedHorizon, it has no food
  at all, or its best food by value per round needs a wait at a bed (ready > dist).
- **What it gets:** an idle worker earns the frontier's credit, `explore_value x hunger_mult / (d+1)`,
  on every reachable tile that is out of view and hasn't been seen for at least S rounds.
- **S:** for each bed this dragon has read, take the largest countdown it read there (a tick resets
  the countdown to the new gap). S is the median of those values, or 10 before any bed is read.
- **Unchanged:** the contest, danger, trap and parking logic.
- **Label:** `stale: patrol`, counted in GM2_DEBUG builds as `STALE patrol-turns`.

## First try, dropped before submitting
The strategist's formula ramps the credit in: `min(1, age/S)` from age 1, with no gate. That let a
just-left tile nearby outbid a farther frontier tile, so workers dawdled.
- Server duels: 27-41 and 30-38 against ring-sprint-6. Official: 11-9.
- tiles_visited@200 fell in both server duels (0.434 v 0.459, 0.433 v 0.470).

The submitted version applies the credit only once a tile is S or more rounds stale.

## Local and duel numbers
- **Local:** 1 seed on maps/server, 16-0-18 against ring-sprint-conveyor. `stale: patrol` fired
  52 times per match, against 8556 `toward goal`.
- **Duel (eph-25c14c28), server suite, random:2:** 32-1-35 against ring-sprint-6.
  - 3+ starters: 22-1-17.
  - 2 or fewer starters: 10-18.

## Arena verdict (ring-sprint-14)
Against the parent ring-sprint-6: 55-0-57 (49.1%), "loses to parent (within noise)".

| suite | W-L |
|---|---|
| server | 31-37 |
| official | 5-5 |
| custom | 19-15 |

| decided by | W-L |
|---|---|
| elimination | 29-26 |
| longer queen | 16-21 |
| longest dragon | 10-9 |

Against the other bots (parent's record in brackets):

| opponent | ring-sprint-14 | ring-sprint-6 |
|---|---|---|
| ring-contest-4 | 59-53 | 99-72 = 57.9% |
| ring-sched-17 | 70-42 | 109-63 = 63.4% |
| ring-rush-1 | 68-44 | 109-63 = 63.4% |

- **Eval P(win) against the parent:** .483 at r100, .490 at r200, .496 at r300.
- **CPU:** p99 25.8M, max 42M.

## Did it fix the weak spot? No: the change is mostly inert
Source: decoded replays of all 110 games against the parent, both sides measured
(scratch script early_pair.py, with feat.py's circling definition and early.py's zones).

| metric (draft v parent) | value |
|---|---|
| circling r0-100 | 8.6% v 8.5% |
| bed-side circling turns per game | 43.9 v 45.2 |
| eats per 100 dragon-turns, r0-60 | 9.40 v 9.34 |
| own / mid / far eats r0-60 per game | 35.3/15.9/5.0 v 35.7/15.5/4.9 |
| dragons at r60 / r100 | 17.4/23.6 v 17.4/24.3 |

- **Half the games never diverge in the opening.** In 26 of about 55 seat-swapped seed pairs, play
  is identical through r100, and most of those pairs end with the same result. That includes all
  Devil (201459/201526, 201461/201469) and Prisoners Dilemma (201173/201228, 201229/201516) pairs.
- **Why it can't fire there:** S is the real bed gap, and these maps' gaps are long.
  - Devil beds are mostly [1,300] and [1,600].
  - Prisoners Dilemma beds are [150,250] and [480,490].
  - So S is about 70-270 rounds, and no tile is that stale before r100-150.
- **Example:** in PD 201173 our stub id 9 circles beside [150,250]/[480,490] beds in r44-56, and
  id 24 circles in r112-149. The parent's copy of the game is identical.
- **Arena self-play rarely shows the server's waiting pattern:** circling is 8.5% on both sides.
  Prisoners Dilemma is the exception, at 16% (29% in r60-100).

## What next
"Ground unseen for a regrow time" can't fire in the opening on maps with long gaps, and any short
threshold (the ramped version) makes workers dawdle near home. Two other levers:
- Value the waiting itself: a worker facing a W-round bed wait takes any goal it can reach and come
  back from within W.
- Address home-first valuation directly (strategist a #4: home food as a reserve when no rival is
  near).
