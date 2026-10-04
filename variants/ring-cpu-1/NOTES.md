# ring-cpu-1

## Idea
CPU fix only. ring-explore-2 (arena ring-plus-7) plays exactly as before, but its worst turn
drops from 95.2M judge points to about 34M. The cap is 100M.

## Where the CPU went
Every turn above ~30M was the queen (id 0/1) from about round 400 on. The queen's harvest
beam (QueenMove) has depth up to 6, width 8 and 4 directions, so up to 192 candidates a turn.
Each candidate ran:
- `Search(board_, t, ...)` to depth 12, allocating two board-sized arrays (4096 tiles on
  big_empty), with a `std::find` over the candidate body (length 70+) for every tile it
  entered;
- a loop over all board tiles for the food term, calling `TileValue`, whose `Occupied()`
  does a `std::find` over `body_`;
- `EscapeLevel`, which also allocated two board-sized arrays;
- from round 430, `QueenContinuation`: a beam of up to 64 states that copied the 70-tile
  body for every state.

Ring4Exit (one turn near r430: a 6-deep, 32-wide beam with `QueenContinuation` on every
candidate) was the second spike. Worker turns (rollout review, search7) stayed under ~25M.

Sandbox, base big_empty vs queen-3, seed 1592614637: queen r441 95.2M, r442 93.0M,
r440 92.8M, r473 90.9M. Workers peaked at 24.6M.

## What changed (decisions identical)
1. EscapeLevel uses epoch-stamped scratch arrays kept on the agent (`Scratch`) instead of
   allocating two board-sized vectors per call. Same marks, same BFS order.
2. In QueenMove's beam, the candidate body is a mark set. The depth-12 food search is an
   in-place BFS that skips the board-sized arrays (same `enter` test, same distances). The
   food max loops only over the tiles reached, not the whole board. `Occupied()` uses a
   mark set of `body_` built once per beam.
3. QueenContinuation keeps the same beam, order and pruning (16 per first direction), but a
   state is "base body + up to 4 new head tiles + the length". Membership is computed from
   that, so no body copies. Its vectors are agent members, so a call allocates nothing.
4. Safety net: `work_` counts the tiles expanded by these searches, plus a per-candidate
   cost (weighted ~1.6x in Ring4Exit, where copies and the sort cost more), and is reset
   each turn in Decide. Once `work_ > kQueenWorkCap` (100k), the QueenMove beam and the
   Ring4Exit beam stop expanding and keep the best route found so far. Depth 0 always runs
   in full. Calibration (sandbox, big_empty): about 540 points a unit on a ~10M fixed turn
   cost, so the cap sits near 65M. Normal games peak near 50k units, so the cap has not fired
   in any game measured. With the cap forced to 2000, games still ran and the queen survived
   (maze, australia).

## Play is unchanged
- Native, `match` A=ring-explore-2 vs B=ring-cpu-1 compared with A=B=ring-explore-2: every
  output field except turn_us is identical. The final build matched on all 32 maps (official
  + server) with seeds 3 and 1592614637 (64 of 64 games). Earlier builds with the same
  decision code matched with seeds 7 and 21 (96 more games).
- Sandbox: the same game, turn for turn, wherever the base stayed under the cap (maze/q12,
  australia/qg7, stronghold/crossover: same winner, same queens, same turn counts).
- One sandbox game diverged: big_empty vs queen-3. The cause is the base, not this
  variant. A sandboxed dragon that runs out of points before it writes anything is
  restarted with a fresh instance and the turn is retried (unswbc turn.py: `TurnBot.ask`
  retries with `_fresh()`). The retry loses all memory, and only the retry's points are
  reported. At r474 the base queen silently overran, was restarted with no memory, and
  moved differently ("ES"). ring-cpu-1 (33.6M that turn) plays the move the native engine
  plays ("SSSW"). So the arena's 95-99M maxima hide more overruns than the 2 it counts:
  each one is a queen with amnesia.

## CPU, sandbox (`unswbc run --sandbox`), points per turn
| game (seed) | base max | new max | base p99 | new p99 |
|---|---|---|---|---|
| official/big_empty vs queen-3 (1592614637) | 95.2M | 34.0M | 20.0M | 19.4M |
| server/maze vs queen-ring17-first-then-grow-12 (5281284940419892006) | 83.0M | 31.0M | 17.2M | 17.0M |
| server/australia vs queen-guard-7 (5823891923403082162) | 83.4M | 29.2M | 17.1M | 16.7M |
| official/stronghold vs crossover-1 (1592614637) | 75.7M | 29.1M | 17.6M | 17.3M |

(These four rows come from the build before item 3's allocation-free vectors; the final build
is a little cheaper.)

Sweep, final build, new (A) vs base (B) in the same game, seed 77, all 17 server maps and 15
official maps: new max 14.3M-31.4M (highest: big_empty 31.4M, unsw 27.6M), new p99
11.9M-19.4M. The base in the same games reached 67.0M (slithery_fight), 64.1M (maze), 63.8M
(australia), 61.3M (trauma), 51.7M (stronghold) and 51.1M (autarky).

## Local sim
`sim --a ring-cpu-1 --b ring-explore-2 --maps maps/server --seed 21,22,23,24 --threads 2`:
68-0-68 over 136 games. Every reported statistic is identical (decided by, r50/r100 economy,
queens, deaths, action counts), so these are mirror games.

## Porting to ring-explore-1 (ring-plus-4, live)
The same four edits apply as they are. EscapeLevel, QueenMove's beam, QueenContinuation and
Ring4Exit match between the two lines, and the explore change is in ChooseGoal only.
