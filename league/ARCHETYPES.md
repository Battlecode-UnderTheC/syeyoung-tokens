# Archetypes

Strategy ideas for **fresh** variants. Each is a direction, not a spec; a worker makes it
concrete. The orchestrator adds an archetype when a new idea comes up, and notes the
variants that tried each one and how they did. The `archetype` field in `variant.toml` names
one of these, or a new one added here.

| Archetype | Idea | Tried by |
|---|---|---|
| `gather-split-trade` | Explore, gather with many small dragons, split early (length 4), trade head-to-head only when it pays, feed workers into leaders late. | strategy-v1 (Elo ≈1740), gather-split-trade-1 (1892), gather-split-trade-2 (1724, tweak) |
| `giant` | Grow one dragon as long as possible and keep it safe: never split it, avoid every enemy head, let small helpers bring pearls to its path or merge by dying next to it. Wins the round-500 longest-dragon count. | giant-1 (1775) |
| `swarm-hunt` | Many small dragons that actively hunt enemy heads for one-for-one trades while outnumbering them, to eliminate the enemy team before round 500 (an empty team loses at once). | swarm-hunt-1 (1873) |
| `territory` | Claim and hold a region with rich beds: camp bed clusters on their countdowns, keep the enemy out by body-blocking corridors and chokepoints. | |
| `bed-timer` | Precise bed economics: memorise every bed's countdown and route to arrive as each respawns (shared bed countdowns under symmetry). Pure efficiency, minimal fighting. | |
| `sonar-pack` | Coordinate through sonar (`PROTOCOL 3`, 64-bit words): share bed timers, enemy sightings, claimed targets, so dragons stop duplicating work and converge on kills. | sonar-pack-1 (not submitted: no gain) |
| `portal-play` | Map portals early and use them: shortcuts to pearls, escapes from chases, ambushes at portal exits (a stepping enemy can't see the exit side). | |
| `body-wall` | Use long bodies as walls: cut the enemy off from pearl fields, trap enemy heads in pockets so they die without valid moves. | |
| `late-surge` | Play safe and small until late, then merge (feed) everything into one dragon in the last rounds to win on the longest dragon. Timing is the whole game. | |
| `counter-swarm` | Specifically beat many-small-dragon opponents (like strategy-v1): avoid even trades, punish splits, win on total length. | counter-swarm-1 (1907) |
| `trading-swarm` | Clone of the first real server opponent (split-v7): always `SPLIT 2`, splits to the end, starts every head-to-head it can (even trades, lunges at longer dragons), no sonar, greedy eating, no consolidation. A sparring partner for the style that beats us on crowded maps. | |
| `portal-farmer` | Find portal-only regions early (areas reachable only through portals, which on real maps hold up to half the beds: Default, Trauma), send a few dragons in to claim and harvest them on their bed timers, and keep the entrances. The strong server opponent won by farming these stores. | |
| `swarm-open` | Open like the trading swarm (greedy nearest pearl, SPLIT 2 at every chance, ignore contest discounts) for the first ~100-150 rounds to win the early eating race, then switch to our consolidating game (champion, feeding, cap give-up). | |
