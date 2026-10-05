# ring-rescuefix-1

## Idea
Fix the dangling `else` in rescue.cpp (g39 audit, B-list note; confirmed by server-r15).
`if (complete && move) for (...) if (!advance(...)) {...} else predicted_valid = false;` bound the
`else` to the inner `if`, so every successful step cleared `predicted_valid` and `would_close`
("would my move fill the queen's last free exit?") never fired for moves; it could only fire for
non-move actions (splits), judged on the unmoved body.

## What changed from the parents, and why
ring-sched-m3 (arena ring-sched-17) + braces around the `for` and the `else` in rescue.cpp. Nothing else.
Now a worker whose planned move would close the queen's last free exit looks for a short "yield"
route (existing beam search) that leaves an exit; a split no longer triggers the check.

## What should improve
Fewer queen deaths against our own bodies. Little at stake: it is rare.

## Local (fresh mapgen maps, --family all --seed 41505 --count 44, seeds 1,2, vs ring-sched-m3)
- 86-4-86 over 176. Per family: anti-centre 8-8, crowded 7-2-7, maze 8-8, mixed 8-2-6, off-axis 8-8,
  open 8-8, rich-near-home 7-9, ring 8-8, rooms 8-8, sealed-spawn 8-8, sparse 8-8. No family clearly worse.
- Instrumented (temporary stderr, removed): would_close fired 19 times in 176 games (~0.1/game), all
  with our body not currently blocking; 16 produced a "rescue: yield" (mostly 1-step), 3 fell through
  to the normal move. No freezing seen (yields are 1-8 steps and the check is per-turn).
- build/tools/check ring-rescuefix-1 maps/official: pass.

## Arena duels vs ring-sched-17 (eph-4bd87b7c)
- server random:2: 34-0-34; queen alive at end 57.4% vs 55.9%, queen length 10.03 vs 9.75. 3+ 20-20, <=2 14-14.
- official random:2: 10-0-10, all metrics identical (the check never fired).
