# ring-field-qdead3 (g52-16): infer the enemy queen's death from death pearls (detection)

- **Line:** ring-field, parent arena `ring-field-26` (variants/ring-field-qdead2).
- **Brief:** ../syeyoung-tokens.league/briefs/g52-16.md (iteration 3, last, of the qdead idea: detection only).

## What the traces showed (sim, QDEAD_DBG, server maps seeds 1-2, v ring-field-qdead2)
- Per-dragon head positions and queen sightings (QPOS) and the queen12 target (QATK) logged, matched
  to the sim's truth. 18 games where their queen died first (game went on 10+): all 16 kills by us
  were queen12 head-on lunges; the attacker dies with her.
- In 6 of the 16, other allies saw her body the round before. Example default s1 r262 (attack id101
  at (29,7)): id96 saw 2 segments of her at r261-262, the kill tile was in its view at r263 with a
  new pearl on it, yet no mark. Cause: the kill tile was a **bed** (countdown 1413) and the g52-6
  pearl rule counted only pearls off beds. Death pearls land on any tile, bed or not.
- In the other 10, the attacker was the only dragon of ours within 3 of her at R-1, R and R+1
  (autarky, devil, maze, queen_of_spades, trophy s2, unsw, weakhold x2, islands, australia s1).
  The attacker had seen her the round before in only 2 of them, and no ally was near enough to see it
  die, so neither an announcement nor watching the attacker die can reach anyone. Out of reach for any
  inference; I did not change the attack.

## Change (detection only, strategy.cpp Remember)
- **Death pearl on a bed:** a new pearl counts as a death pearl also on a bed whose countdown we read
  last round as >= 2. A bed fires at the round start when its countdown reaches 0, so it read 1 the
  round before (game.cpp SetCountdown); any other new pearl on a bed came from a death. Feeds the
  existing rule (2) (new pearl within her one-move reach from her last seen head).
- **Body pearl (rule c):** the enemy queen, any segment of whom (head or not) was in view last round
  and none now, is dead when a new death pearl lies on a tile her body covered last round, or beside
  her last head. A living dragon leaves no pearl behind; a dead one leaves one on every second segment
  from the head, so after a move of s steps her segments 2k >= s lie on her old tiles. Covers
  witnesses that saw only part of her (no head), which (a)/(2) need.
- Delivery, aim, messenger and fortress unchanged. Debug-only: QPOS/QATK prints under QDEAD_DBG.

## Local (sim, 2 threads, traced, both sides measured in the same games)
- Server maps seeds 1-2 (68 games): their queen died first in 18 per side. Detected within 5 rounds:
  new 6/18 v base 4/18; our queen learned within 10: 4 v 2. Recovered: default s1 (r263, learned r266)
  and default s2 (r71, learned r74), both by the bed fix. 0 false marks (QDEAD/QADJ/QLEARN before
  her real death or with her alive) on either side.
- Fresh mapgen seed 16160 (44 maps, seeds 1-2, 176 games): eligible 58 per side; detected <= 5
  rounds new 33 v base 29; learned <= 10 new 24 v 21; 0 false marks. Rule (c) fired 12 times, all true.
- W-L v ring-field-qdead2: server 35-33 (australia s2 won both seats); fresh 87-89, per family 8-8
  except rooms 7-9 (rooms-014 s2: our queen learned at r172 and played fortress, died r269 v the
  base's unaware queen at r398, both queens dead, lost on longest: a fortress outcome, not detection).
- `check ring-field-qdead3 maps/official` passed.
- Arena: `tokenz ephemeral` timed out from this sandbox (as for g52-11); no duels, not submitted.
  Bot dir for the orchestrator: built by `scripts/make-bot.sh variants/ring-field-qdead3 <out>`.
- Detection ceiling: the remaining misses are kills with no witness but the dying attacker.

---

# ring-field-qdead2 (g52-11): deliver the enemy-queen-death news to our queen

- **Line:** ring-field, parent arena `ring-field-22` (variants/ring-field-qdead).
- **Brief:** ../syeyoung-tokens.league/briefs/g52-11.md (iteration 2 of g52-6: fix the delivery path only).

## What the traces showed (sim, QDEAD_DBG, v ring-field-freecost)
- Server maps, seeds 1-2 (68 games): 18 eligible games (their queen died first, game went on 10+).
  A dragon of ours detected it within 10 rounds in only 5; our queen learned in 3. In 25 of 25
  kills of their queen by us, the killer was a `queen12: direct attack` that died in the trade.
- **The attacker can't announce.** A dead dragon casts no ping, so it would have to say it a turn
  early. Killers' TryQueenAttack distances one turn before the kill were 1-8 (7 of 25 had not seen
  her at all the turn before), and sightings at distance <= 3 were followed by a kill in ~8% of
  cases. A strict "every move she has is in our next-turn reach" claim fired once in 68 games, and
  that was false (0 of 25 kills foreseen). Dropped (not in the code).
- **Delivery where detected:** the news sat with 1-10 dragons 20-40 tiles from her (tower_defense s2:
  one knower circling at (5,6), our 2-long queen in her 2x2 ring at (23,9), she heard 0-1 words a
  round). The g52-6 aim only fired when exactly on her row or column and ignored kelp.
- **False marks in the base:** the old g42-4 adjacent-witness rule (enemy queen beside our head last
  round, no segment of her now) fired on a *partial* sighting (1 segment seen, e.g. her body across
  a portal). Fresh maps: our queen went fortress while their queen lived in 8 of 176 base games
  (QLEARN before the death; the previous count looked only at the new detector). Delivery spreads
  these too (sparse-007: false fortress at r55 instead of r180), so it has to be closed.

## Change (delivery only)
- **Messenger** (strategy.cpp `MessengerGoal`, `Messenger`): for kMsgFor = 12 rounds after learning,
  a worker that knows walks (goal step 5, label `qdead: messenger`) to the nearest reachable tile 2-8
  steps out from one of her tiles on a known-clear straight line (kelp and unseen edges end a line,
  portals followed, a dragon in view on it blocks), never beside her. Her tiles: her body in view,
  else her ring (fresh), else the last ring we heard of (`qd_home_`, <= 100 rounds old), else her
  report. With no such tile on a known path: an optimistic BFS (unseen edges open) to those tiles,
  walked as far as known paths match.
- **Aim** (`AimQueenDead`): the champion word (bit 9) goes on any non-back direction whose known-clear
  line reaches her tile (`QDeadLine`), not only her raw row/column.
- **Ack:** a worker stops when it hears she knows: her own ring word (stamp = last round) carrying the
  dead round, or a ring word with bit 55 set (RingWord sets it when the sender is the queen and knows,
  or has heard she knows; bit 55 is the call's top bit, used only with bits 48-51 nonzero, calls end r48).
- **False-mark guard:** the g42-4 adjacent-witness rule now needs her whole body seen last round
  (`!partial`). (Strictly a detection edit; needed because delivery amplifies false marks.)

## Local (sim, 2 threads)
- Fresh mapgen seed 521101 (44 maps x 2 seeds x 2 seats), traced v ring-field-freecost:
  - base: eligible 55, detected <= 10 rounds 23, queen learned <= 10: 18 (78% of detected, 33% of
    eligible); learn lag after first detection 0-4 in 13, then 8,8,8,12,16,31,141; 8 false queen learns.
  - new: eligible 56, detected 24, learned 20 (83% / 36%); lags 0-4 in 16, 8,8,8,8; 0 false learns,
    0 false detections. Faster in the same games: anti-centre-031 s1 r40 v r50, maze-024 s1 r79 v
    r102, sealed-spawn-015 s2 r185 v never, rich-near-home-012 s2 r24 v never (an aimed ping at r23
    from the detector itself, id 11, then the queen still died at r26 to a lunge).
  - Messenger turns: 12.7k over 41 games; 2/3 of them after the queen already knew (the ack only
    reaches ring-word listeners). With a 40-round window it was 32k: hence 12.
- Server maps seeds 1-2 traced: eligible 18, detected 4, learned 2 (base 5/3): the misses are
  undetected kills; messenger active 113 turns in all.
- **The brief's 70% target is not reachable by delivery**: detection is the gap (24 of 56 eligible
  fresh, 4-5 of 18 server), because the killer is the only witness and dies.
- Plain sim v ring-field-qdead: fresh 86-0-90 (anti-centre 8-8, crowded 8-8, maze 8-8, mixed 7-9,
  off-axis 9-7, open 8-8, rich-near-home 7-9, ring 8-8, rooms 7-9, sealed-spawn 8-8, sparse 8-8);
  server seed 1 17-0-17 (identical games). `check maps/official` passed.
- Arena: `tokenz` timed out from this sandbox (ephemeral upload); not submitted, no duels.

---

# ring-field-qdead (g52-6, arena ring-field-21, try 2 = ring-field-22)

- **Line:** ring-field, parent arena `ring-field-18` (variants/ring-field-freecost).
- **Brief:** ../syeyoung-tokens.league/briefs/g52-6.md = strategist B g52 proposal #1.

## Idea
arena-r26 S1: in 24% of losses our queen dies after theirs. `enemy_queen_dead_` was set only by an
adjacent witness (usually the killer, which dies with her) and never relayed, so our queen never
switched to pure survival. Once their queen is dead, a living queen of any length wins, so growth
is worthless and the queen should play a fortress.

## Change
- (a) Detection (Remember): a dragon that saw her head last round and sees no segment of her now
  marks her dead when (1) a BFS from her last head through known edges out to her rule reach
  ceil(L/4)+L-2(+1) stays inside our view (body fully seen), or (2) a pearl that is new since last
  round, on a non-bed tile, lies within that one-move reach (only a death leaves a pearl off a bed).
  The adjacent-witness rule is kept.
- (b) Relay: champion word bit 9 = dead; ring word bit 35 clear with bits 26-34 = dead round.
  `HearQueenDead` sets it; a later sighting of her overrides (`EnemyQueenDead()`). A dragon that
  knows aims one ping along the row/column of our queen (head in view, report, or ring) for 40 rounds.
- (c) Fortress queen (`Fortress()`): no ring pearl split at length <= 7 (from 8 she sheds to 5);
  beam food 0 and pearl gain only at QueenRisk 0; QueenRisk +1 reach (risk floor 1) and enemy bodies
  seen without a head as heads with reach 6; -2 per out-of-view tile within Chebyshev 2 of the
  endpoint; +2 when an ally head is within 3. Labels "queen: fortress" / "ring: fortress shed".
- Try 2 (after ring-field-21's replays): the fortress queen keeps the food term while shorter than
  kFortressKeep (251973: a 2-3-long fortress queen never ate for 200 rounds, met head-on), and loses
  the home-distance pull (251795: with food at 0 it walked her 20 tiles home into a sprinting hunter).

## Local (sim, 2 threads)
- vs ring-field-freecost: server maps seeds 1,2 34-0-34; fresh mapgen (seed 526061, 44 maps) 87-4-85;
  per family anti-centre 9-7, crowded 10-6, maze 6-4-6, mixed 8-8, off-axis 8-8, open 8-8,
  rich-near-home 8-8, ring 7-9, rooms 8-8, sealed-spawn 8-8, sparse 7-9. Fortress queen moves: 54 in 176 games.
- Detection, traced server games (68): 0 false dead marks against the trace's truth. Of 18 games
  where their queen died first and the game went on 10+ rounds, a dragon of ours detected it within
  10 rounds in 5 and our queen learned it within 10 in 3. Misses: the queen12 killer was the only
  dragon that saw her (it dies in the trade and a dead dragon casts no ping); and our queen hears
  few words at all (australia s2: 136 dragons knew, she got 0 words in most rounds). The brief's 70%
  target is not met: the witness is usually the killer.
- Arena duels v ring-field-18: server 34-0-34, official 10-0-10 (identical metrics: rarely triggers).

## Arena
- ring-field-21 (try 1): 56-0-56 v ring-field-18 (server maps 34-34); v ring-field-3 55-57,
  ring-qforage-1 58-54, ring-sprint-21 52-60. Suites: server 141-131, custom 65-71, official 15-25.
  Paired with the parent's games on the same map/seed/seat/opponent (336): only 10 differ, 2 flips
  (default v rs21 won: queen survived; autarky v rf3 lost: fortress queen walked home and died).
- ring-field-22 (try 2): 57-0-55 v ring-field-18, beats_parent True (noise); server suite 35-33;
  v ring-field-3 56-56, ring-qforage-1 58-54, ring-sprint-21 52-60. Suites: server 143-129,
  custom 65-71, official 15-25. CPU p99 24.7M, max 47.4M.
  Paired (336): 8 differ, 1 flip (default v rs21, 252301: their queen died r143, a dragon of ours saw
  it that round, our queen heard it at r193, survived and won 2-0 on queens; the parent lost it).
  Their queen first and ours alive at the end: 76 v 76; S1 losses 39 v 41; queen died later than the
  parent's in 252445 (r335 v r220, heard 4 rounds after the death), earlier in 252463 (r435 v alive).
  Eval v parent r100/200/300: 0.500/0.500/0.505.
- Conclusion: the fortress rarely triggers. Detection needs a witness other than the killer (in
  sim, the queen12 killer was usually the only dragon that saw her), and our queen hears few sonar
  words (learning took 4-50 rounds in the arena games above). The S1 weak spot is barely touched.

---

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
