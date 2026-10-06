# ring-sprint-guard (g44-3) = arena ring-sprint-3

Base: ring-sprint-1. One change: a port of the lunge guard plus escape run (ring-lungeguard-1 in its
final form, arena ring-contest-10, commit 7d551e8), so long workers stay out of enemy stubs' lunge reach.

## Idea
Long workers on ring-sprint-1 still die to enemy heads lunging from several tiles away (arena-r18 T3,
D1 226). Sprints make it worse (g43-5: 8.7 v 7.1 lunged per match). ring-contest-10 cut 6+ head deaths
23% on the server suite with board control and trades unchanged.

## What changed from the parent, and why
- The exact ring-lungeguard-1 diff against ring-contest-m1 (strategy.cpp/.hpp, plugin.cpp):
  BuildLungeField, SafeStep's guard, LungeEscapeRun, and the plugin's keep of gst's step/run against
  the lookahead and the rollout judge. See variants/ring-lungeguard-1/NOTES.md.
- rush.cpp FreeSprint (new in ring-sprint-1): each extra sprint step also refuses tiles in the lunge
  field (`LungeExposed`), as the single step does. Part of the port, not a new idea.

## Local (before submit)
- Fresh mapgen: 87-89 (lunged 10+ 5.8 v 7.3 per match).
- Duels vs ring-sprint-1: server 27-41 and 36-32; official 9-11.
- `check ring-sprint-guard maps/official` passed.

## Arena: ring-sprint-3
- Verdict vs ring-sprint-1: 58-0-54 (51.8%, CI 42.6-60.8), beats parent within noise.
  By suite: server 38-30, official 5-5, custom (rotating) 15-19.
  Decided by: elimination 26-27, longer queen 23-18, longest dragon 9-9.
- Others: ring-contest-4 60-52, ring-sched-17 70-42, ring-rush-1 71-1-40.
- CPU: worst p99 24.6M, max 55.2M.
- Eval P(win) vs parent: r100 .494, r200 .496, r300 .499, r400 .506.

## Step 6: its own 112 games vs the parent, decoded (per game, new v parent)
Tools: arena-r18 decode.py plus two scripts (h6.py, threat.py) in the g44-3 scratchpad.
| suite | 6+ non-queen deaths to enemy heads | of them by stubs (<6) | eats per 6+ turn | Voronoi r100-300 | trades started |
|---|---|---|---|---|---|
| server (68) | 5.37 v 6.84 (-21%) | 4.12 v 5.59 | .136 v .140 | .477 v .476 | 33.7 v 32.2 |
| official (10) | 15.0 v 16.2 | 7.9 v 8.8 | .141 v .150 | .493 v .492 | 37.1 v 36.8 |
| custom (34) | 17.4 v 19.6 | 10.2 v 14.1 | .132 v .147 | .469 v .491 | 56.4 v 54.1 |
| all (112) | 9.88 v 11.54 (-14%) | 6.29 v 8.45 | .135 v .144 | .475 v .481 | 40.9 v 39.3 |

Threatened states (a 6+ worker, a shorter enemy head in view, and every free single step in its
seen-length lunge reach or next to it), all suites:
- an escape run ending clear was taken in 527 of 1881 (the parent: 12 of 2305);
- server only: 116 of 550 (the parent: 4 of 663);
- death to an enemy head within 2 rounds: 15.6% after a clear run, against 27% without one (the parent: 26%).

Side effects (not fixed here): eats per 6+ turn are down 6% overall (2.6% on server), and Voronoi on
the custom maps is down (.469 v .491). Deaths to enemy heads right after one of our multi-step moves
rose (server 0.66 v 0.31 per game), but most of those moves are new escape runs, which still die
less often than staying put.

Replays:
- 190091 (australia) r404: a 14-long worker with a 5-long enemy 3 tiles south takes WNWN out of reach.
  It lives to r457.
- 190014 (devil) r407: a 16-long worker between 4- and 6-long stubs runs SSWW and lives 33 more rounds.
- 189482 (weakhold) r412: a 9-long worker runs EEE away from an 8-long enemy.

The weak spot shows fixed in the arena: on the server suite deaths fall by about as much as
ring-contest-10's (-23%), with board control and trades unchanged.
