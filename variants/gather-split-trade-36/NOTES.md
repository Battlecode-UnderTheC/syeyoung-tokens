# gather-split-trade-36

## Idea

An early "outnumbered" mode against swarms (server battle M491281): when the distinct enemy
heads seen in the last 30 rounds are at least 4 and more than 1.5x our unit count (or, on a
small map, an enemy was seen by r25 and they outnumber us), before r200: workers stop even
trades (only strictly longer enemies are worth a kamikaze) and every dragon treats a step
within 2 steps of an enemy head it won't trade with as exposed (SafeStep's `exposed` flag
only; room counting is unchanged). Off otherwise. Plus an opening centre race on small maps
(<= 800 tiles, r < 30): known tiles score `0.5 / (dist + 1 + torus distance to the grid centre)`,
which replaces the undirected explore pull with a pull to the (symmetric, contested) centre.

## What changed from the parents, and why

gather-split-trade-34 plus: Params `om_*` and `centre_*`, `UpdateOutnumbered()` each turn,
`ExposedNear()` in SafeStep, a no-even-trade line in `WorthTrading`, the centre bonus in
`ChooseGoal`. Namespace gst34 -> gst36. Big maps are untouched (identical results there).

## Measurements (sim, 2026-09-28)

- official vs gather-split-trade-34: 12-0-10 (heavily side-dependent mirror pairs).
- official vs trading-swarm-1: 18-1-3 (parent 18-0-4). The draw is arena as A, both sides dead at r9.
- 20 worldgen maps vs gather-split-trade-34: 20-0-20; vs trading-swarm-1: 39-1-0 (parent 39-0-1).
- Small maps vs trading-swarm-1, us as A (our dragons / theirs):
  - trophy: r50 10/11 (parent 5/11), r100 26/13 (parent 19/14), end r499 WIN longest 35-4 (parent WIN 37-5)
  - devil: r50 11/17 (parent 5/15), r100 12/15 (parent 10/14), eliminated r439 (parent r396)
  - default_small: r50 6/2 (parent 3/6), WIN r68 (parent eliminated r246)
  - Colosseum: r50 13/2 (parent 16/1), WIN r65 (parent WIN r50)
  - arena: draw at r9, both wiped out (parent WIN r38)

## What should improve

Early dragon count on small crowded maps (roughly twice the parent's at r50 on trophy/devil).
Devil is still lost: the swarm keeps more dragons from r100 on, so the mode helps survival but not
the pearl race there.

## Not tried

No parameter sweep (short loop). Could try next: a longer om_until, a stronger centre
value, and applying the mode to leaders' trades too.
