# Gate audit of ring-sched-m1 (arena ring-sched-6)

This audit lists every gate in the variant: a condition, threshold or constant that changes
behaviour. Line numbers are for the parent, `variants/ring-sched-m1/`, as audited. The last
column says what ring-clean-1 does with each gate.

Classes:
- **A**: general. It follows from the rules or from a clear observed cause.
- **B**: a tuned threshold on an observation. The note says what cause it stands in for.
- **C**: a map-shape assumption, such as board size, the geometric centre or a fixed position.
- **D**: a fixed round gate. "rules" means it is tied to the game's fixed 500 rounds (0..499).
  "tuned" means the round was picked from experiments.

Compile-time modes that are switched off (`RING8_MODE 0`, `RING10_DANGER_MODE 0`,
`RING11_RESCUE_MODE 0`, `QUEEN_GUARD 0`, `QUEEN_ENDGAME 1`, so the delivery/courier code never
runs, `SUPPLY14_MODE 1`, so the SUPPLY14 modes 2 and 3 never run, `QUEEN_FEED_MODE 1`,
`kPark 0`, `kSonarShare 0`, `kEarlyHoldRound 0`, `kCautiousMax 0`, `trap_loss 0`, `survivor_units 0`,
`hunt_radius 0`, `outnumber_factor 0`, and the `if (false && ...)` at strategy.cpp:1328) are dead
code. They are not counted below. Their round gates (430/440/450) would be class D if switched on.

## Summary

| class | rows below (individual constants) | changed in ring-clean-1 |
|---|---|---|
| A general | 15 rows | none, as the brief says |
| B tuned on observation | 45 rows (about 100 constants; B27 alone holds about 50 `Params` fields) | 2: the forage gate is rewritten on its cause; the give-up unit floor is dropped |
| C map shape | 3, plus 1 protocol bound that is kept (C4) | all 3 removed; C1 replaced by a bed-density frontier pull (observed) |
| D round gates | 4 rules-based round values (499, 498, 500, 480), about 20 tuned ones (430, 450, 360, 320/400, 280/380, 270, 250, 150, 120, 200, 100, 80, and search7's own) | none, listed only |

## C: map-shape assumptions

| # | where | what | action |
|---|---|---|---|
| C1 | strategy.hpp:224-230, strategy.cpp:497-501, 753-754, 815 | **centre pull**: before r50, on boards of at most 1024 tiles (`centre_max_tiles`), every goal score and explore score gains up to x1.3 nearer the geometric centre `(W/2, H/2)`. Source: body-wall-17 / g15-20 ("on small maps the swarm takes the central field first"). | **Replaced** with an observed equivalent (new B gate `field_*`). Removing the pull outright cost a lot on devil: locally 1-19 vs the parent over 10 seeds, and 10-10 with the pull restored. A trace showed why. On devil both homes are bare, and food lies in 2-wide strips walled by kelp. Without the pull our dragons pile into the first strip at x 8-10 and trap each other, while the parent's spread to x 17-23. "Away from our start tile" did not help (1-19). The kept replacement: before r50, an unexplored-edge tile's explore score gains up to x2 by the share of beds among the known tiles within radius 3 ("food fields continue past what we see"). Devil 4-16 locally, generated maps 69-1-62 (the plain removal scored 64-1-67). **Devil remains a loss**: the old pull's value there came from the map's layout. |
|
| C2 | search7.cpp:675-679 | arena-search-7's explore value gets x(1 + 0.6·closeness to `(W/2, H/2)`) before r80. | **Removed.** This had no effect: the search agent's steps are only used from r250 (plugin.cpp:18 `kLateRound`), and its sonar is discarded. |
| C3 | strategy.cpp:18 `kGiveUpMinUnits = 32` | A 2-3-long stub gives up its slot at the cap only when `units_ > 32`. 32 is half the default `UNIT_LIMIT` of 64. With the default limit the rule is redundant with `FreeSlots() <= 1`. With a limit of 33 or less the rule silently turns off. | **Removed** (kept in the B list as B-give-up). The cap is now read only from `FreeSlots()`, i.e. the `UNIT_LIMIT` the spawn block gives. |
| C4 | ring.cpp:37, 135, 192, 451 | Ring rectangles must not wrap the board edge (`x+w <= width`), and ring/report words pack x and y into 6 bits (Encode strategy.cpp:309, RingWord ring.cpp:24). | **Kept.** This is a protocol bound, not a guess about map shape: a 6-bit coordinate holds boards up to 64. The contest maps are 11-64 a side, but the rules allow 7-256 (docs/game.md). **Risk:** on a board larger than 64, sonar positions alias (the decoder drops x/y >= width, but positions above 63 wrap mod 64). A ring that could wrap the board edge would need another encoding. Fixing this means changing the protocol, so it is left for a separate change. |

Comments that name maps next to constants show where tuning came from:
- strategy.hpp:110-116 (`kFinal`, `kBigTeam`, ...): stronghold 10-0, autarky 8-2, big_empty, schooltime.
- strategy.hpp:224 / strategy.cpp:497 (C1): body-wall-17, "small maps".
- strategy.hpp:261, strategy.cpp:879 (`opening_pearl_factor`): "Devil's open centre".
- strategy.cpp:269 (`kCautiousMax`, off): Schooltime server-seed trace.
- strategy.hpp:113-114: the `kBigTeam` 48 split was picked from big_empty/schooltime vs stronghold/autarky.
- strategy.hpp:475 (`kQueenWorkCap`): sandbox, big_empty (CPU calibration only).
- sym.hpp:68 (`kConfirm`): devil and schooltime differ from their wrong mirror in only a few walls.
- Many `Params` comments cite variants (`body-wall-15`, `gst115`, `late-surge-17`, `league-6`) rather than maps.

## B: tuned thresholds on observations

**Changed in ring-clean-1:**

| # | where | gate | stands in for | action |
|---|---|---|---|---|
| B-forage | strategy.cpp:1291 | queen forages from r0 when `units_ <= 2 && length_ <= 3` | **The team cannot make dragons.** A split needs length >= 4 (rules: 2 <= n <= L-2). A queen of 3 or less with at most one partner cannot produce workers, so the ring has no one to feed it. "<= 2 units" matched the known 2-starter maps, where the second dragon is also short. | **Rewritten.** The queen forages when it cannot split (`length_ < 4`, from the rules), no ally in view at r0 can split (seen length >= 4), and the team has fewer than `kForageSettleUnits` (6) dragons. The last condition is the existing settle rule, so a team of 6 or more would settle on the next line anyway. Effect on the known maps: no change on the 1- and 2-starter maps (arena, default_small, queen_of_spades, trophy, stripes, tower_defense still forage; Colosseum, stronghold, trauma, maze and weakhold still don't). The queen now also forages on portals (3x length 3) and islands (3,3,2,2,2), and on dilemma when its length-11 ally is out of view. |
| B-give-up | strategy.cpp:1883 | `units_ > kGiveUpMinUnits (32)` | at the cap | removed (see C3) |
| B-field (new) | strategy.hpp `field_until 50`, `field_radius 3`, `field_bonus 1.0` | replaces C1: in the opening, explore the frontier next to seen beds first | "food fields continue". The constants are new tuned values (r50 is C1's old window). |

**Left as they are.** For most of these the cause is either directly observed already or has no
simple observable stand-in:

| # | where | gate | stands in for / why kept |
|---|---|---|---|
| B1 | strategy.cpp:1287 | `round_==0 && units_<=1`: solo queen roams/harvests (EARLY15 6) | already the cause ("no other dragon exists") |
| B2 | strategy.cpp:1296, hpp:27 | forage ends at `units_>=6` or `r>=120` (`kForageSettleUnits`, `kForageSettleRound`) | "the colony now feeds itself". 6 and 120 are tuned. A direct measure (worker food income) is not simple. |
| B3 | hpp:27 `kForageRoom=20`, strategy.cpp:1489-1495 | foraging queen: -20 per corridor tile, -2 per missing tile of 20 open room | queen-safety rule (user: the queen never enters corridors or pockets). Kept. |
| B4 | strategy.cpp:1314-1315 | non-forage roaming settles at `units_>=4 \|\| r>=200` | same idea as B2 |
| B5 | strategy.cpp:1353-1356 | queen bootstrap split at length >= 4 unless threatened or in a corridor while foraging | split legality (A) plus safety (kept) |
| B6 | strategy.cpp:1359 | queen sheds to 6 when length >= 8, before r360 | "keep the queen short so it fits its ring". 6 = ring size (A); 8 is tuned. |
| B7 | strategy.cpp:1487 | home tether: `0.6` (before r360) / `0.1` per tile beyond 6 from the queen's start tile | stay near the ring. The anchor is the queen's own observed start tile, not a board position. Weights are tuned. |
| B8 | strategy.cpp:1482-1488 | queen route weights: pearl +12, food x6, room 0.03, liberties 0.3, risk 10000/1000, dead end -1e6, threat -100, `QueenRisk` reach `min(8, ceil(L/4)+L-2)` | the reach is the rules' sprint reach (A), capped at 8 for CPU. The weights are tuned. |
| B9 | strategy.cpp:1343, ring.cpp:73, 170, rescue.cpp:78 | enemy threat reach `min(8, max(3, ceil(L/4)+L-2))`, 8 when only part of the enemy is seen | the rules' reach (A); the clamps 3..8 are tuned |
| B10 | strategy.cpp:1401, 1431 | queen beam: at most `min(6, ceil(L/4))` steps, width 8 | free sprint steps (A); 6 and 8 are CPU bounds |
| B11 | strategy.hpp:477 `kQueenWorkCap=100000` | CPU cap on the queen's search | CPU |
| B12 | ring.cpp:30 | ring bootstrap split when length >= 4, before r100 | split legality (A) |
| B13 | ring.cpp:34-57 | ring = 6-cell loop; score: -3/-8 per ally/enemy head within 3, -2 per bed on the loop, >= 2 exits | ring geometry (A); the weights are tuned |
| B14 | ring.cpp:82 | ring pearl split when length >= 4 (4-loop) or 5 (6-loop) | loop capacity (A) |
| B15 | ring.cpp:126, 137, 189, 201 | ring/delivery words stale after 12 rounds (2 for delivery); courier reselected every 12 rounds | message freshness, tuned |
| B16 | ring.cpp:149 | workers keep the loop plus 2 exterior tiles clear | the queen's exits (safety) |
| B17 | ring.cpp:100, 117 | child escape: 6 steps, beam 32 | CPU bound |
| B18 | rescue.cpp:43, 90, 105, 123 | rescue: 1 free exit required (mode 2), 8-step beam of 32, escape cap max(12, 2L) | safety, CPU |
| B19 | queen_attack.cpp:25, 52 | attack the enemy queen within 12 steps, budget 1024; paid steps only with `units_-1 >= kLungeMinWorkers (4)` | "we have workers to spare". 4 is tuned. |
| B20 | strategy.hpp:17-18 | scheduling: horizon 12, 12 dragons, harvest 8, others' tiles x0.1 | tuned |
| B21 | strategy.hpp:11 `kHungerRounds=20` | the explore multiplier reaches x2 after 20 rounds without food | tuned |
| B22 | strategy.cpp:18-19 | give-up: length <= 3, free slots <= 1, idle goal > 6 steps, ally length >= 6 next to the head | "a long ally needs the slot to escape". Tuned. |
| B23 | strategy.cpp:22, 29 | cap-aware growth within 4 of `UNIT_LIMIT`; `kFeedMaxLen 8`, `kParallelGrowLen 16`, `kCapRelease 12` | relative to the observed limit; the margins are tuned |
| B24 | strategy.hpp:116 `kBigTeam=48` (latched at r270) | a team of 48 or more delays its split/feed schedule by 80 rounds | "food-rich board" (team size stands in for food income). Tuned on big_empty/schooltime vs stronghold/autarky (see the comment). A food-income measure would be the real cause but is not simple. Kept and flagged. |
| B25 | strategy.hpp:126-139 | chop feed at length >= 6 into 3s; feed only length >= 2; reserve 4 tiles around the champion for 20 rounds; body reach 2; walk end 480 at 3 rounds/tile | ceil(L/2) pearls on death (A); the rest is tuned |
| B26 | strategy.hpp:144-145 | trade value: share radius 5, margin 0 | the remains split (A); the radius is tuned |
| B27 | strategy.hpp:158-376 `Params` | split_min_length 4 (rules), feed_distance 3, ally_memory 80, report_memory 40/80, fade 0.5/0.25, obit 5/10/20, pearl_decay 0.97, ripe_bed 0.6, explore 0.25, wait_factor 2, cluster 0.5 r2 pool 40, crowd_threshold 5, stale 50/4, trap_cost 3, trap_candidates 8, opening_pearl_radius 40, factor 1.0, room_margin 3, room_min 8, escape 12/24, share 0.1, fan_out r80 radius 3, crowd discount r3 x0.55, contest 0.3, enemy_claims 2, enemy_memory 60, lunge_reach 6, feed_clear 0.8, merge_min 10, cap_margin 2, reserve 0/6, cross_until 480, blind_fallback 2, poor 1/3/35/x4/6, loop 12/6/50, hot gap 3, hot bonus 1, hot claim 5/8/2/30, no_trap_min_length 10 | each is one tuned weight on an observed quantity. None reads the board's size or a position. Too many to rewrite one by one, and each changes behaviour only a little. |
| B28 | strategy.cpp:95, 184 | vision border at distance 3 (`kVisionRadius`) | rules (A) |
| B29 | strategy.cpp:391 | queen report valid for 80 rounds | tuned |
| B30 | strategy.cpp:651 | an enemy head claims its 2 nearest valued tiles within radius 8 | tuned |
| B31 | strategy.cpp:997 | legend-bud: before r360, length >= 6 buds a 2-long child | tuned (the round is in D) |
| B32 | strategy.cpp:1130, 1137 | blind-portal fallback memory: 30 rounds | tuned |
| B33 | strategy.cpp:1192 | a trapped stub keeps one more segment when length >= 8 so the child hears it | sonar geometry (A) |
| B34 | strategy.cpp:1216 | lunge BFS: 256 nodes | CPU |
| B35 | strategy.cpp:1245 | FeedAhead: 2 steps | off (queen feeding disabled) |
| B36 | strategy.cpp:1784 | seek the champion's body within 3 tiles | vision radius (A) |
| B37 | strategy.cpp:1364-1383 | corridor walk depth 64 | CPU |
| B38 | ring.cpp:288 | GrowthReady: 2 outside pearls, 3 eaten | off (the caller is `if (false && ...)`) |
| B39 | ring.cpp:417 | feed14: sacrifice when a body tile is within 3 of the queen's head, 450..498 | tuned (the round is in D) |
| B40 | sym.hpp:53 `kConfirm=12` | symmetry is used after 12 agreeing pairs | a guard against a wrong mirror. The inference itself only uses what the dragon has seen. It relies on the rule that a map's beds match their mirror (docs/game.md, "A map may declare SYMMETRY"). On a map with no symmetry, contradictions kill every candidate, so nothing is mirrored. |
| B41 | plugin.cpp, rollout_giant2.hpp:27-35 | rollout judge: horizon 8, 16 rollouts, eps 0.15, trade 0.5, lunge 0.5, margin 1.0, model radius 14, pearl age 25 | tuned Monte Carlo settings |
| B42 | search7.cpp:22-47 | lookahead depth 8, gamma 0.9, kSmallTeam 14, kChampMin 6, reply p 0.25, remains radius 5, field reach 30, goal pull 2, stale 0.35, unseen room 3; food sonar rich >= 20 and older than 25 rounds | tuned. Only the step direction is used, from r250. |
| B43 | search7.cpp:642-653 | explore goal: grid stride 3, at least 6 away, stale after 40 rounds | tuned; no board-size gate |

## A: general (kept)

- Torus distance everywhere (`min(dx, W-dx)`): strategy.cpp:94, 114, 247, 287, 336, 493; search7 `TorusDist`.
- Sprint reach and cost: free steps `ceil(L/4)` = `(L+3)/4` (strategy.cpp:1399, ring.cpp:99, rescue.cpp:46, queen_attack.cpp:51), the enemy reach `ceil(L/4)+L-2`, and paid steps need body > 2.
- Split legality: `2 <= n <= L-2`, so length >= 4, and `FreeSlots() > 0` (`UNIT_LIMIT` from the spawn).
- Queens are ids 0/1 (`IsQueen`, `queen_id_` from seen ids 0/1).
- Death leaves ceil(L/2) pearls (chop feed and the trade value).
- Turn order by id breaks bed ties (strategy.cpp:783).
- EscapeLevel / TurnOut / SplitSafe / Liberties: exact flood fills on the dragon's own known body.
- Bed hotness is read from countdown resets (`UpdateHot`): observed.
- Symmetry inference (sym.hpp) uses only the rule that mirrored beds share one countdown, and drops a candidate the first time an observation contradicts it.
- Sonar/echo semantics and the portal ping (Turn).
- `CanSplitNow` / `CanCountOnSplit` against free slots.
- Rescue exits: an observed queen head with its 4 neighbours.
- Ring child release when `FreeSlots()==0`.
- `round_ == 499`: the last turn, where the queen ignores food and dead-end penalties (strategy.cpp:1482-1485) and the ring splits by 2 (ring.cpp:83-84).
- `499 - round_` horizons (ring.cpp:319, 341, 381; strategy.cpp:1403, 1507): search depth clipped at the game's end.


## D: fixed round gates (unchanged)

Rules-based: these use the game's fixed end at round 499 and stand for "N rounds before the end".
- r499 last-turn handling (strategy.cpp:1364, 1482-1485, 1505, 1726; ring.cpp:83-84, 199, 212, 403, 423, 472).
- ring.cpp:278 r498 and ring.cpp:281 `500 - round_`; ring.cpp:9 `RING_RELEASE 500` (never fires).
- `cross_until 480` and `kWalkEnd 480` (walk to the champion before the end).

Tuned (picked from experiments, not from the rules):
- **r430**: queen endgame mode (strategy.cpp:1295 early roaming ends; 1324 Ring4Exit; 1364 closed branch; 1505 RING6 continuation; 1568 last portal escape; ring.cpp:456 the queen's neighbours reserved for workers). "70 rounds before the end": the queen leaves the ring and grows.
- **r450**: workers next to the queen feed it (ring.cpp:403 SUPPLY14 mode 1, ring.cpp:423).
- **r360** `kPantryFrom`: feeding phase (strategy.cpp:1740), queen shed (1359), home tether weight (1487), legend-bud (997), rollout/search `kSplitUntil` (rollout_giant2.hpp:35, search7.cpp:25).
- **r320/400** `kFinal/kBigFinal`, **r280/380** champion freeze, **r270** `kLatchRound`, merge `kFinal+30`, `late_report_from` `kFinal-60`.
- **r250**: `kNoBurnFrom` (parallel growth at the cap), plugin `kLateRound` (search steers), search7 `kSonarFrom`.
- **r150**: `kHungerUntil`, `kGreedyUntil`/`opening_end` (nearest-pearl fallback).
- **r120** forage settle, **r200** roaming settle, **r100** ring bootstrap, **r80** `fan_out_until`.
- search7: r280 small team, r320/340 small-team split/feed, r380 feed, r470 odd feed (off).

All of these assume a 500-round game, which the rules fix, but where they sit inside the 500 rounds was tuned.

## Found while auditing, not changed

- rescue.cpp:63-65 has a dangling `else`: `for (...) if (!advance(...)) {...} else predicted_valid = false;`. The `else` binds to the inner `if`, so any move that advances sets `predicted_valid = false`. As a result `would_close` never fires for real moves, and clearance only happens when our body already blocks the queen's exit (`before < desired && blocking`). Fixing it changes behaviour, so I left it for a separate test.
- `Remember` resets `p_.feed_from_round` from `kFinal`/`kBigFinal` every turn, and then `Decide` overwrites it with `kPantryFrom` (360). So the big-team shift affects only splitting, no_trap, merge and the champion freeze, not feeding.

## Result of the cleaning (details in NOTES.md)

- Forage gate on its cause (B-forage) together with the give-up floor (C3): server suite 37-31
  (54.4%) vs ring-sched-6. Gains on portals, islands and dilemma, the maps where it now fires.
- C1 centre pull removed (v1) or replaced by the bed-field pull (v2): server 28-40 (41.2%) both
  times. This is a clear loss on the known maps in both starter classes. The centre pull is a
  map-shape assumption that pays on the current map pool, and no observation-based stand-in found
  so far recovers it.
