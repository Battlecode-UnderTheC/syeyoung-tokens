# ring-clustercall-3 (g42-11): NOT READY, not submitted

## Idea
ring-clustercall-2 (ring-sched-33) answers a teammate's early cluster call with a detour bonus.
g42-9 suspected that on food-everywhere boards (big_empty 3-11 overall, mapgen open 6-10) the
bonus pulls workers off equally good local food. Here the call is worth only its excess over the
food the answerer already has.

## Change (call.cpp: LocalSurplus, CallAnswered)
- `LocalSurplus()`: the densest field centred in our 7x7 view (the window a finder scores), as
  ClusterScore (live or soon-due beds/pearls within 2) less the other ally heads within kCallNear.
  This is the finder's surplus measure, applied to the answerer's own ground.
- The call's value becomes (1 + cb*(surplus-1)) - (1 + cb*(local-1)) (0 when local = 0). A call
  no denser than the local field is not answered. The value feeds both the detour bonus and
  CallGoal, so the call fades on uniform boards and keeps full strength with no field around us.
- Find, relay, ring word and the answerer cap are unchanged. No new constants beyond the view
  half-width (the game's 7x7 window). No map keys, no fixed points.

## Behaviour (CALL_DBG self-play counts, seed 1, dragon-turns where the call shaped the goal)
| map | -2 detour / walk turns | -3 detour / walk turns |
|---|---|---|
| devil (2 games) | 129 / 38 | 103 / 46 |
| big_empty (2 games) | 458 / 272 | 386 / 98 |
| mapgen open (8 games) | 2685 / 107 | 932 / 45 |

Devil answering stays; on open maps the call mostly fades, as intended. On big_empty the
straight walks drop but detours stay (its pearls sit on 750-round beds, so a view rarely holds a
field and the call keeps most of its value).

## Numbers (v1 = this code)
- Fresh mapgen (seed 61177, 44 maps, seeds 1,2) vs ring-clustercall-2: 99-4-73.
  anti-centre 10-6, crowded 12-4, maze 9-7, mixed 11-5, off-axis 9-7, open 5-11,
  rich-near-home 9-7, ring 10-6, rooms 9-7, sealed-spawn 7-4-5, sparse 8-8.
  Open recheck (seed 70913, 8 maps): 18-14, so open 23-25 overall. Open opening is ahead
  (r20 dragons 30.5 v 29.2, r50 40.2 v 38.4); the open losses are r500 "longest" coin flips.
- Arena duels vs ring-sched-33 (eph-7e746a58), two rounds each:
  - server random:2 31-37 and 30-38 (61-75). Starters 3+ 36-44, <=2 25-31. Queen length at
    end 9.6 v 10.9 and 8.3 v 10.0; "longer queen" games 22-39.
  - official random:2 6-14 and 8-12 (14-26). Colosseum 1-7, big_empty 5-3.
  - Total 75-101 (42.6%), each duel "within noise" but consistently behind.
- vs ring-sched-17, server random:2: 33-35 (base -2 was 60-60 on the ladder).
- CPU max in duels 34.2M.
- Local big_empty, 8 seeds vs ring-clustercall-2: 8-8.

## v2 (tried, reverted): local field = ClusterScore at our own head only
Reason: v1's max over 49 view centres is biased upward and faded the call on server maps too.
- Arena (eph-07936bdc) vs ring-sched-33: server random:4 68-68 (maze 1-7), official random:4
  16-24 with big_empty 1-7.
- Local big_empty 8 seeds: 4-12 (v1: 8-8). Open families: 6-10 and 15-17.
- So fading the call on big_empty LOSES there: g42-9's premise (the call hurts on big_empty) is
  not supported. The early metrics are equal; the losses come at r500 by queen/longest.

## Verdict
Neither version beats the base in light duels; v2 loses the target map clearly. Not submitted.

## Next
- The big_empty problem is probably not the call. Look at its late game (queens die in most
  local games; decided by queen/longest at r500) rather than the r0-48 call.
- The call surplus treats pearls on very slow beds (big_empty: 750 rounds) as regrowing food;
  the answerer cap "one per spare bed, since beds regrow" is wrong there. Weighting by regrowth
  is a candidate, but only if replays show answerers starving at an eaten-out cluster.
