# body-wall-21

## Idea

Supply side of the cap trap: every trapped death of length >= 4 happens at the unit cap with no
split possible (g15-18), so idle stubs give up their slot when the team is at the cap and a long
dragon or a crowd of heads is near, leaving a free slot for a long dragon's escape split.

## What changed from the parents, and why

body-wall-19 (namespace renamed bw21) plus one rule, step 4b in `Turn` (after growth split,
before the goal step), constants `kGiveUp*` at the top of strategy.cpp:

- length 2-3, not a leader, not the champion, not walking to feed the champion;
- free slots <= 1 (`kGiveUpFree`) and more than 32 units;
- idle: no goal, or the goal is more than 6 steps away;
- an ally of length >= 6 (seen length or its report) with its head within Manhattan 5, or 3+
  other heads in view;
- then resign ("give up slot: long ally" / "give up slot: crowd").

Throttle: UNIT_COUNT is live within a round (the engine's `alive_`), so once a stub resigns the
stubs acting after it see more free slots and stay: at most 2 a team a round from 0 free, 1 from 1
free. No sonar needed.

## What was measured

All sims both seats. Worldgen sets are 2 fresh sets of 20.

vs body-wall-19:
- official: 11-0-11, -0.1 longest, +9.1 total. Trapped 45.0 vs 45.5 a match.
- worldgen 1: 21-0-19, -2.6 longest, -15.1 total. Trapped 276 vs 302 a match.
- worldgen 2: 20-0-20, +2.1 longest, +11.8 total. Trapped 166 vs 237 a match.

vs trading-swarm-5 (parent in brackets):
- official: 13-0-9, +3.0 longest (15-0-7, +6.4).
- worldgen 1: 36-0-4, +18.5 longest, +120 total (33-0-7, +15.2, +125). Trapped 235 vs 320 a match.
- worldgen 2: 34-0-6, +15.9 longest, +102 total (29-0-11, +15.9, +85). Trapped 153 vs 237 a match.

Give-ups: ~37 a match on official, ~350 a match on worldgen vs trading-swarm-5 (about 9 of 10 by
"crowd").

Trap study (BW21_DEBUG, worldgen 1 vs trading-swarm-5, 40 matches): 9,386 trapped deaths; length
2: 6,333, 3: 2,393, 4-5: 329, 6+: 331. Every length >= 4 one (660) was still at the cap (free <= 1)
with no split possible. The parent's study (different map set) had ~1,000 at 6+ and ~2,050 at 4+.

## What should improve

Fewer long dragons dying trapped, which helps most against swarms (trading-swarm-5: 83-19 over
102 matches vs the parent's 77-25). Against the parent it is even (52-0-50).

## What didn't work / open

- The "crowd" trigger does almost all of the firing; the "long ally within 5" trigger is rare.
- Long trapped deaths are still all at the cap: a slot is freed but taken by someone else's
  split, or the stub is not close enough. A next step could make the long dragon's view the
  trigger (it sees the cap coming and pings a nearby stub).
- Not swept: `kGiveUpFree`, idle distance, heads threshold, minimum units.
