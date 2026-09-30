# gst-search-1

## Idea
gather-split-trade-131 (v8), with the arena-search line's trade judgement: a head-to-head is
worth it when we come out ahead once the remains are eaten, not by a length rule. Everything
else, including v8's corridor and dead-end farming and its room checks, is untouched.

## What changed from the parent, and why
- **WorthTrading (remains_trade = 1).** The trade value, for us, is what they lose minus what
  we lose, plus remains × (2 × share - 1).
  - remains = ceil(ours / 2) + ceil(theirs / 2): the pearls both corpses leave.
  - share = (our heads + 0.5) / (all heads + 1), counting heads seen this turn within 5 tiles
    of the enemy head, excluding the two that die.
  - We trade when the value is > 0. The one exception kept: never with our last dragon.
- **The same test drives:**
  - the kamikaze (an enemy head next to ours);
  - the lunge;
  - Exposed: which tiles next to enemy heads to keep off. An enemy only trades when it comes
    out ahead.

## The three arena-search mechanisms, each measured vs gather-split-trade-131 (official maps, seeds 1-10, 300 games)
| build | W-D-L | kept |
|---|---|---|
| S1: remains-aware trade | 198-0-102 (66%) | yes |
| S2: S1 + no fear of enemy heads (Exposed off) | 169-0-131 (56%) | no |
| S4: S1 + split without waiting for calm (split_crowd_limit 3 -> 99) | 186-0-114 (62%) | no |

Only the trade judgement transfers. v8's Exposed rule guards only tiles next to heads. With
the new trade test it already means "an enemy that would come out ahead", so removing it gives
free trades away. The crowd gate on splits protects children born into ally scrums.

(Parent notes follow.)

# gather-split-trade-131

## Idea

strategist-o's ablation study (strategy/strategist-o-r7.md) on gst115, each ablation alone and by
parameter only. Kept Exp 3, unified trap costs: one flat `trap_cost` (3.0) for dead ends all game,
no cheaper opening cost and no `trap_loss` expected-loss model.

## What changed from the parents, and why

Parent gst115. Only two parameters differ: `trap_opening_cost` 0.5 -> 3.0 (= `trap_cost`) and
`trap_loss` 1 -> 0. Dead ends are now charged a flat 3.0 whatever the dragon's length or the
pocket's size. Net effect: dragons enter dead ends about half as often ("into a dead end worth
it" 319 v 605 on worldgen, 127 v 207 on official) and fewer short dragons die trapped
("trapped: no move L2-3" 211 v 251 worldgen). The `opening_end` window still gates the opening
nearest-pearl fallback (Exp 1 showed that one matters).

## Measurements (all vs gather-split-trade-115, both seats; sim is gst131-variant vs gst115)

| ablation | official (11 maps) | worldgen seed 113000 (20 maps*) | total |
|---|---|---|---|
| (1) opening_end = 0 (no fallback, no cheap opening trap) | 7-0-15 | 20-0-20 | 27-35 |
| (2) portal_cross = 0, poor_cross = 0, poor_portal_mult = 1 | 11-0-11 | 19-0-21 | 30-32 |
| (3) trap_opening_cost = trap_cost = 3.0, trap_loss = 0 | 11-0-11 | 24-0-16 | 35-27 |

(3) confirmed on a second independent set, worldgen seed 114000 (20 maps): 26-0-14, final
longest margin +10.3, total +19.9. Combined (3): 61-0-41.

*seed 113012 produces a broken map (sim throws "segment 8 is not adjacent"); replaced by 113020.

Counters:
- (1): fewer dragons at r50/r100 on official (9.2/17.8 v 11.3/22.4), final margins -4.9 longest,
  -21 total. Loses on devil (both seats), schooltime (both), default (both). Confirms the
  opening fallback is live and helps (consistent with gst115 > gst101 37-25); strategist-o's
  "dead code" claim is wrong.
- (2): near-even but slightly behind (margins -2.4/-2.0 longest); portal crossing is rare (a
  handful of "portal: explore" per 40 games) so removing it barely matters in sim; keep it
  given its Trauma wins vs trading-swarm-2 in earlier work.
- (3): even on official (margins -3.3 longest, -9 total there), clearly ahead on worldgen.

## What should improve

Fewer dragons spent entering pockets and dying trapped at L2-3; slightly more dragons and total
at r50 on worldgen. Simpler: two rules removed (opening trap discount, the trap_loss model).

## What didn't work

Exp 1 and Exp 2 (above). `check`'s slowest-turn numbers were unreliable this run (machine load
avg ~55; gst115 itself showed 38 ms spikes); this change only removes computation.
