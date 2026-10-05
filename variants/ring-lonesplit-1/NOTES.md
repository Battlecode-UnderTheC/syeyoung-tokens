# ring-lonesplit-1 (g42-7)

Base: ring-sched-17 (code: ring-sched-m3). One change: a lone start splits like a worker.

## Idea
arena-r16: 1-starter maps 76-95. In r0-59 we split 8 v 27 and eat 33 v 85 pearls. Colosseum
151187: the queen bootstrap-splits at r0, then grows to 7 without splitting (the mode-6 harvest
split needs units<=1), while the child circles home beds whose tick is 15-25 rounds away.

## Change (LONESPLIT_MODE, strategy.hpp / strategy.cpp)
While our team has at most kLoneUnits=2 dragons and round < kLoneUntil=60:
- **Queen** (`LoneOpenSplit`): the mode-6 harvest split (Split(length-2) at length 4+) also fires when
  she is on open ground (not a corridor tile, OpenRoom >= kForageRoom=20) with >= kLoneFood=2
  pearls seen this round within kSchedHarvest=8 steps. The existing guards stay: no threat on her
  head, body fully known, SplitSafe.
- **Worker:** a bed it would have to wait at for more than kSchedHarvest rounds gets no value in
  ChooseGoal, so it explores instead of parking.

## Local
- Trace, Colosseum seed 1, seat B: the queen now splits at r9 (length 4, units 2). The base grew to
  8, then shed at r19/r22.
- Colosseum, 40 seeds x 2 seats, vs ring-sched-m3: 43-0-37.
  - dragons r10/r20/r30/r40: 2.2/4.2/6.6/8.8 v 2.0/3.5/6.6/8.5;
  - queen died 69% v 74%.
  - An 8-seed run had read 6-10; that was noise.
- 14 one-starter maps (12 fresh mapgen across 4 seeds, plus Colosseum and arena), 3 seeds: 37-8-39,
  neutral. Instrumenting showed why: most of r0-59 the lone queen is length 2 with no pearls in
  sight, so the gate rarely fires.
- Fresh mapgen, seed 742917, 44 maps, seeds 1,2: 88-0-88. Openings were identical, so the change is
  inert outside lone starts.
- maps/official, seeds 7,8: 30-30.
- **Iteration 2** let the queen gate run to 3 units (kLoneQueenUnits=3): 37-8-39 on the 1-starter
  set, identical, and no better on Colosseum. Reverted.
- check maps/official: all passed.

## Arena
- Ephemeral eph-dbc10c50 duels vs ring-sched-17:
  - server random:2: 34-0-34 (3+ starters 20-20, <=2 starters 14-14), with nearly identical metrics;
  - official random:2: 11-0-9 (<=2 starters 9-7), dragons@100 34.7 v 30.9.
- Draft verdict: see below.
