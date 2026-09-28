# strategy-v1

The first scripted strategy, submitted to the competition server as v3. Hand-tuned in
syeyoung-rl (`strategy/`, commit e111d4d) against the RL policy; the full record of
what was tried is `strategy/EXPERIMENTS.md` there.

## Idea

Explore, gather pearls with many small dragons, feed workers into leaders late, trade
dragons head-to-head only when it pays.

- **Roles:** a dragon alive at round 0 is a leader (never feeds, trades only for a longer
  enemy); split children are workers.
- **Targets:** every reachable tile scored by value per round to collect it: pearls
  (discounted by age), beds by their countdown (`max(distance, rounds until due)`), the
  unexplored edge. Dead ends are worth their pocket's value less the cost of splitting out.
- **Safety:** a step must leave room (flood fill), and no enemy head we won't trade with
  may reach the tile. Trapped: split, so the child walks out the way we came.
- **Numbers:** split from length 4 (numbers win the one-for-one head-to-head attrition on
  crowded maps). Trade evenly only for an enemy at least as long; when outnumbered, only
  for one twice as long.

## Measured (native engine, vs the RL policy at update 28193)

22-0 on the 11 official maps; 79-1-0 on 40 pool maps. Weak spots never tested: other
teams' styles.
