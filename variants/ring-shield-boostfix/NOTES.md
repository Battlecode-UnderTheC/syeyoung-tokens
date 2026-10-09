# ring-shield-boostfix (g53-8)

Base: ring-sealed-shield (arena ring-shield-1). Brief g53-8: the real opening holder traced by g53-5
(variants/ring-sealed-fallback/NOTES.md) on arena 262844 (trophy, id3 walks home at r28-29).

## Trace on this base (262844 reproduced: `match ring-sprint-intercept <us> trophy 17910617060193542843`,
## elimination r148, our queen dies r113; -DWALLIN_DBG CG lines, team 1 id3)
- Parent r28 h(11,16): scored best (12,19) .132 = raw .545 (incl. cluster bonus) x.1 scheduler (S) x2
  contest boost (C); then the opening fallback (ring-shield-1 does NOT have g53-5's fix) overrides it
  with (11,19), plain .167 (its own crowd x share discount .055 ignored). r29 the same.
- Boost fix alone (no contest boost on a pearl sched_other): (12,19) drops to .066, the scored best
  becomes (8,14) .067 (id3's OWN scheduled pearl, NW, away from base), but the fallback still sends id3
  to (11,19): the game is move-for-move the parent's. On server maps seed 1 the openings r10-r100 are
  identical to the parent: alone it is inert on this base.
- So on ring-shield-1 there are two holders: the boost (x2 on an ally's pearl) and the plain fallback.
- Cluster bonus: not part of the hold; once the boost is gone (12,19) (cluster included) is below
  id3's own pearl, so the cluster rule is unchanged.

## Change (strategy.cpp ChooseGoal; BOOSTFIX_MODE / FALLBACK_FIX 0 = parent)
1. No contest boost (kContestDeny x2) on food the scheduler matched to another of our dragons.
2. Needed for 1 to act here: the opening nearest-pearl fallback compares the pearl's score after the
   ALLY discounts ChooseGoal gives it (crowd, share, scheduler-other), i.e. g53-5's fix narrowed to the
   same rule ("a pearl another of our dragons will take does not hold this dragon"). The enemy-contest
   discount (x.3) is left out: with it in (g53-5 as written), islands went 6-18 v the parent over 12
   seeds (fallback-only ablation 7-17; boost-only 10-14); the prevented overrides there were mostly
   enemy-only x.3 at r100-150 and we ate less by r100 (105 v 113). Narrowed: islands 15-9.
- 262844 after: r28-29 id3 goes for (8,14) (its own matched pearl, NW, away from base) instead of
  base pearl (11,19); r30 W-field pearl, r31 back toward (12,19) once no ally holds it (unshared,
  1.083), r32-34 (8,14). Not strictly "explore", but no longer farming the ally's base pearl.
  The match flips: we win by elimination r159 (their queen dies r149; ours lives).
- -DFALLBACK_DBG prints each fallback override the plain score would have made (KEEP/DISCOUNTED).

## Local (v ring-sealed-shield; tokenz timed out, so no arena duels)
- check maps/official: passed.
- Fresh mapgen --family all --seed 53871, 44 maps x seeds 1,2: 84-92 per family: anti-centre 9-7,
  crowded 8-8, maze 8-8, mixed 8-8, off-axis 7-9, open 8-8, rich-near-home 5-11, ring 6-10, rooms 8-8,
  sealed-spawn 9-7, sparse 8-8. r50 totals equal or slightly higher in every family.
  Re-runs: rnh seeds 3-6 11-21, but 8 fresh rnh maps (seed 53999) seeds 1-2 18-14 (results are seat-
  mirrored there: chaos); ring seeds 3-6 14-18. No clear loss outside rnh's noise.
  (Before narrowing, with the enemy discount in: 94-1-81, but islands 6-18.)
- Server maps seed 1: 18-16; openings r10-r40 identical (mirror openings rarely hit the case).
- trophy/devil/islands/stripes seeds 1-4: 18-14 (trophy 4-4, devil 5-3, islands 5-3, stripes 4-4);
  r50 total 32.3 v 32.4. Islands seeds 1-12: 15-9.

## Arena
Not submitted: `tokenz ephemeral` timed out twice (sandbox). Bot: scripts/make-bot.sh
variants/ring-shield-boostfix <out>.

---

# ring-sealed-shield (g53-1)

Base: ring-sealed-wallin (arena ring-sealed-2). Strategist A g53 proposal #1 (big-dragon lunge shield).

## Idea
A-g53 decoded ring-sealed-2's arena losses: 104 of ~160 late head-to-head deaths of our 15+ workers had
the killer in view and inside its TRUE lunge reach; 51 of 64 such killers were partly visible, and
BuildLungeField gave them reach from seen length + 1 (a 9-long stub seen as 3: reach 3 not 10).
Killers' true lengths at the victim's last turn: 2-8 in 139/170 (median 5-6), Manhattan 2-4.

## Change (one behaviour: the lunge shield for non-queen dragons of 12+)
- BuildLungeField: for our length >= 12 (kShieldMin), a partly visible enemy counts as length
  max(seen+1, 8) (kShieldPartialLen; QueenRisk's 8 for partial enemies) in lunge_mark_, so LungeAvoid,
  SafeStep's lunge_guard and its escape-run trigger see it. A per-tile nearest-head distance
  (shield_dist_) is kept with it.
- plugin.cpp, just before ProtectQueenAction: StrategyAgent::LungeShield. If the final move's end tile
  is in that reach, take the single step or free run (2..ceil(L/4), no segment paid) whose end is
  furthest from the nearest untradeable head (outside every reach best), only if further than the
  chosen move's end, not Exposed, room >= RoomNeeded, EscapeLevel >= 1, path free; at most 16
  room/escape evaluations. WorthTrading enemies are not in the field (trades still happen).
- Iterations: v1 = proposal as written (unknown length, clear step else LungeEscapeRun): on fresh maps a
  clear end existed in 10 of 12229 exposed turns (with unknown length the whole 7x7 view is in reach),
  so it was inert. v2 = furthest end instead of clear-only. v3 (submitted) = v2 with partial length 8.
- Debug: -DSHIELD_DBG prints SHIELD counters; -DSHIELD_DBG -DSHIELD_OFF is the parent's behaviour with
  the same counters.

## Local (fresh mapgen --family all --seed 53101, 44 maps x seeds 1,2, v ring-sealed-wallin)
| build | W-L | longest-decided | lunged by enemy, 10+ (/match) | shield moves |
|---|---|---|---|---|
| parent (OFF) | 88-88 | 25-25 | 9.2 (len 137) | - |
| v1 | 89-87 | 25-26 | 8.7 (len 127) | 10 total |
| v2 | 84-92 | 21-29 | 7.6 (len 113) | 7596 (43/match) |
| v3 | 86-90 | 24-26 | 7.7 (len 114) | 7523 |
Server seed 1 smoke (v1): 14-20 (OFF mirror 17-17), mostly seat-mirrored chaos.

## Arena duels v ring-sealed-2 (server suite random:2 = 68 games; official 20)
- v1: server 36-32 (longest-decided 10-3), official 10-10.
- v2: server 37-31 (longest 6-4), official 10-10.
- v3: server 32-36 (longest 6-9).
All within noise. CPU max 31-47M.

## Arena verdict (ring-sealed-7, v3)
- vs ring-sealed-2: 58-0-54 (51.8%, BEATS parent within noise). By suite: server 37-31, custom 17-17,
  official 4-6. Decided by: elimination 27-26, longer queen 22-17, longest dragon 9-11.
- Others: ring-field-3 63-49, ring-qforage-1 59-53, ring-sprint-21 53-59. CPU p99 23.3M, max 50.4M.
- Eval P(win) v parent: r100 0.501, r200 0.500, r300 0.501, r450 0.502. Longest at r490 27.5 v 29.6.
- Targeted weak spot (A-g53 big.py method, both sides of the same 112 games): late (r380+) head-to-head
  deaths of 15+ non-queen dragons, us v parent: 95 v 110; killer visible and in true reach 33 v 63
  (share 35% v 57%); but killer out of view and in reach 21 v 8 (runs end near unseen stubs).
- Decoded: free multi-step moves of our 12+ workers with an enemy head in view, r200+, us v parent:
  267176 (fixed-home-clusters) 161 v 10, 267182 (slithery) 13 v 0, 267167 (slithery) 11 v 5. The
  shield runs 2-4 free steps away from stubs of 2-10; deaths in view fall, but longest at r490 does not rise.
