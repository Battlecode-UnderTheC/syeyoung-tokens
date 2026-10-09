# ring-shield-rooms (g53-9)

Base: ring-sealed-shield (arena ring-shield-1). Strategist A g53 proposal #6 (stripes rooms), trace-first.

## Trace (step 1)
- Reproduced arena stripes losses of ring-shield-1 locally (build/tools/match, arena seeds; 267558 and
  267557 replay move for move v ring-contest-4) with -DWALLIN_DBG -DROOMS_DBG -DFIELD_DBG (goal, SafeStep
  per-direction breakdown, each later layer's step, the rollout judge's candidate values).
- Arena losses (19 stripes losses, decoded): first room-pearl eat by a worker r18-34 or never for us, r8-12
  for them; eats by r30 4.8 v 6.5, by r60 15.6 v 23.3.
- Worker turns with a room pearl as goal within 6 steps (stripes, 7 seeds v ring-contest-4): 266; the goal
  step was taken in 220. Of the 46 refused: **35 by the rollout judge** (plugin; its 8-round rollouts score
  the dead end as a dragon that dies, never as a split-out harvest; e.g. sim seed 1 r25 id5 at (6,4), no enemy
  near: gst's door step scored 0.25, the step away 1.94), 11 by SafeStep (in-room last tile, or the door step
  refused because the goal came from the founder/field/rush/nearest-pearl paths without ChooseGoal's
  dead-end mark: e.g. duel seed 715293079177898441 r25 id4 at (7,3), "detour (goal step unsafe)").
  RoomNeeded/Corridor/Exposed did not block approaches (room 8/8, exp0 at every door step).
- Many judge refusals were correct: an enemy dragon already inside the room (e.g. 267557's seed r20/r22).
- Also: the plugin's "rollout: ..." labels are overwritten by ProtectQueenAction's reason (pre-existing).

## Change (step 2: one behaviour, StrategyAgent::PocketAhead)
A worker's goal is "in a pocket" when it lies behind a single tile of the shortest path (the door; our head
counts) whose far side has fewer than RoomNeeded() tiles (SafeStep's own dead-end measure) and at least 2,
holds food in view, no other dragon in or beside it, no enemy head whose lunge reach (seen length +1 if
partial) covers the door, and a way out: a split (we are or grow to 4+ with a free slot) whose stub
(trap_stub, gst's g18-3 loss model) the food beats. Then the goal carries the dead-end mark (SafeStep
trap_ok/grow) wherever it came from, and the rollout judge keeps the goal step (like "dead end" farming
steps). Never the queen. No map keys, sizes, coordinates or new constants.
Iterations: v1 judge-only (no eat gain); v2 + dead-end mark (stripes room eats up, arena maze 0-4);
v3 + food > trap_cost (inert: stripes rooms hold exactly 3); v4 (submitted) food > trap_stub, split exit only.
Debug: -DROOMS_DBG traces, -DROOMS_DBG -DROOMS_OFF is the parent with the same traces.

## Local
- Stripes, 12 seeds as seat A, room-pearl eats by r60 / r100 (v4 v parent):
  v ring-contest-4 9.2 / 25.8 v 8.4 / 24.2, W 6-6 v 4-8; v ring-sealed-shield 13.8 / 41.9 v 12.5 / 38.1,
  dragons at r60 21.0 v 18.9, W 6-6 v 6-6. Workers lost in rooms before r60 (mostly planned stub resigns
  after the split-out): 2.1 v 1.75, 3.7 v 3.25.
- Maze 8 seeds x 2 seats v ring-sealed-shield: 8-8.
- Fresh mapgen --seed 53909 (44 maps x seeds 1,2) v ring-sealed-shield: rooms 8-8, maze 7-9, crowded 8-8,
  ring 8-8, sealed-spawn 8-8, open 8-8, sparse 8-8, mixed 8-8, off-axis 9-7, anti-centre 8-8,
  rich-near-home 8-8 (v1 89-87 overall).

## Arena duels v ring-shield-1 (server random:2 = 68, official random:2 = 20)
- v1: server 35-33 (stripes 1-3). v2: server 31-37 (maze 0-4, stripes 2-2), official 12-8.
- v4: server 31-37 (stripes 3-1, maze 1-3, portals 1-3, islands 1-3), official 12-8. CPU max 34.5M.

## Arena verdict (ring-shield-5, parent ring-shield-1)
- 49-0-63 (43.8%, within noise; "loses to parent"). Server 29-39, official 4-6, custom 16-18.
  Decided: elimination 22-28, queen 18-20, longest 9-13. v ring-shield-3 52-60, ring-sealed-2 52-60, ring-field-3 58-54.
- eval P(win) v parent: r100 0.482, r200 0.468, r300 0.462. CPU p99 23.5M, max 50.5M.
- Stripes v parent 1-3 (277108 L, 277098 W, 277084 L, 277072 L). Room-pearl eats by r60 15.8 v 12.5; first room eat
  r18/r28/r38 for us. The two seeds are the parent's own loss seeds; the opening (r0-30) is unchanged in them:
  the change fires at doors with food, but who reaches the first room first (r8-12 v r18+) is decided by the
  opening heading (field mode walks the worker away from spawn along its stripe), not by the room term.
