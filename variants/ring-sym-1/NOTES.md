# ring-sym-1

Task g39-sym. Parent: ring-sched-1. Arena line: ring-sched (user's change; not ring-sym).

## Idea

Every contest map declares SYMMETRY x, y or xy. The engine requires beds to match their mirror,
and a bed and its mirror share one countdown. Checked offline on all 22 maps in maps/server and
maps/official: the declared mirror also holds exactly for every kelp/portal edge. Bots are not
told the symmetry, so we infer it and use the mirror to roughly double board knowledge.

## What changed from the parent, and why

`sym.hpp` (new), hooked into `StrategyAgent::Remember` after the sonar inbox:
- **Inference.** Each dragon keeps three candidates (x: y flips, y: x flips, xy: both). For each
  first sighting of an edge, and each tile in view every turn, it compares against the mirror
  under every live candidate. A mismatch in bed-ness, edge kind, or countdown kills the
  candidate. Countdowns are compared when both readings are in the same round, or when the older
  one predicts no tick in between.
- **Commit rule.** A candidate is used only when it is the *last* live one, with 12 or more
  informative agreeing pairs (kelp/portal edges, beds). The first version used any confirmed
  candidate and was wrong on devil, schooltime, weakhold, autarky and trauma. Wrong mirrors agree
  with many pairs before a contradiction, and fixed-gap (1-1) beds all read the same countdown.
- **Sharing.** Report words (0xA7) carry the usable candidate in bits 1-3, which were always zero
  before; the decoder now ignores those bits. A dragon adopts a teammate's single candidate while
  its own observations still allow it, so split children get it at birth.
- **Use.** Mirrored empty/kelp edges are written into `BoardMemory` where never seen; portals
  are not, since their pairing would need invented ids. This covers pathing, RoomAround
  pocket checks and the frontier. Mirrored beds get their shared countdown, and plain tiles are
  filled once. Inferred tiles are stamped one round older than their source, so `Current()` is
  never true for them. A bed seen for real keeps its memory, unless that memory is stale and
  holds no pearl; then the fresher mirrored timer replaces it. So beds due soon on the far side
  enter ring-sched's food matching.
- **Access.** `BoardMemory` is in the shared layer with private arrays. `sym.hpp` reaches them
  through the explicit-instantiation access idiom, so the shared layer is unchanged.
- Enemy-home (c) was not used.

CPU: the full mirror pass runs once per candidate, w*h*3. Per turn the work is about 160 window
items per check. `check maps/official`: slowest turn 12.2 ms (trauma), all checks passed,
including the memory-step audit, which confirms the inferred edges are true.

## Local numbers

Inference instrument (SYM_TRACE build, not committed), 22 maps x seeds 11,22 x 2 seats vs
ring-sched-1:
- Final rule: 0 wrong commits.
- It found the declared symmetry on all 22 maps. Arena and big_empty resolve to xy, since the
  countdowns kill x and y.
- First own commit per map: r0-r63. Most dragons get it by sonar.
- Local W-L: 37-51 overall, 32-36 on server maps (noise level).
- r50/r100 dragons: unchanged within noise per map.

## Arena

Light duels of eph-42e766c0 vs ring-sched-1:

| suite | W-L | score | 3+ starters | ≤2 starters |
|---|---|---|---|---|
| server, random:2 | 36-32 | 52.9% | 22-18 | 14-14 |
| official, random:2 | 9-11 | 45.0% | 0-4 | 9-7 |

- Server, decided by: elimination 13-11, queen 14-17, longest dragon 9-4. dragons@100 23.0 v
  23.0; queen alive at end 0.68 v 0.63; queen length at end 9.5 v 8.4; CPU max 27.8M.
- Official: dragons@100 31.3 v 32.9; queen alive at end 0.60 v 0.40; CPU max 33.4M.

Submitted as `--bot ring-sched --parent ring-sched-1`. Verdict: see the arena.
