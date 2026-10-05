# ring-clustercall-2 (g42-9, arena ring-sched-33)

## Idea
ring-clustercall-1 (ring-sched-27) calls the team to a rich bed cluster found before r40. Its
answerers walk straight there, so on trophy/devil dragons at r40 were lower than
ring-sched-17's (8.7 v 10.0), and the gain came only at r100+. Here the answerers keep eating
and splitting on the way, and only as many go as the cluster's surplus can feed.

## What changed from ring-clustercall-1, and why
One change, on the answer side of the call: `call.cpp` (CallMine, CallAnswered, CallGoal) and
hooks in `ChooseGoal`. Find, relay and the ring-word carrier are unchanged.
- **Detour bonus:** a food tile (pearl or bed) nearer the answered cluster than our head scores
  max(own score, (value + call value) / (ready + torus(tile, cluster) + 1)). Eating it and walking
  on is worth both, per the rounds the whole trip takes. The call value is -1's
  (1 + cluster_bonus * (surplus - 1)). The scheduler, crowd and contest discounts still apply
  afterwards, so food that another dragon owns stays discounted.
- **Straight walk only without food:** CallGoal (walk to the known tile nearest the cluster) runs
  only when the chosen goal is not food, i.e. exploring or no goal.
- **Answerer cap = surplus:** each worker ranks the workers it knows of and answers only if it
  ranks within the cap. Known means in view, or reported within 8 rounds; queens and workers
  already within 4 of the cluster are left out. The rank is by (torus distance, id). The cap is
  one answerer per spare bed, since beds regrow.
  - The first version capped at surplus / 2 (the pearls a 2-long worker needs to split). It lost
    prisoners_dilemma 0-12 to ring-sched-27 in duels: the capped workers stayed in the contested
    centre column. Local ablation on that map: no bonus 0-4, no cap 2-2, cap = surplus 4-4.
- Observation only: no board-size gates, no fixed points, no map keys. The CALL_DBG blocks are
  debug prints only.

## Numbers
- Arena duels vs ring-sched-27, light, random seeds, final version (b):
  - server random:2: 39-29.
    - Eval .58/.55/.55.
    - Dragons at r40 11.7 v 10.8, at r100 26.1 v 21.6.
    - 3+ starters 24-16, <=2 starters 15-13.
  - official random:2: 8-12. Colosseum went 0-4; a Colosseum recheck at random:6 went 6-6.
  - trophy, devil, prisoners_dilemma, autarky and slithery_fight at random:4: 20-20. Dilemma
    went 4-4 (version a: 0-8).
  - custom random:2: 38-30.
- Version a (cap surplus / 2): server 34-34, official 10-10, trophy/devil 12-12; r40 dragons +1.1.
  - On trophy/devil vs ring-sched-17, r40 dragons were 9.5 v 9.7 (ring-sched-27: 7.9 v 9.9).
  - Devil eval .60/.64/.68 (ring-sched-27: .42/.44/.45).
  - Trophy eval .47/.42/.42 (ring-sched-27: .37/.32/.33).
- Fresh mapgen, version b vs ring-clustercall-1 (seed 42977, 44 maps, seeds 1 and 2): 86-90 in
  total. By family:

  | family | W-L | family | W-L |
  |---|---|---|---|
  | anti-centre | 9-7 | off-axis | 7-9 |
  | crowded | 7-9 | open | 6-10 |
  | maze | 7-9 | rich-near-home | 8-8 |
  | mixed | 10-6 | ring | 8-8 |
  | rooms | 8-8 | sealed-spawn | 9-7 |
  | sparse | 7-9 | | |

- **Arena verdict (ring-sched-33):** 60-64 vs parent ring-sched-27, "loses" within noise.
  - Server maps only: 44-30. Custom and official were worse, about 12-22 and 4-6 after removing
    duplicate game rows. These are spread thin, 2 games per map; the custom duel above was 38-30.
  - big_empty: 0-2 vs the parent and 3-11 overall (duel 1-3), possibly real.
  - vs ring-sched-17 60-60, vs ring-s17-1 64-56, vs ring-rush-1 72-50.
  - Eval vs parent .494/.501/.506 at r100/r200/r300. By suite: server .50/.52/.53, custom
    .49/.45/.48, official .48/.51/.45.
  - Dragons at r40 vs parent: server 11.1 v 10.7, custom 19.8 v 19.4, official 15.2 v 15.2.
  - CPU p99 27.4M, max 35.5M.

## Replays (same seed and seat; before = a copy of ring-sched-27, after = this)
- **Devil**, seed 2336152673805268595, seat A: duel-8d595db2 game 10 (before) v duel-a05e4fc8
  game 10 (after).
  - Before: the top-left workers (2, 4, 6) go north to the edge pearls. Nobody touches the
    12-bed centre cluster until r42. 9 dragons at r40; lost.
  - After: workers 2 and 4 head east, eating the pearls on the way. Worker 2 is at the cluster's
    edge at r22 and splits there at r26. Four workers farm the cluster by r30, while 8, 10 and 14
    keep their own ground (the cap). 17 dragons at r40; won.
  - The mirrored seat (game 11) shows the same: 9 to 18 dragons at r40.
  - This is the intended behaviour.
- **Trophy** (game 9 pair, seat B, L to W): the centre is walled, and neither version enters it.
  The win comes from a different route and split in the south, not visibly from the call. The
  trophy gain is not clearly the intended mechanism.

## Next
- The server-suite gain (devil 12-4, maze, around_unsw) shows the intended behaviour.
- On open maps where food is everywhere (big_empty, open family 6-10), the bonus may pull workers
  off equally good local food. Next try: scale the call value by how much of the trip's food is
  better than what is nearby.
