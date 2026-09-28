# body-wall-15

## Idea

Two endgame/portal fixes from the league-2 replay study (VISUAL.md #20 and #19) on top of
body-wall-14: a frozen champion never walks into a dead end, and short dragons (or ones that
can't split now) no longer step through unknown portals.

## What changed from the parents, and why

Parent body-wall-14 (namespace `bw14` -> `bw15`, macros `BW14_*` -> `BW15_*`).

1. **Champion self-coil (#20)** `champ_no_trap = 1`: in `ChooseGoal`, from `champ_freeze`
   (r330) a dragon that believes itself the champion skips dead-end (trap) candidates, like
   the cap-safety rule does. Before, 5-15% of champion turns were "into a dead end worth it";
   it coiled inside its own body, trap-split and left a ghost champion for 10-20 rounds.
2. **Sealed-exit drops (#19)** `cross_min_length = 4`: the unknown-portal explore crossing (and
   the ping that precedes it, and the explore value an unknown portal gives) needs length >= 4
   and `CanSplitNow()`, so a crosser landing in a 1-tile box can split out (the child walks
   out the entry side). Most of our dragons are 2-3 long (split_min_length 4), so this nearly
   switches the explore crossing off.
3. `never_cross = 1`: a trapped/doomed split within 2 rounds of a crossing marks that portal
   (entry tile, direction) never-cross, and the dragon pings a `kNever` sonar word (0x3D) for
   2 rounds so the child / allies in line learn it. Measured to do almost nothing on its own
   (see below): crossers mostly die outright, and the child rarely hears the word.

Debug counters (`BW15_DEBUG`, compiled out): crossings, crossers dead within 2 rounds, frozen
champion trapped splits and dead-end turns. `BW15_TUNE` also exposes the three new params.

## What I measured (vs body-wall-14, both seats)

| maps | W-D-L | sum longest margin | sum total margin |
|---|---|---|---|
| official (11 maps) | 11-0-11 | +36 | -326 |
| worldgen set 1 (20) | 29-0-11 | +328 | -157 |
| worldgen set 2 (20) | 24-0-16 | +149 | +1601 |

Parent-equivalent settings (all three off) reproduce the parent exactly (11-11, 20-20, 20-20).

Counters, parent settings -> body-wall-15 (official / wg1 / wg2):
- unknown-portal crossings 349/985/1108 -> 12/21/39
- crossers dead within 2 rounds 59/292/337 -> 2/7/6
- frozen champion "into a dead end" turns 287/290/555 -> 0/0/0
- frozen champion trapped splits 103/184/371 -> 45/203/257 (still ~5 a match: most come
  from being boxed in some other way, not from chosen dead ends)

Ablations on the 40 worldgen matches each:
- only the champion fix (cross_min_length=2): 26-14, 21-19 (47-33); deaths after crossing unchanged
- only the portal fix (champ_no_trap=0): 25-15, 24-16 (49-31)
- both: 29-11, 24-16 (53-27)

## What should improve

Fewer dragons lost to sealed portal exits (about 7 a match on worldgen) and a champion that
doesn't cut itself down late. Official maps are seat-decided and stay even.

## Didn't work / next

- The never-cross mark barely changes anything alone (293 vs 292 deaths): drop or find a better
  channel. The length gate is what saves the dragons.
- Remaining champion trapped splits (~5/match) are the next target: trace where they happen
  (detour into doom? walled by enemies?).
- The explore crossing is now nearly off; a smarter exit check could win back useful crossings.
