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
