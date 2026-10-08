# ring-sealed-qhunt (g53-4, arena ring-sealed-9)

Base: ring-sealed-wallin (arena ring-sealed-2). Strategist B g53 proposal #1: hunt the enemy queen
when she leads the queen race. Code: qhunt.cpp (+ hooks in Decide/Turn, members in strategy.hpp).

## Step 0 (trigger rate)
- Temporary sighting log (QHUNT_DBG-style print in Decide) on trauma, portals, maze, big_empty and 4
  fresh `open` maps (mapgen seed 53401), seeds 1-4, both seats, v ring-sealed-wallin and
  v ring-sprint-guard2 (= ring-sprint-21): 128 games, 62 losses, 23 decided by queen.
- Trigger (enemy queen seen at r>=380 with length >= max(8, our known queen + 3), or our queen known
  dead) held in 5 of 23 queen-decided losses (22%) and in 7 wins. Above the 10% bar, so implemented.
- But every trigger was late: first trigger r471, 479, 485, 495, 496. Both queens are short until the
  ring exit (r430), so a 3-length lead rarely shows before the last 30 rounds.

## Change
- Sighter (worker) stores her tile/round/length and sends a 0xBC word (team bit 55, x 0-5, y 6-11,
  round 12-20, length 21-27) in place of one ping for 3 rounds; a hearer adopting a newer word younger
  than 3 rounds re-pings it once.
- Pursuit (HuntMove, before Conveyor in Decide): worker len >= 4, word <= 10 rounds old, enemy queen not
  seen dead since, our queen not sealed, re-checked that our queen isn't the longer one by its own
  knowledge; goal = her tile or a neighbour within (500-r)/2 path steps; yields to 2 nearer ally heads
  in view; SafeStep, walking_to_ally_ so plugin keeps it; TryQueenAttack does the kill.
- Iteration 2: the ping direction. Iteration 1 (round & 3) reached no ally in 24 race games (18
  sightings, 0 hears: a ping stops at the first dragon, mostly enemy bodies near her). Now the word
  goes on the direction whose visible ray first meets an ally (else round & 3): 6 hears.

## Local
- Race maps (trauma/portals/maze, seeds 1-4, v base): 12-0-12, 18 trigger sightings, 6 hears, but only
  1 pursuit move. Rejections: hearer knows our queen is longer (24), out of (500-r)/2 reach (15), no
  path/already beside (15).
- Fresh mapgen (seed 53417, 44 maps, all 11 families, seeds 1,2): every family 8-0-8 and mirror-
  identical: the hunt never fired.
- Light arena duels (iteration 1, eph-9734e993): server random:2 35-0-32 (52.2%, noise; queen-decided
  17-15; 3+ starters 21-18, <=2 14-14); official random:2 10-0-10 with metrics identical to the base.

## Verdict
- Arena ring-sealed-9 (iteration 2): v parent ring-sealed-2 55-0-57 (49.1%, noise; arena says loses to
  parent). By suite: server 34-0-34, official 4-0-6, custom 17-0-17. Decided by: elimination 26-26, longer
  queen 19-20, longest 9-10, total 1-1. v ring-field-3 58-54, ring-shield-1 54-58, ring-qforage-1 55-57.
- CPU: p99 22.6M, max 46.1M.
- eval P(win) v parent at r100/r200/r300: 0.500/0.500/0.500 (every pair identical through r300).
- Of 56 seed pairs v parent, 55 are exact mirrors: the change altered ONE game (269539, default_small,
  we are A, our queen died r399). Replay: the 0xBC word went out from r461 and was relayed between our
  dragons 213/268-272 (23 hunt pings; several hit allies); dragon 271 turned toward her last tile
  (E, SEN, E, SSS, ...) where the parent's 271 went EES/S. Her queen (~20 long) moves 4-5 tiles a turn
  (NWWN, NNEE, SESEE) so a 1-step pursuer chasing a stale tile never reached her. In the parent's
  mirror game (269526) its dragons ran into her by chance and lunged her at r482 (won on longest 48-13);
  ours did not and lost 0-15 on queen. Enemy queens dead at r380+ in our games 8, ours 9 (mirror).
- Conclusion: the targeted weak spot is not fixed. The trigger exists (22% of local race losses) but
  only in the last ~30 rounds, the hearer's re-check usually finds our queen longer, and pursuit of a
  sprinting long queen by walking to her last tile does not catch her.

# ring-sealed-wallin (g52-13, arena ring-sealed-2)

Base: ring-field-sealed (arena ring-sealed-1). Strategist B g52 proposal #8 (contested walled fields).

## Step 1: the holding term (traced)
- Server-r26 Devil stand-off reproduced from arena game 254366 (ring-field-3 v server:TTT, decoded):
  the field is the walled column block x=14..17, y=5..10, entered only from rows 3-4 / 11-12. Our
  heads stood at x=10..13 against its west kelp wall r9-r40 while TTT walked in from the top and
  bottom; the field's pearls stayed uneaten by us.
- Local trace (build with -DWALLIN_DBG: WD per-turn goal lines, CG goal-score breakdown, BW when
  the exemption picks the goal), sim devil, server seed 14429235285695362080, v ring-field-sealed:
  starter id4 "arrived" (field mode, pearls in view) at (12,8) r10, outside the wall. ChooseGoal
  then scored the field pearls (14,7)..(15,10) at raw .27-.35 per round, but each carried the SHARE
  discount (x0.1): an ally (id2 at (10,9)/(11,9), or a sonar report) whose straight-line distance
  (3-4) is below our WALKING distance (7-10). It took a home bed (9,6) at .13 and walked away; it
  reached the field only at r29. Not the trap/RoomNeeded cost (no T flag; unknown counts as open),
  not Exposed, and contest_factor (E) appears only from r24 on 2 tiles. Crowd (c1-c3) and the
  scheduler (S) add smaller discounts.
- So the hold is a metric mismatch: beside a wall every ally at the wall is "nearer" than we are.

## Change (ChooseGoal share discount only)
Before r60, for a non-queen, non-champion worker with HeadsNear(head,4) ours >= theirs: for food
behind kelp (kelp-only BFS walk over known ground, bodies ignored, > straight line) whose 7x7 view
holds live/soon-due beds (BedWeight) >= our heads within 4 of it (us included), the share test
compares an ally's straight line with OUR straight line (not our walk). First version dropped the
share discount outright and counted body detours as walls: it fired too broadly (rich-near-home
openings r40 49 v 75); v2 = kelp only + fair comparison.

## Local
- Devil trace after: id4 targets (14,8)/(15,8) at r10, walks (13,8)->(13,11)->(14,11) and enters by
  r15 (contest race/trade) instead of r29.
- v ring-field-1, seeds 1-4: devil 4-4 (r40 total 25.1 v 24.9; base 25.6 v 25.5), trophy 4-4
  (18.6 v 18.1; base 18.1 v 18.1), islands 4-4 (45.8 v 45.6; base 3-5). Mirror opponents rarely
  hold the field, so locally the effect is small.
- Fresh mapgen seed 52131, 44 maps x 2 seeds v ring-field-sealed: 80-96 total; anti-centre 8-8,
  crowded 6-10, maze 6-10 (r40 total 26.2 v 22.4, we eat more), mixed 7-9, off-axis 9-7, open 8-8,
  rich-near-home 5-11 (but seeds 3-6 14-18 with openings 61.9 v 63, and 8 fresh rnh maps 14-18 with
  openings identical: mostly seat-mirrored chaos), ring 8-8, rooms 7-9, sealed-spawn 8-8, sparse 8-8.
- Duels v ring-sealed-1 (random:2): v1 server 32-36, official 12-8; v2 (eph-db1b8445) server 34-34
  (3+ starters 19-21, <=2 15-13; dragons@100 25.2 v 23.8, board control@200 .51 v .45), official
  11-9 (board control .52 v .46).
- check maps/official passes.

## Arena (ring-sealed-2, draft, parent ring-sealed-1)
- Verdict: 84-0-56 v parent (60.0%, CI 51.7-67.7) -> BEATS parent; server suite 55-31, official 7-5,
  custom fixed-* 22-20. Decided by: elimination 30-25, longer queen 34-17, longest 20-12, total 0-2.
- v others: ring-field-3 69-61, ring-qforage-1 71-71, ring-sprint-21 64-64. All 540 games 288-252.
- Eval P(win) v parent (game.eval, our side): r100 .500, r200 .539, r300 .500 (134/122/107 games):
  the W-L margin is larger than the eval moves; treat it as near-even.
- Per-game metrics v parent: eaten r40 38.6 v 37.7, r60 77.3 v 75.0, dragons r60 23.3 v 22.6.
  Devil/Trophy/Islands v parent (12 games): eaten r40 33 v 35, 6-6.
- CPU: worst p99 22.7M, max 47.4M.
- Did the targeted weak spot move in the arena? No measurable change. Decoded Devil replays (pearls
  eaten inside the walled field x14-17,y5-10 by r60): ring-sealed-2 and the parent produce the same
  games against our own lineage (seed 13802921253180459702: whichever side the spawn order favours
  takes 40-49 field pearls, the other 5-11, for both versions; 258832, 258568, 258794). Game 258832
  (us v parent): our heads 5 and 3 enter the field from the bottom and top by r16-17 alongside theirs
  and lose the contest; it is not the wall stand-off. The stand-off needs an opponent that walks in
  while our heads hold back (server TTT, 254366: field 3 v 46 by r60), and the arena pool has none,
  so the arena games cannot confirm the fix. Locally the traced stand-off is gone (above).

# ring-field-sealed (g52-10)

Base: ring-field-freecost (arena ring-field-18). Strategist B g52 proposal #3.

## Idea
A queen sealed in a closed pocket can't grow (or only up to the pocket's size), and maps are
symmetric, so both queens tie and the game goes to the longest dragon (server-r26 Schooltime: 19
losses all on longest with queens at 3; arena mg40506, queens in a 3x4 kelp pen: 3-33).

## Change
- `UpdateSealed()` (end of strategy.cpp): any of our dragons that sees our queen (she sees herself)
  floods her component over known edges. An unseen edge, unfound portal end or never-seen tile
  means open. Sealed when the component is closed and has at most 3L+6 tiles with no bed, or at
  most L+1 tiles with a bed (she can't grow past tiles-1 and keep moving; stepping into her own
  tail kills). Schooltime's 2x2 pen has a bed under the queen, so the proposal's "no bed" clause
  alone never fired there; the L+1 clause is the generalisation. Latched (kelp never changes).
- Relayed on bit 30 of the champion word (length field cut to 11 bits).
- While sealed: no voluntary queen split (ring bootstrap/pearl split, settle/forage/founder/shed,
  queen11 wait); the life-saving escape split and boxed legal split stay. Champion() skips her
  and ids 0/1; UpdateLateChampion doesn't wait for her and starts die-near and late conc at r380.
- Field mode unchanged.

## Local (build with -DSEALED_DBG for SEALED / SEALAWARE lines)
- Detection: Schooltime r0 by the queen herself (4 tiles, bed), mg40506 (12 tiles), sealed-spawn
  exit0 pens (12 and 15 tiles). Team awareness at r380 on Schooltime: 222 of 223 dragon-turns.
  44 fresh mapgen maps (seed 52010, all families): 0 detections, 88-88 identical to the base.
- v ring-field-freecost, seeds 1-4: Schooltime 7-1 (our longest at r500 ~40 v ~21), mg40506 3-5
  (base v rf1 4-4; when our army survives, longest 26-56 v base 8), sealed-spawn-005 7-1,
  -009 4-4, -012 4-4 (mirror). Total 25-15, all decided on longest.
- Duels v ring-field-18 (eph-4dfdd8b8): server random:2 35-33 (longest dragon 8-6, all else
  identical); official random:2 10-10, identical.

## Arena
Submit was blocked by the permission system; handed to the orchestrator.

# ring-field-freecost (g52-5)

- **Line:** ring-field (parent arena `ring-field-3` = variants/ring-field-1).
- **Brief:** league/briefs/g52-5.md: fix the 1.2.3 move-cost rule in the lookahead models.

## Idea
unswbc 1.2.3 MOVE: a dragon of length L (fixed at the start of the move) takes its first ceil(L/4)
steps free; each later step costs a tail segment and needs length > 2. So the longest move is
ceil(L/4) + L - 2 steps. The rollout judge and the search7 lookahead still used the old rule
(every step after the first pays; reach L-1), so the judge's rollouts charged our free sprints and
lunges segments they don't cost, and the lookahead under-estimated how far an enemy head can lunge.

## Change (the rule only)
- rollout_giant2.hpp: FreeSteps(L) = ceil(L/4), MoveReach(L) = FreeSteps(L) + max(0, L-2).
  Rollout(): the candidate's step k pays only when k >= FreeSteps(L_) (was k > 0). Policy lunge:
  pays only k >= FreeSteps(L). LungePath reach min(MoveReach(L), 4) (was min(L-1, 4): same for
  L >= 3). Judge Lunge() reach MoveReach(L_) (was L_-1).
- search7.cpp: same helpers. BuildThreat enemy reach MoveReach(e.len) (was e.len-1); ReplyCost
  min(MoveReach(e.len), 8) (was min(e.len-1, 8)); Trade() reach MoveReach(len_) and the length at
  impact len_ - max(0, steps - FreeSteps(len_)) (was len_ - dx).
- strategy.cpp Lunge(): reach min(ceil(L/4)+L-2, lunge_reach=6) (was min(L-1, 6)).
- Already on the 1.2.3 rule, untouched: ring.cpp, rescue.cpp, queen_attack.cpp, champfood.cpp,
  rush.cpp, strategy.cpp lunge field / sprint search. rollout.hpp is not compiled (dead copy).
- Debug only (#ifdef JUDGE_DBG, plugin.cpp + Judge::Reviewed()): counts of judge reviews,
  overrides, gst multi-step moves reviewed and cut.

Reach examples (enemy lunge reach, old -> new): L=2 1->1, L=3 2->2, L=4 3->3, L=5 4->5,
L=8 7->8, L=10 9->11, L=16 15->18. Free steps: L 2-4 1, 5-8 2, 9-12 3 (docs/game.md).

## Local
- Judge on gst's multi-step moves (server maps, seed 1, vs ring-field-1, JUDGE_DBG build):
  old rule 5747 reviewed, 3263 cut (57%); new rule 5221 reviewed, 1112 cut (21%). All reviews
  214849 -> 213993, overrides 22579 -> 20650; judge lunges taken 79 -> 82.
- Server maps seed 1: 17-0-17. Fresh mapgen seed 52508, 44 maps x 2 seeds: 84-0-92; per family
  open 9-7, rich-near-home 7-9, maze 8-8, rooms 6-10, sealed-spawn 8-8, ring 8-8, off-axis 7-9,
  sparse 8-8, crowded 8-8, anti-centre 8-8, mixed 7-9 (none a clear loss).
- Arena duels vs ring-field-3: server random:2 32-0-36 (3+ starters 18-22, <=2 14-14);
  official random:2 11-0-9. CPU max 47.9M.

## Arena verdict (ring-field-18)
- vs parent ring-field-3: 56-0-56 (50.0%, within noise); server suite 33-0-35, official 5-0-5
  (rotating none). Decided by elimination 27-26, queen 18-18, longest 10-11, total 1-1.
- vs ring-qforage-1 58-54, ring-sprint-21 51-61, spar-field-2 57-55. Whole draft 222-0-226.
- eval P(win) vs parent: r100 0.503, r200 0.505, r300 0.507 (server 0.499/0.498/0.502).
- CPU: worst p99 24.7M, max 47.4M.
- Replays (24 server games vs parent, decoded): non-queen moves at L>=5 with another head in the
  7x7 window (where the judge reviews): free sprints 17810/55671 = 0.320 per turn v parent
  16644/56652 = 0.294 (+9%); paid moves 486 v 470; deaths per such turn 0.0029 v 0.0030. With an
  enemy head in view only: free sprints 185 v 214 (rare either way), deaths 883 v 912.
  The judge now leaves more free sprints alone, as intended; no W-L effect.

---

# ring-field-1 (g46-1)

- **Line:** ring-field (arena draft `ring-field-1`, parent ring-sprint-21 = variants/ring-sprint-guard2).
- **Brief:** league/briefs/g46-1.md: strategist g46 A S1 / B #1, the committed field-first opening.

## Idea
Server losses open between r20 and r40 (eaten 8.5 v 14.4 at r30). Local play is the same; the
difference is where the heads stand (starters 7.1 v 10.7 tiles from spawn at r20). Earlier fixes were
ChooseGoal score terms that nearby home food outbid, so this mode owns the goal outright.

## Change (field.cpp, plus hooks in strategy.cpp Decide step 5 and the header)
- **Who:** every non-queen dragon alive in round 0: the starters and the children they split off in
  r0. Excluded: the queen's own r0 child (its tail touches hers), and a dragon that already sees 4
  live or soon-due beds at spawn.
- **Heading:** its facing at r0. A starter faces into the board; an r0 child faces the other way
  along the same line. Each turn it takes the reachable frontier with the most progress along the
  heading per step of known path, then the most unseen ground. Unknown portal edges count as
  frontier. Frontier an ally head is nearer to scores x0.25. The heading is held, never re-scored.
- **On the way:** it eats a pearl in view only if it costs at most 2 extra steps (one step off the
  path). FreeSprint takes the free extra steps at length 5+.
- **Arrive:** 4 pearls lying in view, less one per ally head there, or 2 after crossing a portal.
  The normal agent then takes over, splits there, and its cluster call tells the team.
- **Exit:** on arrival, at r35, or when no frontier makes progress on the heading. In that last case
  it first walks to a fresh cluster call if one exists. Kept off for a dragon that saw no new tile
  for 6 rounds.
- Children of splits made on the way are ordinary agents and farm where they are born.
- Replaces the g40-5 two-explorer rush (`rush_state_` 1 never set) and, for mode dragons, the founder.
- `FIELD_MODE 0` gives the parent's behaviour exactly (checked on around_unsw: identical games).
- `FIELD_DBG` prints mode transitions and per-turn reasons before r45.

### Iterations (local, sim)
1. Heading re-weighed each turn: a Devil starter stopped in a slow-bed lane 4 tiles short of the
   centre (arrival counted soon-due slow beds), then swung north up a lane.
   - Arrival changed to count pearls lying in view.
   - The heading is now held (it starts as the dragon's facing).
2. A second heading (by unseen ground) only made the r0 children facing the board-edge kelp tour
   their spawn room until r35. They now exit at once and farm home.
3. Spawn threshold: 3 beds (the brief's number) fired by chance on Trophy's slow beds (gap up to
   1000), so it is 4.
4. Tried and dropped: an exit after 15 rounds without eating (around_unsw 72 v 98 total at r50,
   worse than without it).

## Local
- `check ring-field-1 maps/official`: passed.
- devil+trophy, seeds 1-2 v ring-sprint-guard2: 7-1. The mode fires for every non-queen starter and
  its r0 child.
  - Devil: starters reach the central field (12,8)/(12,9) by r10-12.
  - Trophy: the starter crosses the bottom portal into the cup at r10.
- Server maps seed 7: 14-20 (opening totals equal to the base).
- Fresh mapgen (seed 46101, 44 maps, seeds 1-2): 92-84 overall. Per family: anti-centre 8-8,
  crowded 9-7, maze 8-8, mixed 9-7, off-axis 11-5, open 9-7, rich-near-home 7-9, ring 9-7,
  rooms 7-9, sealed-spawn 4-12, sparse 11-5.
- **Sealed-spawn is a clear loss** (4-12; seeds 3-5: 7-17).
  - At r50: 92 v 108 total. Base self-play is 100 v 100, so we eat less and the opponent eats more.
  - These are sparse maps with no 4-pearl field. Mode dragons walk to r35 eating only pearls on
    their path, and the queen is left alone.
  - Tried, no help: an exit at r20 (9-15), arrival at 3 pearls (9-15). Not in the submitted code.
  - Bug found after submitting: rush.cpp's g40-5 explorer is off for every dragon, including those
    the mode leaves out (spawn field, the queen's child). The arena draft has it. Restoring it for
    non-mode dragons changed sealed-spawn by nothing (7-17), so the committed code equals the draft.

## Arena
- **Light duels vs ring-sprint-21 (eph-b60dded6):**
  - server: 29-39 (devil 4-0, trophy 4-0, around_unsw 0-4, slithery 0-4);
  - official: 8-12;
  - board control@200 .55 v .43.
- **Verdict:** 55-0-57 vs parent (49.1%, noise): server 33-35, custom 17-17, official 5-5.
  - Against the others: ring-qforage-1 52-1-59, ring-contest-4 62-50, ring-sched-17 71-41.
  - CPU p99 25.8M, max 41.9M.
- **By map (all opponents):** devil 16-0, trophy 12-4, schooltime 13-3; weakhold 3-13,
  tower_defense 4-12, custom fixed-edges 0-8, fixed-portal-larder 1-7, fixed-honeypot 1-7.
- **Targeted situation, measured on the draft's own Devil/Trophy games (32 games, all opponents):**
  - startdir r20 (mean head distance from own spawn): devil 9.9 v 6.7, trophy 10.6 v 9.6;
  - bedprox (beds within 3 of a head): devil r40 9.7 v 7.0, trophy r40 12.3 v 8.2;
  - eaten by r30: devil 21.0 v 11.2, trophy 13.2 v 10.1;
  - dragons at r40: devil 17.6 v 10.2, trophy 12.2 v 7.8;
  - board control r30: devil .57, trophy .46 (r20 .56).
- **Eval vs parent:** P(win) r100 .544, r200 .466, r300 .443. The opening edge fades, and losses are
  mostly longer-queen games on maps outside the target.
- **Cross-lineage duel** (server, random:2): ring-field-1 v cs-queen-split-tail-risk-v1-7 53-15.
- **Next:** the mode costs where no dense field exists (sealed-spawn/sparse, weakhold, tower_defense).
  It needs a "no field on this heading" test better than r35, e.g. comparing pearls in view with
  the density already seen. Also judge it against the g46-6 field-style sparring bot, since the
  arena pool never contests the fields.

## Draft 2 (arena ring-field-2, same parent): fix for starts with no field
The coordinator asked for two fixes after draft 1: an early "no field on this heading" exit, and the
old rush restored for dragons the mode leaves out.

### Changes
1. **Barren-heading exit (field.cpp).** Each tile the walk uncovers counts once, with the pearl it
   shows when first seen. The test runs once the walk has seen two 7x7 windows of new ground, or at
   r12, whichever comes first. The mode ends when any of these holds:
   - the new ground holds no pearl;
   - it holds pearls no denser than the spawn view;
   - the walk has seen less than one window of new ground;
   - the head is less than half a tile from spawn per round walked (circling).
   Why r12: the top teams' starters are on the clusters by r10-12. Why half a tile a round: their
   starters are 10.7 tiles from spawn at r20.
2. **Rush restored.** Dragons the mode leaves out (a spawn field, the queen's child) explore as in
   the parent. A mode dragon that would have been a g40-5 explorer (id q+2/q+4) becomes one when the
   mode ends without arriving (FieldExit). This was the weakhold loss: in the parent, the explorer
   reaches the centre field (17,7) by r44 and the team has 19 dragons at r100. In draft 1, the
   dragon back in normal play followed home calls inside the spawn pocket and the team had 6.
3. **Founder resumes after the mode** (it waited instead of being cancelled). Neutral in sim.

### Local (v ring-sprint-guard2)
- weakhold: 6-6 over seeds 1-6. Draft 1 went 2-18 over seeds 1-10; with the exit alone it was still
  2-10, and the explorer hand-over fixed it.
- tower_defense: 6-6.
- Fresh sealed-spawn (4 maps, seeds 1-6): 25-23. Draft 1 went 7-17.
- devil+trophy (seeds 1-6): 20-4.
- Fresh mapgen (seed 46102, 44 maps, seeds 1-2): 85-91. At r50 total length is 67.5 v 68.5.
- `check` passed.
- The custom fixed-* trick maps aren't in this repo's mapgen, so they are judged from the arena.

### Light duel v ring-sprint-21 (eph-ddd50d02, server, random:2)
- 39-29 overall. By map: devil 4-0, trophy 4-0, weakhold 2-2, tower_defense 3-1, around_unsw 1-3,
  maze 1-3.
- dragons@100 26.5 v 22.8; queen alive at the end 60% v 46%.

### Arena verdict (ring-field-2)
- **vs parent ring-sprint-21:** 54-0-58 (48.2%, noise).
  - By suite: server 33-35, custom 15-19, official 6-4.
  - Decided by: elimination 26-27, longer queen 13-22, longest dragon 15-9.
- **Others:** ring-qforage-1 55-57, ring-contest-4 66-46, ring-sched-17 68-44.
- **CPU:** p99 25.8M, max 44.4M.
- **Eval P(win) vs parent:** r100 .540, r200 .520, r300 .509 (draft 1: .544/.466/.443). The late fade
  is gone.
- **By map, all opponents, draft 1 -> draft 2:**
  - targets kept: devil 16-0 -> 16-0, trophy 12-4 -> 12-4;
  - recovered: weakhold 3-13 -> 9-7, tower_defense 4-12 -> 8-8, fixed-portal-larder 1-7 -> 3-5;
  - still lost: fixed-edges 0-8 -> 0-8, fixed-honeypot 1-7 -> 0-8, fixed-pen 6-2 -> 0-8,
    fixed-rooms 2-6, fixed-split 2-6, around_unsw 4-12.
  - The parent's own records there: edges 77%, honeypot 81%, pen 71%, rooms 71%, split 67%.
- **Draft 2's own Devil/Trophy games:** identical to draft 1 (the barren rule never fires there).
  - startdir r20: devil 10.0 v 6.1, trophy 10.1 v 9.6;
  - bedprox r40: devil 9.7 v 7.0, trophy 12.3 v 8.2;
  - eaten by r30: 21.0 v 11.2 and 13.2 v 10.1;
  - dragons at r40: 17.6 v 10.2 and 12.2 v 7.8.
- **Weakhold games:** eaten by r30 7.5 v 10.1, dragons at r40 5.2 v 5.8. Tower_defense: even.

### Custom fixed-* maps (extracted from arena replays, sim v ring-sprint-guard2, seeds 1-12)
- Draft 2: edges 6-18, honeypot 6-18, pen 8-16. With FIELD_MODE 0 each is 6-6 per 6 seeds.
- Edges: the deficit is material. At r50 the total is 31 v 53, and our dragon count falls between
  r10 and r20 (mode dragons traded off in the open middle). The queen died in all 12 games.
- Tried, not kept (no improvement there, or Devil broke):
  - judging at r7 / one window: edges 6-6, but the Devil starters exit at (7,9) just short of the
    field, so devil+trophy 16-8;
  - food density by BedWeight plus an early 'poorer than home' exit: edges 0-12, honeypot 1-11;
  - spawn-field threshold 2: edges 4-8, honeypot 1-11;
  - no barren exit: pen 8-4, but edges 0-12;
  - no explorer hand-over: weakhold 0-12.
- Next: these are open or sparse maps with food scattered round the spawns. Sending every non-queen
  r0 dragon out leaves the home food and the queen's surroundings to the opponent. A candidate is
  keeping one r0 dragon per spawn group at home when its spawn view holds fast beds.

## Draft 3 (g47-1, arena ring-field-3): the home keeper
- **Change:** a round-0 child (its tail touches its parent's) whose spawn view holds 2+ live or
  soon-due beds stays home while its parent walks. FIELD_KEEPER / FIELD_HOMEFOOD.
  `FIELD_UNTIL` is a macro for ablations.
- **Films** (render.py --film, ring-field-2 v parent, arena):
  - Devil 218024: cyan 3 and 5 are on the central field at r16-24; 20 v 11 dragons at r40.
  - Trophy 218220: cyan 3 crosses the bottom portal into the cup by r8-16; 14 v 9 at r40.
  - Edges 217918: cyan 8 sat in the empty middle r0-24. Orange held the food band on the wrap
    edges, and from r48 grew 17 -> 34 -> 56 dragons (r48/r80/r112) while we stayed at 10.
  - Honeypot 217945: the food is scattered round the outside and both sides stay small.
  - Pen 217904: no pearls at all before r40 (beds of gap 3849), 5 v 5 dragons all opening.
- **Local, v ring-sprint-guard2 over 12 seeds (24 games per map):**

  | Variant | Edges | Honeypot | Pen | Devil+Trophy |
  |---|---|---|---|---|
  | keeper, 2 beds (submitted) | 6-18 | 6-18 | 8-16 | 42-6 |
  | keeper, 1 bed | 6-18 | 6-18 | 8-16 | 42-6 |
  | no keeper (draft 2) | 6-18 | 6-18 | 8-16 | 42-6 |
  | lowest-id group keeper | 4-20 | 6-18 | 8-16 | 39-9 |

  The 1-bed keeper fires 70 times over the 24 edges games and the 2-bed keeper about 2 per game.
  Neither changes a single result.
- **Ablation on how long the mode lasts:**
  - mode ends at r0 (`FIELD_UNTIL` 0, so it never moves anyone): edges 12-12, honeypot 12-12
    (parity);
  - mode ends after r0's move (1): edges 9-15, honeypot 6-18, pen 10-13;
  - ends at r6: edges 7-17, honeypot 6-18.
  - One different first move already gives the full loss on honeypot. On these sparse maps
    the sim result follows chaotic divergence from the first move, not the walk. In sim, these
    maps can't confirm or reject a fix against the parent.

### ring-field-3 arena verdict
- **vs parent ring-sprint-21:** 58-0-54 (51.8%, noise); beats_parent is true.
  - By suite: server 35-33, custom 17-17, official 6-4.
  - Decided by: elimination 26-28, longer queen 15-21, longest dragon 17-5.
- **Others:** spar-field-2 60-52, ring-qforage-1 57-55, ring-contest-4 64-48.
- **CPU:** p99 25.8M, max 42.0M.
- **Eval P(win) vs parent:** r100 .539, r200 .517, r300 .477.
- **By map, all opponents:**
  - devil 16-0, trophy 12-4, tower_defense 10-6, around_unsw 9-7, weakhold 7-9;
  - fixed-edges 3-5, fixed-split 4-4, fixed-honeypot 1-7, fixed-pen 1-7, fixed-rooms 2-6,
    fixed-portal-larder 2-6.
- **Light duel v spar-field-2** (eph-1893880c, server random:2): 35-33.
  - By map: devil 3-1, trophy 3-1, tower_defense 4-0, portals 4-0; default 0-4,
    prisoners_dilemma 0-4, stripes 0-4.
  - Opening numbers in that duel (us v spar-field-2):
    - Devil (4 games): startdir r20 11.3 v 5.9, eaten r30 17.0 v 7.2, dragons r40 14.5 v 8.5;
    - Trophy (3 replays): startdir r20 7.3 v 7.3, eaten r30 9.0 v 8.3, dragons r40 10.7 v 9.0.
  - Parent v spar-field-2 (arena games, 4 each):
    - Devil: startdir 6.2 v 6.7, eaten 10.8 v 9.8, dragons 12.0 v 8.2;
    - Trophy: startdir 9.6 v 9.2, eaten 10.0 v 13.2, dragons 8.5 v 12.0.
