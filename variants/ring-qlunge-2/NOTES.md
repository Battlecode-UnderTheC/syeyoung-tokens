# ring-qlunge-2

Task g38-6. Base: ring-plus-19 (code: variants/ring-capgrow-1).

## Idea
A guarded paid-step lunge on a visible enemy queen. ring-qlunge-1 (ring-plus-18, g38-1) let
TryQueenAttack search paid steps too. That won queen-decided games on 3+-starter maps, but on
1-starter maps (official/arena 0-8 in the arena) our only worker lunged early and traded with their
queen, which left our queen alone. So the paid lunge now runs only when the team can spare a worker.

## What changed from the parents, and why
- `RING12_ATTACK_MODE` 2 -> 1 (strategy.hpp).
- In `queen_attack.cpp`, paid steps are searched only when `units_ - 1 >= kLungeMinWorkers` (4).
  `units_` is the observed UNIT_COUNT, which includes the queen. Otherwise the search depth stays
  at min(12, free steps): the old mode-2 behaviour.
- Nothing else changed.

## What should improve
More enemy-queen kills and queen-decided wins on 3+-starter maps, without the 1-starter collapse.

## Local results (seeds 21-26, both seats, 2 threads, vs ring-capgrow-1)
Groups by starters per team: 1 = Colosseum and arena; 2 = DRAGON_COUNT 4; 3+ = the rest.
Pearl share is not printed by sim, so total length at r50 stands in for it.

| maps | W-D-L | decided (elim / queen / longest) | total r50 (us v them) | dragons r100 | our queen died v theirs |
|---|---|---|---|---|---|
| server, all | 105-4-95 | 38-36 / 52-40 / 15-19 | 30.2 v 30.3 | 21.7 v 21.8 | 33% v 40% |
| server, 2 | 42-0-42 | 19-20 / 21-20 / 2-2 | 16.2 v 16.7 | 13.2 v 13.8 | 39% v 40% |
| server, 3+ | 63-4-53 | 19-16 / 31-20 / 13-17 | 39.9 v 39.9 | 27.6 v 27.5 | 28% v 39% |
| official, all | 83-4-93 | 41-43 / 31-29 / 10-20 | 30.9 v 31.4 | 21.1 v 21.6 | 38% v 41% |
| official, 1 | 10-0-14 | 10-14 elim | 22.1 v 25.3 (15 games) | n/a (most games end before r100) | 67% v 67% |
| official, 2 | 29-0-31 | 13-14 / 13-14 / 2-2 | 19.3 v 19.9 | 13.4 v 14.2 | 37% v 37% |
| official, 3+ | 44-4-48 | 18-15 / 18-15 / 8-18 | 39.5 v 39.5 | 26.2 v 26.2 | 32% v 38% |

- official/arena: 5-7, and the seat decides it (as A 5-1, as B 0-6). ring-qlunge-1 lost 0-8 there in
  the arena. Colosseum: 5-7.
- Fatal "queen12: direct attack" turns: about 0.3-0.6 per match, against the base's 0.1-0.3.
- The 3+-starter queen-decided gain holds (server 31-20). The 2-starter splits are even.
  1-starter is 10-14 (n = 24, a high-variance seat effect): not a clear loss.

## vs cs-tailrisk-7 (g38-3's local port of cs-queen-split-tail-risk-v1-7, not committed; seeds 21-23)
| maps | ring-qlunge-2 | ring-capgrow-1 (base) |
|---|---|---|
| official, 1 | 3-0-9 | 3-0-9 |
| official, 2 | 18-0-12 | 18-0-12 |
| official, 3+ | 25-0-23 | 24-0-24 |
| server, 2 | 20-0-22 | 20-0-22 |
| server, 3+ | 30-0-30 (elim 11-8, queen 13-16) | 29-0-31 (elim 10-8, queen 13-18) |

In total that is 96-0-96 against the base's 94-0-98: the same in every group, and +1 on the 3+ maps.

## Arena verdict
Not submitted: the permission system blocked `tokenz submit`. Bot dir: /tmp/g38-6/bot
(`scripts/make-bot.sh variants/ring-qlunge-2`), ready for the orchestrator.

## Next
If the arena shows the 3+ gain without an official 1-starter loss, keep the guard. If the 1-starter
maps still lose, add a round floor (e.g. r60). On 1-starter maps the guard of 4 workers is reached
by about r20-30.
