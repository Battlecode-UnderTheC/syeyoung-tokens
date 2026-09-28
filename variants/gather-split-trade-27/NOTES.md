# gather-split-trade-27

## Idea

gst23 plus cap-aware trap safety. A SPLIT with no free unit slot kills the splitter, so a
dragon of length >= 4 near the unit cap can't count on splitting out of a dead end. With
fewer than `cap_margin` (2) free slots it plans like a length-2 dragon: no dead-end goals in
ChooseGoal, no "into a dead end worth it" steps, and a step must keep its room need (or be
level-2 escapable). Also added (off, `reserve_slots = 0`): growth splits leave N slots for
trapped dragons of length >= `reserve_length` (6); shorter ones split out only above the reserve.
Trapped-resign reasons now carry a length bucket and "@cap" (<= 1 free slot) for tallies.

## What changed from the parents, and why

Namespace gst23 -> gst27. `CanCountOnSplit()` (planning) / `CanSplitNow()` (actual trapped
split) replace the bare `length_ >= 4 && units_ < unit_limit` tests. With cap_safety=0 and
reserve_slots=0 it plays identically to gst23 (verified: 20-0-20, margins 0.0/0.0).

## What I measured (vs gather-split-trade-23)

| config | official (22) | worldgen A (20 maps, seed 6201) | worldgen B (seed 6202) |
|---|---|---|---|
| cap_safety=1, reserve 0 (committed) | 12-0-10, +1.0 longest, -1.4 total | 21-0-19, -0.3 longest, +13.1 total | 22-0-18, -0.1 longest, +19.2 total |
| cap_safety=1, reserve 2 | 11-0-11, +0.3 / -4.0 | 21-0-19, -1.9 / -6.3 | 21-0-19, -1.7 / -9.9 |

Trapped resignations per match on worldgen A (fatal "trapped: no move", by bucket):
- baseline (= gst23): L2-3 166, L2-3@cap 58, L4-5@cap 21, L6-9@cap 10, L10+@cap 2 (L4+@cap ~33)
- cap_safety only: L2-3 163, L2-3@cap 64, L4-5@cap 23, L6-9@cap 8, L10+@cap 2 (L4+@cap ~34)
- cap_safety + reserve 2: L2-3 237(+7@cap), L4-5 25 (reserve blocks their split), L4-5@cap 4, L6+@cap 0.6

## What should improve

Slightly more total length on maze-like maps; no change to longest. Mostly neutral.

## What didn't work / surprises

- Cap safety barely changes the count of long dragons resigning at the cap (~33/match): they
  are not dying from *choosing* dead ends, they get boxed in (by bodies/others) at the cap.
- The reserve does what it says (long @cap resigns 33 -> 5/match), but costs: 2 fewer slots
  for growth, and L4-5 dragons blocked by the reserve resign instead (+25/match); net worse
  on margins. Consistent with KNOWLEDGE: idle slots are the problem at the cap, not escapes.
