# swarm-hunt-24

## Idea

Crossover: port body-wall-15's two portal/endgame fixes into swarm-hunt-19's HuntAgent
(namespace `swh19` -> `swh24`, macros `SWH19_*` -> `SWH24_*`). The shared movement layer
(all our lines descend from gather-split-trade-38/39) meant the port was near-verbatim.

## What changed from the parents, and why

Parent swarm-hunt-19, near-verbatim ChooseGoal/Decide/Turn edits from body-wall-15:

1. **Champion self-coil**: `champ_no_trap = 1` (new param). In `ChooseGoal`, `no_trap` (which
   already skipped dead-end candidates near the unit cap) now also triggers once we're the
   frozen champion (`round_ >= p_.champ_freeze && Champion().id == spawn_.id`), same as the
   cap-safety rule. Before this, a frozen champion could still pick a dead-end candidate
   ("into a dead end worth it"), coil inside its own body, and trap-split into a ghost
   champion for many rounds.
2. **Sealed-exit drops**: `cross_min_length = 4` (new param), gating all three unknown-portal
   crossing sites the same way body-wall-15 gates them:
   - the explore value an unknown portal gives a candidate tile (ChooseGoal): now needs
     `WillCross() && length_ >= cross_min_length`, not just `WillCross()`.
   - the pre-crossing sonar ping (Turn, the "step next to an unknown portal: ping through it"
     block): only pings when `length_ >= cross_min_length`.
   - the actual crossing step (Decide's "portal: explore" block): needs
     `length_ >= max(2, cross_min_length)` and, since `cross_min_length` here is 4 (our
     split_min_length), `CanSplitNow()`, so a dragon landing in a 1-tile box past an unknown
     portal can split out (the child walks out the entry side).
   Left out the `never_cross` sonar mark: body-wall-15's own notes say it did almost nothing
   measurably (the length gate is what saves the dragons), so it wasn't worth the extra sonar
   channel and code.

## What I measured

vs swarm-hunt-19, both seats:

| maps | W-D-L |
|---|---|
| official (11 maps, both seats = 22) | 12-0-10 |
| worldgen batch, 20 maps, both seats = 40 | 21-0-19 |

`check swarm-hunt-24 maps/official`: all checks passed, 0 avoidable deaths, slowest turn
0.1-33.0 ms depending on map (matches swarm-hunt-19's own range on the same maps and hardware;
not a regression from this change -- these native-build numbers aren't the wasm judge number).

Action-reason counts on official maps (swarm-hunt-24 vs swarm-hunt-19 as B): "portal: explore"
crossings 1 vs 19, "into a dead end worth it" 103 vs 163 (both sides of one match, not directly
comparable per-dragon, but the direction matches body-wall-15's own counters: crossings and
dead-end walks both drop sharply).

Both W-D-L are at least even (slightly positive) with the parent, per the submit bar.

## What should improve

Fewer dragons lost stepping through unknown portals into sealed 1-tile boxes, and a champion
that stops cutting itself down late with dead-end walks late-game. The margin on worldgen
(21-19) is smaller than body-wall-15 saw against body-wall-14 (53-27); swarm-hunt-19 may take
fewer portal risks to begin with (portal_explore/portal_cross tuning differs from body-wall),
so there was less room for this fix to recover.

## Didn't work / next

- Didn't try the never-cross sonar mark (skipped per the brief -- body-wall-15 found it did
  nothing measurable).
- Didn't sweep champ_freeze/cross_min_length for swarm-hunt-19's own portal-usage rate; next
  task could check whether swarm-hunt-19's lower baseline crossing count makes the length gate
  less valuable here than in body-wall, or whether a smaller cross_min_length (e.g. 3) recovers
  more useful exploration while still avoiding 1-tile boxes.
