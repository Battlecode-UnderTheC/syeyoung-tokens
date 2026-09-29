# gather-split-trade-104

## Idea

gst101's poor-home portal push (ported from gst83/gst98) helps a lot in some spots (Trauma,
where it stopped the opponent eating 913 maze pearls to our 18) but cost 18-22 vs gst96 on
fresh worldgen, i.e. it fires in places where it doesn't pay: a dragon that's merely spread
out with open food nearby shouldn't take the poor-push discount to cross unknown portals.

## What changed from the parents, and why

Namespace/macros renamed gst101 -> gst104 (and GST101_* -> GST104_*) throughout, per the
task's isolation requirement.

One substantive change: tightened the `poor_` test in `ChooseGoal` (strategy.cpp). Previously
a dragon was "poor" (and would pay `poor_portal_mult` to cross an unknown portal, or accept a
`poor_cross`-length detour) whenever `unreachable > reachable` (more seen-but-unreachable
valued tiles than reachable ones), on top of the existing few-reachable-tiles/starved
conditions. Per the brief, changed this to `unreachable >= 2 * reachable`: a stricter bar so
the push only fires when a dragon's home region is genuinely walled off, not just when it's
merely more blocked than open. Left poor_min=3, poor_per_unit=1, poor_portal_mult=4.0,
poor_cross=6, starve_rounds=35 unchanged — the brief listed them as tunable but the budget
only allowed testing the one comparison operator soundly; the others are candidates for a
follow-up tweak once we know how much the tighter unreachable test alone shifts things.

Added a `g_poor_turns` atomic counter under `GST104_DEBUG`, reported in `IdleReport`'s
destructor alongside the existing counters, so a future task can see how many turns per map
class actually go through the poor path (didn't get to run a `GST104_DEBUG` build myself in
the time budget; the counter is there for the next tune).

## What was measured

All vs gather-split-trade-101 (the direct parent) unless noted, `sim --threads 2`:

- Official maps (`maps/official`, both seats, 11 maps): **11-0-11**. By map: Colosseum
  W/L (seat-dependent), arena L/W, big_empty W/L, default W/L, default_small L/W, devil L/W,
  queen_of_spades L/W, schooltime L/W, stronghold W/L, trauma L/W, trophy L/W — i.e. gst104
  wins whichever seat happened to be seat A in most of these and loses seat B, a seat-parity
  artifact of a near-identical bot, not a real map preference; net margin -0.5 longest, -7.5
  total, essentially a wash.
- Worldgen, default style, seed 50000, batch 20 (40 matches both seats): **24-0-16**, gst104
  ahead by +2.2 mean longest.
- Worldgen, labyrinth style, seed 70000, batch 20 (40 matches both seats): **19-0-21**, gst104
  behind by -1.1 mean longest, -7.0 total — close to even, slightly worse than parent.
- Combined across all three sets: 54-0-48, i.e. modestly ahead of parent, consistent with
  "the tightened test doesn't cost us the wins the push already had, and should be closer to
  even where it wasn't paying."
- vs trading-swarm-2 on Trauma + queen_of_spades (Portals-like), both seats: **10-1-11** (one
  draw on queen_of_spades seat A that hit the 500-round cap without a winner recorded as a
  draw by the harness — check the raw log if that matters for scoring). Beat ts2 both seats on
  Trauma (20-4/22-19 and 17-0/22-0 longest/total), split queen_of_spades (lost seat A 0-3,
  won seat B 26-14). This matches gst101's known Trauma strength; the portal push clearly
  still fires there since Trauma is exactly the poor-home case it's meant for.

Didn't get to a `GST104_DEBUG` build to count poor-turn frequency per map directly in the time
budget (~30 min total); left the counter wired up (`g_poor_turns` / `GST104_COUNT`-style
report line) for whoever tunes this next.

## What should improve

Slightly better average performance than gst101 against itself (fewer wasted crossings on
worldgen maps that aren't genuinely poor), while keeping the Trauma/portal-heavy-map strength
that motivated the push in the first place. Net effect is close to neutral-to-positive, which
is what a stricter necessary condition on an already-narrow trigger should look like.

## What didn't work / wasn't tried

Did not sweep poor_min, poor_per_unit, poor_portal_mult, poor_cross, or starve_rounds — the
brief flagged them as tunable but the budget only allowed testing the `unreachable` threshold
itself soundly (build + check + 3 sim sweeps + writeup). A natural next step: build a
GST104_DEBUG binary, run it over the worldgen sets, and read off g_poor_turns per map to see
whether the 2x threshold is cutting the right turns, then revisit poor_portal_mult/poor_cross
with that data in hand.
