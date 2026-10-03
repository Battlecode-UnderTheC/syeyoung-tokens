# ring-forage-1

## Idea
On small 2-starter maps ring17's queen parks from r0 and the team falls far behind
(6 v 25 dragons at r100 in server losses; queen eats 0.3 pearls). A short queen (<=3 long)
on a small start (<=2 dragons) should forage and split first, then settle into the ring.

## What changed from the parents, and why
Parent: queen-ring17 (arena queen-ring17-first-then-grow-1). One change, in QueenMove
(`FORAGE35_MODE`, strategy.cpp/.hpp):
- Gate at r0: units <= 2 and queen length <= 3 (observed, never the map) -> `queen_forage_`.
- While foraging: the EARLY15 mode-6 harvest split (at length 4, child L-2) fires for any
  team size (was units<=1 only), but not on a corridor tile; the home tether is dropped;
  routes through a corridor tile (kelp on 2+ sides) cost 20; endpoints with under 20 tiles
  of open room (unseen edge counts as open) cost 2 per missing tile.
- Settle into the ring at 6 dragons or r120 (split off a worker first if safe).
Queens that start 4 long (maze/trauma/weakhold) or teams with 3+ starters are untouched.

## Local (vs queen-ring17, both seats)
- maps/server seeds 1-4: 39-41 (only trophy and queen_of_spades are affected; the rest are
  the same games). trophy seeds 1-8: 7-9; queen_of_spades seeds 1-8: 13-3
  (dragons at r100 10.5 v 3.4).
- maps/tournament-sprint seeds 1-4: 33-23. stripes 8-0, tower_defense 5-3, all others 4-4
  (unchanged code path, including maze and weakhold).
- maps/official: default_small 5-3, arena 5-3, trophy 5-3 (26.1 v 17.6 dragons at r100),
  queen_of_spades 8-0.
- check maps/official: all passed.

## Arena verdict (ring-plus-1)
- vs parent queen-ring17-first-then-grow-1: 40-0-38 (51.3%, CI 40-62), beats_parent=True but
  within noise; server maps 35-33. Decided by longer queen 19-19, elimination 16-14, longest 5-5.
- vs queen-ring14-portal-safe-1 41-37, queen-guard-7 53-25, codex-queen-value-1 46-32.
- Head-to-head vs parent on the gated maps: stripes 3-1, queen_of_spades 3-1, trophy 2-2,
  tower_defense 1-3, default_small 1-1, arena 1-1; ungated maze 2-2, trauma 2-2, weakhold 2-2.
- All opponents, per map (parent's own all-time record in brackets): stripes 7-9 (36.5%),
  queen_of_spades 11-5 (50.9%), trophy 8-8 (51.9%), tower_defense 6-10 (62.5%),
  default_small 2-6 (68.5%), maze 10-6 (72.1%). Small samples; no clear opening gain.
- Metrics: dragons at r100 22.6 v 22.5 (whole pool); queen alive at end 0.64 v 0.44.
