# ring-shield-port2 (g53-10)

Base: ring-shield-bastion (arena ring-shield-3 = ring-shield-1 + g53-7's walled-food frontier).

## Change (one: a port)
g53-6's shield memory (variants/ring-shield-memory, its diff v ring-sealed-shield) applied with
`git merge-file` onto ring-shield-bastion; no conflicts (memory touches LungeShield, bastion touches
ChooseGoal). LungeShield's end choice also scores untradeable enemy heads remembered 1-3 rounds ago
(distance less age * ceil(Le/4), Le >= kShieldPartialLen) and, among equal scores, prefers ends inside
the 7x7 view. See variants/ring-shield-memory/NOTES.md for the reasoning.

Debug only (-DSHIELD_DBG): the candidate validity tests are now one lambda (same tests, same eval cap,
same order; release behaviour unchanged) so the memoryless choice (ring-shield-1's far(): in-view
distance only, no inner tie-break) can be recomputed and compared. New line at exit:
`SHIELDMEM ran N ghosts G | memory changed end: add A keep K other O` (add = memory moved where the
old shield kept the action; keep = the reverse; other = both moved, to different ends).

## Local
- SHIELD_DBG, maps/server seed 1 (34 matches) v ring-shield-bastion: 22-0-12. SHIELDMEM: ran 266,
  with remembered heads 90, memory changed the end in 77 (add 2, keep 8, other 67) = 29% of shield runs.
- Fresh mapgen (--family all --seed 531013, 44 maps x seeds 1,2) v ring-shield-bastion: 90-0-86;
  per family (16 each): anti-centre 8-8, crowded 8-8, maze 8-8, mixed 8-8, off-axis 8-8, open 8-8,
  rich-near-home 9-7, ring 8-8, rooms 8-8, sealed-spawn 9-7, sparse 8-8. No family loss.
- check maps/official: pass.

## Arena duels v ring-shield-3 (eph-4b957bbc)
- server random:2: 34-0-34 (elim 11-11, queen 16-15, longest 6-8); 3+ 20-20, <=2 14-14.
  longest@400 12.6 v 11.9, queen_length@end 9.2 v 9.2. CPU max 36.3M.
- official random:2: 10-0-10 (3+ 3-1, <=2 7-9); longest@300 11.3 v 11.0, queen_length@end 16.5 v 17.8.
  CPU max 35.2M (parent 40.9M).

## Arena verdict: ring-shield-6 (draft, parent ring-shield-3)
- v ring-shield-3: 51-0-61 (45.5%, within noise; "loses to parent"); by suite: server 29-39, custom
  17-17, official 5-5. Decided: elim 24-24, queen 19-21, longest 7-16, total 1-0.
- v others: ring-sealed-2 67-45, ring-field-3 60-52, ring-qforage-1 61-51. Overall 239-209.
- eval P(win) v parent (game.eval, draft side): r100 0.502, r200 0.502, r300 0.509, r400 0.511.
- CPU: worst p99 23.4M, max 54.1M.
- Targeted weak spot NOT fixed. Late (r380+) head-to-head deaths of our 15+ dragons in the 112 games
  v parent (strategist A's big.py classification, killer at the victim's last turn): blind-in-reach
  workers 30 (+3 queens) draft v 27 (+1) parent; vis-in-reach 39 v 32; all classes 119 v 101.
- Replay 278071 (big_empty, draft B, lost on longest 42-57): 6 blind-in-reach worker deaths v the
  parent's 1. The killers shadow at Chebyshev 4 (d(2,4), (1,4), (4,0)) after being in view 1-2 rounds
  earlier (r403 d(2,2) -> r405 death of a 19 after escape runs WNNEE, NWWWS, SSS). With a ghost
  grown by age*ceil(Le/4) (6 for a 2-round-old 10-12 long) every end within ~6 tiles scores 1, so the
  memory cannot separate the ends exactly where the shadowing killer sits; runs still end in reach.
- Next: score a remembered head by where it can be next turn from its last seen tile and heading
  (or require the run to leave the ghost's reach rather than ranking saturated scores).
