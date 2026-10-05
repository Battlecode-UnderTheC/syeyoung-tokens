# ring-portal-1 (g43-9, line ring-contest)

Base: ring-contest-m1 (= arena ring-contest-1 / ring-contest-4; its notes are in
variants/ring-contest-m1/NOTES.md). Operator: mutate. One idea: workers use portals like
ordinary steps.

## Why
server-r17: in r0-150 of losses the opponents took 80 portal steps a game to our 17 (Trophy
1.4 v 25, Default 8 v 81, Islands 27 v 147, Around UNSW 8.5 v 155) and filled portal-gated
and walled rooms while we stayed on the rims.

## What stopped our dragons (measured with a PORTAL_DBG build, portal maps, seeds 1,2)
- Pathing was already fine: BoardMemory::Step resolves a portal once both ends were seen, so
  BFS, frontier and arrival times already go through KNOWN portals.
- **Known portals:** SafeStep counted a step whose landing is out of view as "blind" (libs 1),
  so 44% of worker goal steps through a known portal were refused ("detour") until the dragon
  came back, pinged and stepped again.
- **Unknown portals** (far end never seen; the common case, since each dragon has its own
  memory): the frontier scan pulled workers to the tile beside one, but at d=0 that tile stops
  being a candidate, so they crossed only when their goal elsewhere was worthless; otherwise
  they walked off to food.

## Change (PORTAL_TRUST, workers only; the queen keeps the old caution)
1. SafeStep: a known portal's out-of-view landing is trusted ("portal: step") unless a ping
   through that very portal hit a dragon within kLandingMemory = 5 rounds, or a dragon was seen
   on the landing within 5 rounds (5: a short worker's body clears a tile in about its length).
2. A worker stepping next to a known portal whose landing is out of view pings through it, so a
   clean echo lets it cross at once ("portal: pinged clear"). The team's other sonar words are
   kept. A first build cleared them, as the unknown-portal ping does. Its server duel went 28-40
   with our queen alive 50% v 63%, so the words now stay. Extra pings only make a clean echo
   rarer.
3. ChooseGoal: an unknown portal beside the head is scored as unseen ground one step away
   (explore_value x hunger multiplier, x poor_portal_mult when poor). When that beats the chosen
   goal, the existing "portal: explore" crossing fires (still after a clean ping, body in view,
   no enemy head in view).

No map keys and no positions: only the dragon's own portal memory and pings.

Debug counters: build with -DPORTAL_DBG. At exit it prints PORTALDBG: r0-150 head jumps, turns
beside known/unknown portals, crossings, and goal steps through known portals with their
refusals.

## Local numbers (sim vs ring-contest-m1)
- Portal maps (trophy, default, islands, unsw, devil, portals, schooltime), seeds 1,2:
  - r0-150 head jumps: 600 in base self-play, 837 now. Known-portal crossings 278 -> 392 and
    unknown-portal crossings 640 -> 823 over 28 games (base counts are 2x the seed-1 run).
  - r100: 31.8 dragons / 81.7 total v 28.4 / 73.2. W-L 13-15.
  - Deaths per use (per match): "portal: explore" 1.6 of 61, "pinged clear" 1.2 of 22,
    "portal: step" 0.5 of 4. Before the dirty-ping memory, "portal: step" killed 1.8 of 8: a
    ping had hit a dragon, and two turns later the step was trusted with no fresh information.
- Server maps seeds 3,4: 34-34 (r100 dragons 24.6 v 24.4).
- Fresh mapgen (seed 4309431, 44 maps, seeds 1,2): 78-8-90 (r100 dragons 27.6 v 28.1).
  Per family (W-D-L): anti-centre 8-0-8, crowded 8-0-8, maze 6-4-6, mixed 7-0-9, off-axis
  9-0-7, open 7-0-9, rich-near-home 8-0-8, ring 8-0-8, rooms 1-4-11, sealed-spawn 9-0-7,
  sparse 7-0-9.
  - Rooms re-checked: the same 4 maps on seeds 3-6 went 12-6-14, and 8 fresh rooms maps
    (mapgen seed 8812001, sim seeds 1,2) went 17-0-15. Rooms total 30-10-40: weaker, but not a
    clear loss; the 1-4-11 was queen deaths on two maps (our queen died in 12 of 12 decisive
    games). Ablations on the seed 1,2 rooms set: parts 1-2 off 4-4-8, part 3 off 6-4-6.
  - Seen in a rooms trace (rooms-s4309431-014): two of our workers crossed the same unknown
    portal from both sides in the same round and died head to head (twice in one game). A clean
    ping only proves the landing was empty when we pinged; dragons that move after us can
    still arrive.
- check maps/official: all checks passed.

## Arena duels (vs ring-contest-1)
- First build (the pre-ping cleared the team's sonar words), eph-aebd93e9: server random:2
  duel-adf9c54b 28-40 (queen alive at the end 50% v 63%, longer queen 12-21); official 10-10.
- Final build, eph-008a8e15:
  - server random:2, duel-c296c907: **38-30** (elim 11-12, queen 18-14, longest 8-3; 3+
    starters 25-15, <=2 13-15). Dragons@100 25.2 v 23.8, board control@200 .495 v .457, queen
    alive at the end 57% v 56%.
  - official random:2, duel-795b69e0: 10-10.
- Decoded duel replays (portal maps, 4 games each; per game, us v them):

  | map | portal steps r0-150 | pearls eaten r0-150 | deaths on a crossing turn (whole game) |
  |---|---|---|---|
  | default | 51.2 v 8.0 | 98.8 v 70.2 | 1.00 v 0.75 |
  | islands | 47.8 v 26.8 | 347.8 v 259.8 | 6.25 v 7.75 |
  | schooltime | 41.0 v 20.0 | 331.5 v 296.8 | 14.0 v 9.5 |
  | portals | 92.2 v 89.8 | 225.2 v 243.8 | 0 v 0 |
  | trophy | 3.2 v 3.8 | 163.8 v 166.0 | 1.00 v 1.75 |

  Trophy is unchanged. Its one pair's ends are 16 rows apart, so almost no worker has seen both
  ends (the portal stays unknown), and the cup is crowded: dirty pings and enemy heads in view
  block the crossing.

## Arena verdict
Submitted from /tmp/g43-9/botS (agent g43-9, --bot ring-contest --parent ring-contest-4).
The submit ran in the background and exited 0, but its output (the arena id and verdict) was
not read: the permission classifier blocked reading it. The orchestrator checks the verdict.
The committed code also has the PT_KNOWN / PT_UNKNOWN ablation switches (both default 1, so
behaviour is identical to the submitted bot).
