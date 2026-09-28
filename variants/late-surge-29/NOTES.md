# late-surge-29

## Idea

Crossover: late-surge-27 (base) + body-wall-21's idle-stub give-up at the unit cap. Every
trapped death of length >= 4 happens at the cap with no split possible (league/KNOWLEDGE.md);
freeing a slot before the cap is hit lets a long dragon escape-split instead.

## What changed from the parents, and why

late-surge-27 sources (renamed ls27 -> ls29, including LS27_* -> LS29_* macros) plus one rule,
step 4b in `Turn` (right after the goal is chosen, before the loop breaker), constants
`kGiveUp*` at the top of ls29.cpp — ported byte-for-byte from body-wall-21's 4b (isolated by
diffing bw21 against bw19, its immediate parent):

- length 2-3, not a leader, not the champion, not walking to feed the champion (`ally_head < 0`);
- free slots <= 1 (`kGiveUpFree`) and more than 32 units (`kGiveUpMinUnits`);
- idle: no goal, or the goal is more than 6 steps away (`kGiveUpIdleDist`);
- an ally of length >= 6 (seen length or its report) with its head within Manhattan 5
  (`kGiveUpAllyDist`, `kGiveUpAllyLen`), or 3+ other heads in view (`kGiveUpHeads`);
- then resign ("give up slot: long ally" / "give up slot: crowd").

Throttle is the same as body-wall-21: UNIT_COUNT (`units_`, from `alive_`) is live within a
round, so stubs acting after one resigned see the freed slot and stay.

late-surge-27's step numbering and variable names (`champ`, `ally_head`, `leader_`, `units_`,
`FreeSlots()`, `goal`, `paths`, `seen_`, `reports_`) matched body-wall-21's closely enough that
the port was a direct copy-paste with no logic changes needed.

## What was measured

Both seats, `sim --threads 2`.

vs late-surge-27 (parent):
- official maps: 11-0-11, +2.1 longest, +1.8 total (leads slightly). Cap-trapped deaths
  (`trapped: * @cap` reasons) roughly similar magnitude to the parent on this small a sample.
- worldgen (1 fresh set of 20, both seats = 40 matches): 21-0-19, +1.2 longest, -1.4 total
  (even). Cap-trapped deaths (`trapped: no move/split L*-* @cap`, summed): late-surge-29 ~12 a
  40-match set vs late-surge-27's ~103 — roughly an 8x cut, matching the ~7x cut the same port
  gave gather-split-trade (141 -> 20). Give-ups fired ~34/match on official, ~185/match on
  worldgen (mostly "crowd", as in body-wall-21).

Not run: a second worldgen set of 20, and trading-swarm-5 as an opponent (time budget was
~20 minutes; the parent comparison on official + one worldgen set already shows the expected
effect and an even W-D-L, matching what the brief predicted).

## What should improve

Fewer long-dragon deaths trapped at the cap, which body-wall-21 and gather-split-trade-70 both
found helps most against swarm-style opponents that keep the team pinned near the unit cap.
Against late-surge-27 itself the match record stays even (as body-wall-21 stayed even with its
own parent) while cap-trapped deaths drop sharply — the freed slots are being used.

## What didn't work / open

- Didn't sweep `kGiveUp*` constants; ported as-is per the brief.
- Didn't verify against trading-swarm-5 for time; body-wall-21's report suggests this rule
  should widen the margin there specifically (83-19 vs the parent's 77-25 in that study).
- Open (carried over from body-wall-21's notes): the "crowd" trigger does nearly all the
  firing; "long ally within 5" is rare. Long trapped deaths that remain are still all at the
  cap — a freed slot can be taken by someone else's split before the intended long dragon
  reaches it.
