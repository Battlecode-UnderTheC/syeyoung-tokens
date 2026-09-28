# gather-split-trade-78

## Idea

When a dragon's home region is poor, it should cross unknown portals early instead of pacing an
empty strip. That is replay pattern #17/#8 on trauma: our spawn strip has few beds, and a
maze reachable only through portals holds the pearls. When the region is poor, an unknown
portal's explore value goes up, and a dragon standing beside a portal whose exit pinged clean
crosses it even when its goal is a far valued tile.

## What changed from the parents, and why

Base: gather-split-trade-68 (namespace/macros renamed gst68 -> gst78). All changes are in
`ChooseGoal` and the portal-crossing check in `Decide`:

- `poor_`: set each turn in ChooseGoal. It is true when (a) the number of reachable
  pearl/bed candidates is below `max(poor_min=3, poor_per_unit=1 x units)`, or (b) nothing
  has been eaten for `starve_rounds=35` rounds (only before `cross_until`).
- When poor, the explore score of a tile beside an unknown portal is multiplied by
  `poor_portal_mult=4` (0.25 -> 1.0, so it is worth as much as a pearl at the same distance).
- When poor, the "portal: explore" crossing (which still needs last turn's clean ping, the
  body in view, no trap goal, and no enemy head in view) also fires if the current goal is
  at least `poor_cross=6` steps away. Before, it fired only when the goal had zero value.
  The known-portal safety from gst42/46 (ping first, exit seen empty) is unchanged.
- Debug-only counters (GST78_DEBUG) for pearls eaten, explore crossings, portal steps and
  poor turns in rounds 0-149.

## What I measured

Debug counts, r0-149, vs trading-swarm-5, both seats summed. "Parent-equivalent" means the
poor logic switched off (that config gives an exact 11-0-11 mirror vs gst68 on official):

| map | eaten (parent-eq -> gst78) | explore crossings | portal steps |
|---|---|---|---|
| trauma | 87 -> 133 | 16 -> 16 | 26 -> 35 |
| default | 145 -> 126 | 14 -> 11 | 42 -> 33 |
| stronghold | 124 -> 163 | 0 -> 16 | 0 -> 25 |
| queen_of_spades | 95 -> ~93 | 11 -> ~15 | 27 -> ~29 |

(The queen_of_spades values are from an earlier config with similar thresholds.)

W-D-L, both seats (final defaults):

| opponent | official 11 | worldgen seed 101 (20) | worldgen seed 202 (20) |
|---|---|---|---|
| gather-split-trade-68 | 10-0-12 (longest -0.4, total +21.1) | 20-0-20 (-1.2, +15.8) | 21-0-19 (+2.6, -5.3) |
| trading-swarm-5 | 12-1-9 | 32-0-8 | 31-0-9 |
| (gst68 vs trading-swarm-5) | 14-1-7 | 29-0-11 | 32-0-8 |

That totals 51-0-51 against the parent. Against trading-swarm-5 it is 75-1-26, the same as
gst68's 75-1-26. On official vs gst68 it now **wins trauma in both seats** (the parent won it
in both seats before). It loses a little on default and stronghold.

## What I tried that didn't work

- Forced crossing whenever poor (poor_cross=1, i.e. any goal distance), with per_unit=2, min=4:
  8-14 official, 15-25 / 20-20 worldgen vs gst68. It is too eager: dragons leave good
  goals for any clean portal.
- Boost only, no forced crossing (poor_cross=0): roughly even or slightly better overall
  (10-12, 20-20, 25-15), but on trauma early eating *dropped* (87 -> 60). Dragons walk to the
  portal, ping it, then turn back to a far valued goal, so the boost alone just adds pacing.
  Crossing has to be forced.
- poor_portal_mult=2: about the same as 4.
- Without the starvation trigger: no better.

## What should improve / next

Trauma-like maps (a poor spawn strip next to a portal-only maze) should improve. Large open
maps are about unchanged. Next: the "poor" flag still fires a lot on stronghold/default (hundreds of
turns in r0-149). A sharper test, such as "seen but unreachable beds outnumber reachable ones",
or a per-dragon (not per-team) food count, might keep the trauma gain and drop the small
default/stronghold losses.
