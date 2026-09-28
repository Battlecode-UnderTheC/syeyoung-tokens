# gather-split-trade-23

## Idea

Stop the team walking to a dead champion. gst15 picks the champion as the longest ally
report under 40 rounds old, and a report is only ever relayed with the round the champion
itself last sent it. A champion that dies (or, most often, gets trapped and splits its mass
off into a child, leaving a 2-long stub under its old id that dies a turn later) stays
"champion" for everyone for up to 40 rounds. gst23 scores reports by
`length - 0.5 * age` instead: a ghost's score fades, and the live heir (which pings itself
with a fresh round every turn) takes over within a few rounds.

## Diagnosis (gst15, temporary debug dump at rounds 340/380/420/460/499)

Losses at 499 with more total length, vs swarm-hunt-4, counter-swarm-5, gst13 (worldgen
pg_81426, pg_81433, pg_81435, official schooltime):

- **Most of the team believes in a dead champion most of the late game.** E.g. pg_81426
  (sh4): at 340 all 63 of 65 dragons named 799 (dead since 328); at 380 32/54 named 508
  (dead); at 460/499 ~half named 965/1071/1095, all dead. pg_81433 (gst13): 64/67, 52/58,
  46/47, 25/38 believe a dead champion at 340/380/460/499; pg_81435 (cs5): 65/65, 60/60,
  57/57 at 340/420/460. Only 0–1 dragons named the real longest in those rounds.
- **How champions die:** almost always `trapped: split` then `trapped: no move`: the
  champion is boxed in, splits `length-2` off (the child walks out with the mass), and the
  2-long stub (the champion's id) resigns. The child is a new id; its reports lose to the
  ghost's larger length. Occasionally the champion is killed (a strike took a 58-long one
  at 442 in pg_81435).
- **Where the mass is:** at 499 we typically have 20–40 dragons, the longest 30–40 and many
  of 15–30 (pg_81426: 39, 37, 33, 31, 29, 28, 25, ... total 439): several separate "champions"
  grew in parallel because feeders walked to ghost positions or didn't feed at all
  (`champ.length >= length_` fails when the believed champion is a stale, shorter record).
- **Also seen (not fixed here, g5-4's topic):** before round 350 we sit at 64 dragons, and a
  trapped dragon that can't split resigns whole: ~110 dragons of length 4–19 died
  `trapped: no move` at the unit limit in pg_81426 alone (incl. the champion 799).
  After 340, ~190 `trapped: no move` deaths of 2–3-long stubs/workers vs only 16 feeds (pg_81435).

After the fix (same traces): believe-dead dropped to 0–17 of ~30–46 dragons, the longest at
499 went 39 → 78 (pg_81426 vs sh4) and 50 → 63 (pg_81435 vs cs5).

## What changed from the parents, and why

- `Champion()`: pick max `length - report_fade * age` (report_fade 0.5) instead of max
  length. Nothing else. (Namespace gst15 → gst23.)

## Measured

Fresh worldgen maps (20, seed 777, both sides) and official maps:

| opponent | maps | gst23 | gst15 | longest margin gst23 / gst15 |
|---|---|---|---|---|
| gather-split-trade-15 | official | 10-0-12 | | -3.5 |
| gather-split-trade-15 | worldgen fresh | 19-0-21 | | +0.5 |
| gather-split-trade-15 | worldgen (diagnosis set) | 22-0-18 | | +2.1 |
| counter-swarm-5 | worldgen fresh | 32-0-8 | 26-0-14 | +12.5 / +5.8 |
| gather-split-trade-13 | worldgen fresh | 19-0-21 | 25-0-15 | +0.5 / +2.1 |
| swarm-hunt-4 | worldgen fresh | 27-0-13 | 25-0-15 | +9.6 / +4.7 |

Total-length margins fell (more of the mass goes through feeding, which passes on only
every second segment), longest-dragon margins rose against the swarms.

## What didn't work

- A handover message: the champion's stub names its child the heir (id + predecessor in the
  sonar word). The stub can't: it is trapped (its pings hit kelp), dies the next turn, and
  the child's head is `length` tiles away, out of its window.
- A retraction (the splitting champion pings its new length 2): same reason, it reaches
  almost nobody.

## Next

- Stop the champion getting trapped at all (it is the main way champions die): stricter
  room checks for the champion only, or never let it enter dead ends after ~300.
- At the unit limit, a trapped long dragon resigns whole; free a slot (g5-4) or feed.
- report_fade was not tuned (0.5 only).
