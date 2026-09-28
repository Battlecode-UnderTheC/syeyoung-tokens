# body-wall-1

## Idea

Use our bodies as walls against enemy heads, on top of gather-split-trade-23's economy
(copied whole, namespace `bw1`). Before splitting or gathering, a dragon with an enemy head
fully in view checks each of its safe steps (not exposed, own timed escape flood ok):

- **seal**: a step after which the enemy's timed flood finds no way out (our body and its
  own body free tail-first as they move; kelp and other bodies stay; leaving the window
  counts as a way out) is taken when it seals more enemy length than our other safe steps.
- **squeeze**: otherwise (length >= 3, before the feed round) the step that most shrinks
  the tiles the enemy heads in view can reach (static bodies, counted in view, up to 12) is
  taken when it takes at least 3 tiles more from them than our worst safe step.

The champion (from the feed round) and leaders from round 300 don't wall.

## What changed from the parents, and why

Fresh variant; the economy is gst23 unchanged. Added `EnemyReach` (timed / static enemy
flood), `Wall` (step 2c in `Decide`, after kamikaze/lunge), and a `waller` role (off:
`waller_mod = 0`): split children with id % 4 == 0 grow to 10 before splitting and walk
towards enemy heads seen in the last 5 rounds.

## Measured

| version | opponent | maps | W-D-L |
|---|---|---|---|
| seal only | gst23 | official | 10-0-12 (seal fired 3 times in 22 matches) |
| + squeeze (len >= 5) + wallers (1 in 4) | gst23 | official | 10-0-12 |
| same | gst23 | 20 worldgen, both sides | 17-0-23 (total -47: fewer splits; our trapped deaths 30 -> 50 on official) |
| **final**: squeeze len >= 3, no wallers | gst23 | official | 11-0-11 (squeeze ~15/match) |
| final | gst23 | 20 worldgen | 22-0-18 (squeeze ~55/match, longest +1.4, total +15) |
| final | league quick | anchor-random / anchor-greedy / gst23 | 42-0-0 / 41-0-1 / 19-0-23 |

About even with gst23 overall (52-0-52 summed).

## What we learned

- **A true seal is rare**: in ~11k evaluations with enemy heads in view, only 3 steps sealed
  an enemy that no other step did. Short enemies (2-3) escape any pocket of 3+ tiles by
  following their own tail, and pockets must be fully inside our 7x7 window to be judged.
  Hundreds of times the enemy was already sealed whichever way we went (natural traps).
- **The economy keeps bodies short**: with split at 4, almost no dragon is 5+ long before
  round 300 except the leaders, so "long bodies" means giving up splits. Wallers (1 in 4
  children growing to 10) cost the economy (worldgen 17-23, total -47) and their bodies
  trapped our own workers (our trapped deaths rose ~60%): walls block allies too.
- A static room flood that treats leaving the window as escape never measures anything
  (the window is too small); count the room in view instead.

## Next

- Wall only where it can't hurt allies (count allied room lost as a cost in the squeeze).
- Wallers only at the unit cap (no splits possible anyway, so no economy cost).
- Seal with the lunge: a multi-step MOVE lays the head across a corridor exit in one turn.
