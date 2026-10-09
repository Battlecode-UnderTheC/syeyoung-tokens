# ring-shield-twolines (g53-22, arena ring-shield-18)

Base: ring-shield-rush (arena ring-shield-10). Strategist C g53 proposal #3.

## Idea
Late, one feed line at a time can't deliver the material waiting within walking budget of the queen
(ring-shield-3 both-alive queen losses: 74 v 60 length within budget at r470, queen +4.0 v +11.2 over
r470-485, 22 v 15 still unused at r485).

## Change (conveyor.cpp, TWOLINE_MODE; 0 = the parent)
"Throughput mode" from kTwoLineFrom = 470, or earlier when rounds left x her ceil(L/4) free steps <
the material this worker knows within budget (itself + allies in view + reports <= 3 rounds old):
- her line counts busy only at 2 turns of sprint pearls (two lines at once);
- while one line is down, the second is laid only with its end on the other side of her head
  (our offset from her head . mean offset of the pending pearls <= 0);
- the first two in line walk to her zone (the second to a zone tile on the free side);
- the rest within budget walk to the queue just outside her zone (kQueueDist + 1) instead of foraging;
- the extra walking only on a sighting or a report from this/last round (iteration 3: in a
  reproduced arena stronghold game, walkers chased a 3-5 round old report tile while a 33-long
  queen starved r473-486; 36 -> 51 at r499 after the gate).

## Local
- server s1 v ring-shield-rush: iter1 18-0-16, iter2 20-0-14 (both near-identical to the parent).
- fresh mapgen seed 92253, 11 families x 2 seeds: iter2 87-4-85; iter3 every family even
  (anti-centre 6-4-6, all others 8-0-8).
- Lay counts barely move (lay ~5/match both; second line ~0.1/match): busy (pending >= need) is
  rare, the queen eats a line within a turn or two. The limit is finding her, not the busy rule.

## Arena
- Duels v ring-shield-10: iter2 server 32-36, official 8-12 (queen r499 both-alive 38.6 v 42.8:
  worse; 2 late queen deaths v 1). iter3 server 35-33, official 10-10 (queen r485/r499 both-alive
  32.3/38.5 v 29.0/35.0).
- Draft ring-shield-18: 54-2-56 v parent (server 33-35, custom 17-17, official 4-2-4); v
  ring-shield-15 51-2-59, ring-sealed-2 69-43, ring-field-3 63-49. CPU p99 26.5M, max 50.4M.
- Arena games v parent: eval 0.500 at r100/200/300/400, 0.489 at r480. Queen length in both-alive
  games r470/490/500: 18.2/21.8/24.4 v 18.2/21.8/24.3. Decoded 22 longer-queen games: r485 23.8 v
  23.5, r499 28.3 v 28.2, own-drop pearls eaten r470+ 8.3 v 9.0, late queen deaths 0 v 0.
- Weak spot NOT fixed in arena. Traced 289961 (slithery_fight, B, 26 v 33): our queen stays 17
  from r470 to r485 while she walks ~1 row a turn looking for food; workers out of view head for
  different reported tiles (q436 / q816 / q943 / q1004 ... in the same round), so no line reaches her.
  The bottleneck is knowing where she is (and her walking away), not one-line-at-a-time.

## Next
Make her wait for deliveries (stop wandering when the queue is known to be coming), or broadcast her
tile reliably every round late, before adding more feed lines.
