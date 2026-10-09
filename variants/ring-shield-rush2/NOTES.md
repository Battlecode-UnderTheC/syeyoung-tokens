# ring-shield-rush2 (g53-19)

Base: ring-shield-rush (arena ring-shield-10). The parent's notes: variants/ring-shield-rush/NOTES.md.

## Idea
ring-shield-10 wins the opening race on open-centre maps but loses where the middle is walled (default 3-13,
fixed-centre-trap 1-7, tower_defense 5-11): its rushers circle a middle reachable only through portals or a
maze to r13-27 without eating. Give up the race on what the rusher sees about the way in.

## Change (rush.cpp WayIn + one test in UpdateCentre)
- WayIn: BFS from the head to the rush target over every tile not known to be shut off. Edges never seen
  count as open (the frontier's own optimism), seen kelp as shut, a portal only when both ends are known
  (from a seen tile, Step's kUnknown is an untraced portal, since every side of a tile in view is seen).
  It is a lower bound on the walk to the middle.
- When there is no such way (a portal-only or sealed middle), or round + WayIn - kArrive > kRushUntil (r30,
  the race's own end), the race is lost by its own terms: hold if within kCentreRadius and a cluster is seen
  (as the stall rule), else explore the frontier (the head's explorer). No new constants.
- The parent's dropped "r30 bound" used frontier + straight line (ignores known walls, so it misread devil's
  bed-column walls); WayIn counts the known walls and stays <= r30 on devil seeds 1-3 in both seats.

## Traces (match v ring-shield-rush, seed 1; CENTRE_DBG prints CWAY when it fires)
- default: A id4 gives up r18 (way 16, straight 8) v r23; B id3 r17 (way 18) v r25.
- fixed-centre-trap: A id4 r13 (way 20, straight 10) v r19; B id5 already gave up r11 (stall).
- tower_defense: A id2 r3 (way 31) v r6.
- Never fires: devil s1-3, trophy, dilemma, stripes, islands, fixed-edges, fixed-pen, fixed-off-axis,
  queen_of_spades (their rushers arrive or stall as before; those maps' losses are not the way in).

## Local (v ring-shield-rush)
- 12 maps (server losers/winners + 4 arena fixed-*), seeds 1-2: 25-23. r50 total 21.9 v 21.6, r100 44.1 v 41.6.
- fresh mapgen seed 619019 (44 maps x seeds 1,2 x seats): 90-86; per family 8-8 everywhere except rooms 9-7,
  sealed-spawn 9-7. r50 43.3 v 43.1. check maps/official passes.

## Arena
- `tokenz ephemeral` timed out twice (whoami works); handed back for the orchestrator to submit.
