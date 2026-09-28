# swarm-hunt-1

## Idea

Many small dragons (split from length 4) that hunt enemy heads with the **lunge**: a MOVE
may take up to length-1 steps in one turn (every step after the first costs a tail
segment), so a dragon of length L reaches an enemy head up to L-1 steps away along free
tiles before the enemy can react. Nothing else moves during our turn, so a lunge seen is a
lunge landed (a head-to-head kills both). Opponents that only keep off the tiles *next to*
enemy heads (strategy-v1) never see it coming.

- Keepers (dragons alive at round 0) split off hunters, trade only for a longer enemy and
  keep out of enemy strike range (up to 2 steps, by the enemy's length); they are the
  round-499 fallback. Split children are hunters: gather, split, close in on enemy heads
  within 6 steps and strike/lunge.
- Trade rule for hunters: an enemy at least as long (strategy-v1's rule; lengths seen are
  lower bounds, so this is mildly aggressive).
- Less self-jamming: room checks don't count tiles next to an ally's head (the ally may move
  there), and a dragon leaves a pearl/bed/explore target to an ally head that is nearer.

## What changed from the parents, and why

Fresh variant. The movement/target core (board memory, value-per-round goals, dead-end
pockets, split-when-trapped) is adapted from strategy-v1 into namespace `swh`; new are the
lunge, the keeper strike-range guard, and the two crowding fixes.

Diagnosis that drove the crowding fixes: with `SWH_DEBUG=1` the agent dumps its window when
it resigns "trapped: no move". Most such deaths (hundreds per worldgen match for both
strategy-v1 and this variant) are small dragons boxed in by their **own allies** plus kelp,
not by enemies.

## Measured (native sim, 2026-09-28; machine heavily loaded)

| opponent | maps | W-D-L |
|---|---|---|
| anchor-random | official (22) | 22-0-0 |
| anchor-random | worldgen practice set A (40) | 40-0-0 |
| anchor-greedy | official | 21-0-1 |
| anchor-greedy | worldgen A | 40-0-0 |
| strategy-v1 | official | 14-0-8 |
| strategy-v1 | worldgen A | 30-0-10 |
| strategy-v1 | worldgen B (40 maps, seed 777) | 60-0-20 |

Ablations vs strategy-v1:
- lunge off: worldgen B 46-0-34 (vs 60-0-20 with it), official 16-6 (noise-level).
- base (lunge, no crowding fixes): official 10-12, worldgen A 26-14.
- crowd-aware room only: official 12-10, worldgen A 23-17; ally-yield only: 10-12, 34-6;
  both: 14-8, 30-10.

`check`: all checks passed; slowest turn is 0.1-0.4 ms on most maps, occasional 4-9 ms
spikes seen only while the machine load was ~80 (noise).

## What should improve

Beat strategy-v1 on worldgen maps clearly (~75%) and on official maps modestly.

## Known issues / next

- Keeper detection uses `round == 0` on the first turn, so children born in round 0 also
  become keepers (seen in debug dumps). A fix: a child's first observation carries ECHOES
  (it inherits PROTOCOL 3), a map dragon's first does not.
- "trapped: no move" is still ~50/match on official and ~360/match on worldgen maps: the
  biggest lever left. Ideas: never enter 1-wide dead ends at length 2 (can't split out),
  stronger spreading of the swarm, account for ally tails that vacate.
- Not measured: enemy-count estimation for more aggressive trades (trade_rule 1/2 exist
  as env params `SWH_TRADE_RULE`), split length (3+3 hunters lunge further), lunge defense
  against opponents that lunge too.
