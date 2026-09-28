<!-- Copied from syeyoung-rl dragons-env/game.md at 7a15f42. The engine in shared/engine/ implements exactly this. -->

# The dragons game

The rules as the official engine implements them: `engine/src` of
[unswcpmsoc/unswbc](https://github.com/unswcpmsoc/unswbc), shipped as
`unswbc_engine.wasm` in unswbc 1.0.0 (protocol 2.1.0). Function names in
brackets are the engine's; `cpp/dragons/` ports each of them.

## Board

A **torus** of `W x H` tiles, 7 to 256 a side (the contest maps are 11 to 64).
Tiles may be **pearl beds** with a respawn gap range `[min, max]`. Between
tiles are **edges**: empty, **kelp** (impassable, stops sonar, does not block
sight) or a **portal** paired with exactly one other edge of the same
orientation. Stepping across a portal lands next to its partner, still heading
the same way (`TileAfterStep`).

A map may declare `SYMMETRY x|y|xy`. The engine then requires each bed to match
its mirror, and a bed and its mirror share one countdown.

## Dragons

A dragon is a chain of segments, head first, on team A or B. Ids are handed out
in order: first the map's dragons, then each split child. A team may have at most
`UNIT_LIMIT` dragons alive (64 by default); the limit is only checked when
splitting. Dead dragons never come back.

## A round

Matches last at most 500 rounds, numbered 0 to 499 (`Game::Run`):

1. **Pearl tick** (`PearlTick`): every bed's countdown drops by one. A bed at 0 or
   below gets a pearl, unless it already has one or a living dragon lies on it,
   and draws a new gap. Before round 0 every bed draws its first gap
   (`InitPearlCountdowns`). Gaps come from one `std::mt19937` seeded with
   `0x5eed5eed`, drawn in row-major bed order, so a map plays out identically
   for identical actions.
2. **Turns**: dragons act one at a time in id order, each seeing what the ones
   before it left. A dragon that was killed is skipped. A child born this round
   acts later in the same round.
3. **Result** (`ResultAfterRound`), only once the round is complete. If a team
   has no dragons left, the game ends: the other team wins, or it is a draw if
   both are gone. After round 499 the team with the longest single dragon wins;
   total length breaks a tie, and equal totals make a draw.

## A turn (`TakeTurn`)

The dragon receives its round block. Its sonar inbox is emptied, the reply is
applied, and then, if the dragon is still alive, its `SONAR` is cast.

* **MOVE d1 d2 ...**: one step per letter. Step k ≥ 2 first requires length > 2
  and otherwise kills the dragon (no valid action). Each step (`Step`) turns the
  dragon to face the step, then:
  * kelp kills it (wall);
  * its own body, tail included, kills it (self);
  * another dragon's head kills **both** dragons (head-to-head);
  * another dragon's body kills the mover (other body);
  * otherwise the head moves in. A pearl there is eaten, which grows the dragon
    (the tail stays); with no pearl the tail moves up. Steps after the first
    cost one tail segment each.
* **SPLIT n**: needs 2 ≤ n ≤ length − 2 and fewer than `UNIT_LIMIT` living
  dragons on the team; otherwise the dragon dies. The last n segments become a
  new dragon, reversed, so the parent's tail is the child's head, facing from
  its second segment towards its head.
* anything else (no MOVE or SPLIT line, or one that doesn't parse) kills the
  dragon (no valid action).
* **SONAR** (see [Sonar](#sonar)).

**Death** (`Kill`) leaves a pearl on every second segment starting from the
head (segments 0, 2, 4, ...), on any tile, bed or not.

## Sonar

Probed from the 1.0.0 wasm (the published source predates it):

* `SONAR <dir> <word>` pings in one direction, `<word>` plain decimal up to
  2^64 - 1 (anything else and the line is ignored). Up to four pings a turn,
  one per direction; a repeated direction keeps its last line.
* The bare `SONAR <word>` is the old form: read as C `strtoull` reads it (a
  sign allowed, negatives wrapping, overflow saturating), cut to its low 32
  bits, and sent along the facing *after* the action, unless that direction
  was named explicitly.
* After the action, if the dragon is alive, the pings are cast in N, E, S, W
  order. A ping opposite the facing starts at the **tail** and runs the way
  the tail points (from the second-to-last segment to the last); any other
  starts at the head. It goes through portals for up to `W + H` steps; kelp
  stops it; the first living dragon it reaches, which can be the sender,
  receives the word.
* A word wider than 32 bits is only delivered to a dragon that has declared
  `PROTOCOL 3` or higher; any other hit still counts as an echo.
* `PROTOCOL n` (the last valid one in a reply counts, and it sticks) with
  n >= 3 adds the `ECHOES kelp ally ally_head enemy enemy_head` line to every
  later round block: how many of the dragon's pings last turn stopped at kelp,
  an ally's (or its own) body or head, an enemy's body or head. A split child
  inherits its parent's protocol, so it sees ECHOES (zeros) on its first turn.

## What a dragon sees

At spawn (`BuildInitBlock`): `ID`, `TEAM`, `MAP W H`, `UNIT_LIMIT`.

Each turn (`BuildRoundBlock`):

```
ROUND r
DIR d                      the head's facing
LENGTH n
UNIT_COUNT k               living dragons on its team
NUM_MSGS m, then m lines   sonar words received since its last turn
ECHOES k a A e E           only for a dragon that declared PROTOCOL 3+
49 lines "x y pearl countdown"   the 7x7 window around the head, row-major from
                                 the top left; countdown -1 on tiles that are not beds
DRAGON_BODIES c, then c lines "team id x y facing isHead"
                                 every living segment in the window, by dragon id and
                                 then head to tail; a body segment faces towards the
                                 next segment nearer the head
8 rows of 7, then 7 rows of 8 edge tokens: '.', 'w' or a portal id
                                 (north sides, then south of the last row;
                                  west sides, then east of the last column)
```

A portal edge shows the smaller id of its pair. Tiles carry absolute
coordinates. The window ignores kelp, so a dragon can see through it.
