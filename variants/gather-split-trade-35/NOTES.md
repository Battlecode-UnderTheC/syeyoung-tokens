# gather-split-trade-35

## Idea

gather-split-trade-34 plus two narrow defences against what beat gather-split-trade-27 in server
battle M491281: the **reverse-split lunge** (an enemy head waiting near our tail kills the split
child, whose head is the old tail, before it moves: children act last in the round) and **kamikaze
pincers** on our last or longest dragon.

## What changed from the parents, and why

- **Split veto** (`TailThreatened`): no growth split, and no "doomed" split, while an enemy head seen
  in the last 2 rounds is within 3 steps (BFS over free tiles, portals included) of our tail. A
  trapped dragon with no free step whose tail is watched first tries an unknown portal, then splits
  anyway (the alternative is death). The veto applies only when the child would be at least 6 long
  (`split_veto_child`): the first version vetoed every split, cut growth splits by ~12% and lost
  13-27 to the parent on worldgen maps. A short child being lunged is an even trade anyway.
- **Guard** (`Guarded`, `DangerMap`): our last dragon (any length), or the team's champion when it is
  at least 10 long, ranks steps with "not in danger" right after "not doomed": danger = next to any
  enemy head in view, or within lunge reach (min(length-1, 8) free steps) of an enemy head shorter
  than us (any enemy, for the last dragon). The preferred step is taken only if not in danger. Doomed
  steps still lose to dangerous ones (a lunge is a maybe, a dead end is sure).
- Every reason of a dragon of length >= 10 now ends in " L10+" (tallies only), so long-dragon deaths
  show up in `sim`'s fatal turns and in `--trace`.
- Namespace gst34 -> gst35.

## Measurements (sim, both sides of every map)

| opponent | maps | gst35 | gst34 (parent) |
|---|---|---|---|
| gather-split-trade-34 | official (22) | 10-0-12 | - |
| gather-split-trade-34 | worldgen seed 1101 (40) | 19-0-21 | - |
| gather-split-trade-34 | worldgen seed 2202 (40) | 17-0-23 | - |
| gather-split-trade-34 | worldgen seed 3303 (40) | 20-0-20 | - |
| trading-swarm-1 | official | 18-0-4 | 18-0-4 |
| trading-swarm-1 | worldgen 1101 | 38-0-2 | 36-0-4 |
| trading-swarm-1 | worldgen 2202 | 36-0-4 | 35-0-5 |
| trading-swarm-1 | worldgen 3303 | 36-0-4 | 36-0-4 |

vs parent 66-76 overall (within noise of near-mirror play: the veto almost never fires, the guard
a few times per 40 matches, and the matches then diverge chaotically). vs trading-swarm-1 128-14
against the parent's 125-17.

Deaths of our >= 10-long dragons vs trading-swarm-1, official + worldgen 1101 + 2202, both sides
(`--trace`; "before" = this code with the veto and guard switched off):

- before: 929 no action (trapped: no move @cap), 130 feed champion (intended), head-to-head 17
  (trapped: trade 8, portal explore 4, kamikaze 2, detour 2, lunge 1)
- after: 903 trapped @cap, 117 feed, head-to-head 18 (trapped: trade 8, trapped: unknown portal 5,
  portal explore 2, detour 2, lunge 1)

trading-swarm-1 in simulation doesn't do the reverse-split ambush or pincers, so the sim can't show
the gain; it shows the change costs little. The real test is the server opponent.

## What should improve

Fewer long dragons lost to the M491281 opponent's lunges on fresh split children and to 2-long
pincers on our last dragon.

## Surprises / next

- The big long-dragon killer in simulation is **"trapped: no move L10+ @cap"**: at the unit cap a
  long dragon in a dead end can't split out and resigns whole. It concentrates on a few worldgen
  maps (pg_1106: 231 deaths of >= 10-long dragons over its two matches; pg_2215, pg_1110, pg_1108,
  pg_1103). Worth a look: keep slots free for long dragons, or stop entering dead ends near the cap.
