# gather-split-trade-70

## Idea

Crossover: gather-split-trade-63 (base, renamed gst70) plus body-wall-21's cap-trap fix, ported
verbatim. Every trapped death of length >= 4 happens at the unit cap with no free slot to
escape-split into (league/KNOWLEDGE.md: "The cap trap is set before the cap is reached"); an idle
short stub gives up its slot so a longer dragon can split away.

## What changed from the parents, and why

- Renamed namespace/macros `gst63`/`GST63_*` -> `gst70`/`GST70_*` everywhere (sources, no logic
  change) so it can't clash with the parents when loaded together.
- Ported body-wall-21's step 4b give-up rule into `Turn()`, placed right after the goal is chosen
  (same slot bw21 used: after growth split, before the loop breaker), with the same constants
  (`kGiveUpMaxLen=3, kGiveUpFree=1, kGiveUpMinUnits=32, kGiveUpIdleDist=6, kGiveUpAllyDist=5,
  kGiveUpAllyLen=6, kGiveUpHeads=3`): length 2-3, not a leader, not the champion, not walking to
  feed the champion, <=1 free slot, >32 units, idle (no goal or goal >6 steps away), and either an
  ally length >=6 with head within Manhattan 5, or 3+ other heads in view -> resign
  ("give up slot: long ally" / "give up slot: crowd"). No other logic touched.

## What was measured

Both seats, `check` passes (all maps, slowest turn 0.2ms on official well under the 0.5ms limit).

vs gather-split-trade-63 (parent):
- official: 12-0-10, +2.6 longest, -7.1 total. Give-ups fired 24 (crowd) + 3 (long ally) a match.
- worldgen set 1 (20 maps): 18-0-22, -1.8 longest, +2.8 total. Give-ups 261+21 a match.
- worldgen set 2 (20 maps): 18-0-22, +0.3 longest, -7.0 total. Give-ups 103+5 a match.
- Combined 48-0-54, small margins both ways: essentially a wash against the direct parent, as
  body-wall-21 also found against its own parent (52-0-50).

vs trading-swarm-5:
- official: 16-1-5 over 22 matches.
- worldgen set 1: 29-0-11 over 40 matches.
- worldgen set 2: 29-1-10 over 40 matches.
- Clear improvement here, matching the pattern bw21 reported (83-19 vs the parent's 77-25):
  the fix helps most against opponents that crowd the board and force the cap.

`trapped: no move`/`trapped: split @cap` counts dropped in the give-up build relative to the
un-ported parent's `trapped: no move L2-3 @cap`/`L4-5 @cap`/`L6-9 @cap` buckets seen in the same
matches (e.g. worldgen1: parent had 74+42+25 @cap trapped actions vs gst70's 6+9+5).

## What should improve

Fewer long dragons trapped at the cap, which should show up most against swarm-style or
crowd-heavy opponents, mirroring body-wall-21's result. Even against the direct parent it isn't a
regression.

## What didn't work / open

- Not swept: the bw21 constants were carried over unchanged; no parameter search was done here
  (out of scope for a 20-minute crossover task).
- bw21's own follow-up note (skipping growth splits to keep slots free) was reported worse there;
  not attempted here either.
- Next: bw21 suggested making the long dragon's own view the trigger (it sees the cap coming and
  pings a nearby stub) rather than relying on the stub noticing on its own.
