# ring-sprint-intercept (g52-15)

- **Line:** ring-sprint. **Parent:** ring-sprint-guard2 (arena ring-sprint-21).
- **The one change:** Strategist B g52 proposal #5: short allies near the queen trade heads with
  an enemy head closing on her.

## Idea
arena-r26: P3 "queen killed in a live race" 311 losses (lunged 293); S1 (our queen dies after
theirs) had an ally within 3 tiles in 337 of 653. server-r26: 25 of 68 theirs-first losses had an
ally within 3. ring-sprint-escort's stationed escorts were never near; this uses whoever is near.

## Change (queen_attack.cpp, plugin.cpp, strategy.hpp)
- TryQueenAttack's bounded search is factored into `HeadAttack(o, target, own_queen_head, paid_ok,
  out, rivals)`; TryQueenAttack calls it unchanged (same paid guard).
- `TryIntercept`: a non-queen dragon of length <= 6 (`kInterceptMaxLength`, the proposal's value)
  that sees our queen's head, and sees an enemy head H whose rule reach ceil(Le/4)+Le-2 (+1 segment
  if its body leaves the view, as in BuildLungeField) plus 2, through free tiles in view, covers a
  tile within 1 of her head, puts its head on H's head when HeadAttack finds a path this move (free
  and paid steps, never her sole empty exit). Threats nearest her first.
- Deferral: other short (<= 6, fully seen) non-queen allies in view with a path to H within their
  own rule reach that are nearer by path (ties: lower id) go instead; this dragon does nothing.
- plugin.cpp: tried right after TryQueenAttack (same rescue gate); "intercept: trade with her
  hunter". The judge and the lookahead are kept current (the hunter may step away).
- The base has no field mode, so the proposal's "not the r0 starters in field mode" has no effect.
- `-DINTERCEPT_DBG` logs INTERCEPT (trades taken) and ITHREAT (any non-queen dragon seeing a threat,
  its length, Manhattan distance to H, whether a path exists).

## Local (v ring-sprint-guard2)
- `check ring-sprint-intercept maps/official`: passed.
- Server maps seed 7: 16-0-18; 4 interceptions in 34 games; queen died 41% v 35% (1 seed, noise).
- Fresh mapgen (seed 91527, 44 maps, seeds 1,2): 81-6-89; queen died 66% v 67%; 106 interceptions in
  176 games (mostly length 3). Per family: anti-centre 9-7, crowded 8-8, maze 6-4d-6, mixed 6-2d-8,
  off-axis 7-9, open 7-9, rich-near-home 7-9, ring 7-9, rooms 8-8, sealed-spawn 8-8, sparse 8-8.
- Threat situations are rare (53 dragon-turns in 34 server games); the binding limit is reach:
  short allies that see the threat are mostly 3-4 tiles from H, and a length 2-3 stub reaches 1-2.
  No 7+ ally ever saw such a threat in those games.

## Arena
- Ephemeral duels v ring-sprint-21: server (random:2) 33-0-35, official 10-0-10 (both noise);
  queen alive at end server 58.8% v 63.2%, official 45% v 35%.
- Submitted as ring-sprint-34 (parent ring-sprint-21). Verdict: even with parent, 56-0-56
  (server 34-34, custom 17-17, official 5-5); decided by elimination 26-28, queen 18-19, longest
  12-9. v ring-sealed-2 56-56, ring-field-3 53-59, ring-qforage-1 58-54. CPU p99 24.7M, max 40.9M.
- Eval P(win) v parent: r100 .492, r200 .488, r300 .490 (105/95/85 games).
- Queen in the 112 games v parent: alive at r300 75 v 78, at the end 50 v 55; head-on deaths 60 v
  57 (lunged 53 v 53); with a short (<= 6) ally within 3 at her death 16 v 20. Short-ally trades
  within 4 of our queen 61 v 57. Weak spot not fixed.
- Replays: the trade happens as intended (263249 r224 id266 len3 W,S onto a len-4 hunter 2 from
  her; r286 id491 S,S onto a len-7 hunter beside her; 263280 r370 id818 SSS onto a len-6 hunter),
  but it rarely applies. In the 19 draft-side queen deaths with an ally within 3, at that ally's
  last turn it was 3-7 path tiles from the killer and mostly length 2-3 (reach 1-2); only 4 allies
  of any length had reach >= the distance, 2 of them <= 6. 263120 r356-357: the len-3/4 ally sat
  beside the len-2 hunter, but its own body, then the hunter's body, blocked every path.
