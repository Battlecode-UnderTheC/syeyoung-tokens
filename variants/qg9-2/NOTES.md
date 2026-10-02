# qg9-2 (not submitted: neutral in sim)

qg9-1 plus an heir hedge: from the feed round the non-queen dragons consolidate into the longest
non-queen dragon instead of the queen (the queen keeps its pasture all game). Motive: in
queen-guard-1's arena games, when our queen dies after r300 (23 of 96 games) we win 8; nobody had
consolidated while it lived.

Sim, maps/server seeds 1-16 vs gst-search-8: 166-2-152, identical to qg9-1 (longest-decided 65-71
vs 62-76). Feeding counts are the same (~23 feeds a game): in sim our queen is usually dead by the
feed round, so the hedge rarely fires. Would need arena games to judge.

# qg9-1

queen-guard-1 (gst-queen-guard-3) with the queen's early splits restored: before round 100 an
economy split may leave the queen at 2 (gst-queen-guard-2's minimum); from r100 the minimum is 3.

## Why
Arena metrics: queen-guard-1 trails its opponents in dragons and pearls from r50 (14.3 v 16.5
dragons at r50, pearls eaten 113 v 127 at r100), while queen-guard-2 is even. Its two changes vs
queen-guard-2, ablated in sim (maps/server, seeds 1-8, vs gst-search-8, 160 games each):

| build | W-D-L | r100 dragons (us v them) | our queen died |
|---|---|---|---|
| queen-guard-1 | 85-1-74 | 19.3 v 22.5 | 78% |
| pasture off | 80-0-80 | 19.0 v 22.8 | 81% |
| queen min 2 (pasture on) | 89-1-70 | 20.3 v 21.3 | 83% |
| queen-guard-2 | 88-0-72 | 20.5 v 21.2 | 82% |

The pasture costs nothing measurable; the queen min length 3 is the whole economy cost (the
queen's early splits compound).

## Local (seeds 1-16, 320 games vs gst-search-8)
- qg9-1: 166-2-152, r100 dragons 20.3 v 21.3/21.7, queen died 81.5%, queen-decided 38-0.
- queen-guard-1: 158-1-161, r100 dragons 19.3/17.8 v 22.5/23.5, queen died 79.5%, queen-decided 40-1.

# gst-queen-guard-3

gst-queen-guard-2 with early queen growth:
- **The queen's pasture all game.** While our queen lives, every other dragon leaves the pearls
  within kReserve (4) of the queen's head to it. In gst-queen-2 this applied only from the feed
  round.
- **The queen's economy splits stop at length 3** (kQueenMinLen 3; gst-queen-guard-2 used 2).

Escorts were tried and dropped: allies trading into any enemy head within 4 of our queen made no
measurable difference (52-48, the same as the pasture alone). The trade rarely has a path.

## Local sim (1.2.3, FP-faithful)
**vs gst-queen-guard-2, maps/server (10 maps), seeds 1-6, both seats:** 59-0-61.
- queen-decided games: 9-4;
- r100 queen length: 2.1 v 1.5;
- r100 dragons: 18.7 v 21.9.

**How our queen ends** (vs gst-queen-2, the 7 queen maps, seeds 1-5, 70 games):

| | alive | enemy head-on | trapped "no move" | own dragon | body |
|---|---|---|---|---|---|
| gst-queen-guard-2 | 17 | 18 | 25 | 6 | 4 |
| gst-queen-guard-3 | 24 | 23 | 15 | 8 | 0 |


## gst-queen-guard-2's notes

# gst-queen-guard-2

gst-queen-2 (b8918f7 on task/arena-search: the heir champion) plus gst-queen-guard-1's layer:
- the queen guard (`queen_guard.hpp`), here with the queen allowed to split down to 2
  (kQueenMinLen 2, the best W-L in gst-queen-guard-1's ablation);
- allies yield: non-queen dragons don't step beside our queen's head;
- body_ rebuilt from the segments in view after a multi-step move (from gst-sprint-1,
  task/g26-2): the guard's queen sprints are multi-step.

Built with -ffp-contract=off (main's CMake). No map keys, no environment variables, no mutable
static state (the gst counters are debug-only).

## Measured: judge sandbox (unswbc 1.2.3), all 10 server maps, both seats, vs gst-queen-2
| build | seeds | W-D-L | our queen alive at r500 | theirs |
|---|---|---|---|---|
| 6d2ecd7 (no body rebuild) | 51-55 | 59-0-41 | 23/100 | 0/100 |
| 28b4d69 (body rebuild) | 56-60 | 54-0-46 | 19/100 | 1/100 |
| together | 51-60 | 113-0-87 (56.5%) | 42/200 (30% of the 140 queen-map games) | 1/200 |

Per map, seeds 51-60 (W-L, our queen alive):

| map | W-L | queen alive |
|---|---|---|
| autarky (doomed) | 9-11 | 0/20 |
| default | 9-11 | 2/20 |
| devil | 10-10 | 2/20 |
| dilemma (doomed) | 10-10 | 0/20 |
| portals | 12-8 | 0/20 |
| queen_of_spades | 15-5 | 9/20 |
| schooltime | 10-10 | 7/20 |
| slithery_fight (doomed) | 10-10 | 0/20 |
| trauma | 17-3 | 14/20 |
| trophy | 11-9 | 8/20 |


## gst-queen-2's notes

# gst-queen-2

## Idea
gst-queen-1 (v12) with an heir champion. Champion() returns our queen while she is heard of
(her report is at most kQueenStale = 60 rounds old, and not an obituary). Otherwise it returns
gst's longest-dragon election, with the queen's id excluded from it. Feeding ignores the length
test only when the champion is the queen. This is strategist z3's P1, built independently.

Why: queens die early on most maps (sim 1.2.3: about 95% of games, for both sides). gst-queen-1
then had no champion, so nobody fed, and round-limit games fell to the longest dragon.

The queen-safety knobs (kQueenNoTrap, kQueenReach, kQueenLookahead, kQueenSplitUntil,
kQueenKeep) are in the code and all off. That is giant's work in gst-queen-guard-1.

## Measured (sim on unswbc 1.2.3, vs gst-queen-1)
**Tuning, server maps seeds 1-3 (60 games):**

| build | W-D-L |
|---|---|
| heir champion (kept) | 37-0-23 |
| + queen never takes a dead end | 38-0-22 |
| + queen out of enemy reach | 38-0-22 |
| + queen keeps 6 and makes no growth splits | 28-0-32 |
| + queen moved by the lookahead | 39-0-21 |
| all of these | 23-0-37 |
| + queen grows all game (no growth splits) | 23-0-37 (official 43-0-47) |

The queen died in 92-100% of games in every build.

**Held out, seeds 21-25:**

| maps | W-D-L | decided by (wins-losses) |
|---|---|---|
| official | 88-0-62 | longest 54-11 |
| 10 server maps | 58-0-42 | longest 39-8 |

**1.2.3 sandbox (seeds 1-2, 8 server maps, both seats; the knobs off):** 16-0-16 vs gst-queen-1.
The sandbox and sim disagree here (sim: 58-42 on server seeds 21-25), so the sandbox number
needs a larger sample.

`check` passes on maps/official and maps/server. The release build has no mutable static state:
the gm2 counters exist only under GM2_DEBUG.

(gst-search-8's notes follow.)

# gst-search-8

## Idea
gst-search-6 with one rule changed: the rollout judge's contested-pearl review (another head
and a pearl in view) stops at this game's consolidation round, gst's feed_from_round, which
gst's team-size schedule sets. From then on, feeders and the champion keep gst's own moves.

The change is two lines in `plugin.cpp` plus a `FeedFrom()` accessor in `strategy.hpp`. There
is no map-keyed logic: the trigger is gst's own schedule, which follows the observed team size.

## Why
gst-search-6 trailed gst-search-4 on Slithery (3-7 on server seeds 21-25). The two differ only
in the rollout judge, and every Slithery game went to the round limit and was lost on longest.
Of the new judge's reviews, the contest review is the one that fires constantly on a hot-bed
map like Slithery (31 gap-1 beds), and in the endgame it pulls feeders and the champion off
gst's consolidation.

**This is a hypothesis tested by ablation, not a replay trace.** The two ablations below, both
on Slithery server seeds 1-10 vs gst-search-6 (20 games), agree with it:
- contest review stopped at the feed round: 15-0-5;
- contest review off: 9-0-11.

## Measured vs gst-search-6 (held out, both seats)
| maps / seeds | W-D-L |
|---|---|
| slithery_fight (server) 21-30 | 15-0-5 |
| all 10 server maps 21-25 | 55-0-45 |
| dilemma (server) 21-30 | 10-0-10 |
| portals + trauma (server) 21-30 | 21-0-19 (portals 12-8, trauma 9-11) |
| official 21-25 | 78-2-70 |
| official 26-30 | 73-0-77 |

Against gst-search-4 (v11) on dilemma 21-30, both gst-search-6 and gst-search-8 win 20-0 with
identical margins. The change never touches dilemma: its games end before the feed round.

`check` passes on maps/official and maps/server.

(gst-search-6's notes follow.)

# gst-search-6

## Idea
gst-search-4 (v11: gst-merge-2 + arena-search-7's lookahead steering plain late moves) with
one piece of gst-giant-2 ported: its rollout judge.
- **The judge.** rollout_giant2.hpp is a verbatim copy from tag variant/gst-giant-2 (credit:
  the giant team) and replaces gst-merge-2's older judge. Its rollouts let the other side lunge
  too. It reviews trapped actions against split escapes, vetoes steps into our own body, and
  reviews contested pearls (another head and a pearl in view).
- **The review structure is gst-giant-2's plugin.** gst-merge-2's farming skip and gst-search-4's
  path check on judge lunges are kept, and so is the late lookahead steering.
- **Self-contained:** only `plugin.cpp` and the added `rollout_giant2.hpp` differ from
  gst-search-4. No map-keyed logic.

## Which gst-giant-2 piece wins dilemma, default and devil?
Each piece moved toward gst-giant-2 on its own. Server maps dilemma/default/devil, seeds 1-5,
both seats, 30 games vs gst-giant-2:

| build | W-D-L | default | devil | dilemma |
|---|---|---|---|---|
| gst-search-4 as is | 10-0-20 | 5-5 | 3-7 | 2-8 |
| v8's schedule (kFinal 380, freeze 330) | 12-0-18 | 6-4 | 4-6 | 2-8 |
| split crowd gate on (3) | 11-0-19 | 5-5 | 4-6 | 2-8 |
| hot-bed farming off | 9-0-21 | 5-5 | 2-8 | 2-8 |
| **gst-giant-2's rollout judge (gst-search-6)** | **15-0-15** | 4-6 | 4-6 | **7-3** |

- **Dilemma is the judge.** gst-giant-2 reviews contested pearls ("rollout: contest" about 39 a
  match on dilemma), and dilemma's 12 gap-1 beds are always contested. gst-search-6 vs
  gst-search-4 on those maps: dilemma 9-1, default 3-7, devil 4-6 (16-14 overall).
- **Default and devil** don't follow any single piece.
- **Portals and trauma** vs gst-search-4 (seeds 1-5): 10-0-10 (portals 4-6, trauma 6-4). No regression.

## Held out vs gst-search-4 (both seats)
| maps / seeds | W-D-L |
|---|---|
| official 21-25 | 77-3-70 |
| official 26-30 | 78-0-72 |
| all 10 server maps 21-25 | 61-0-39 (61%) |

Per map on the server maps:

| map | W-L | map | W-L |
|---|---|---|---|
| dilemma | 10-0 | autarky | 7-3 |
| trauma | 7-3 | queen_of_spades | 7-3 |
| portals | 6-4 | schooltime | 6-4 |
| default | 5-5 | devil | 5-5 |
| trophy | 5-5 | slithery_fight | 3-7 |

- **Sandbox points** (big_empty, seed 1, self-play): p50 10.0M, p99 18.8-19.1M, max 25.3M per turn.
- **`check`** passes on maps/official and maps/server.

(gst-search-4's notes follow.)

# gst-search-4

## Idea
gst-merge-2 (the swarm team's gst line + hot-bed farming + the giant team's rollout judge,
taken verbatim from a1a5672 on task/g26-2) with gst-search-3's surgical lever. From r250,
gst's plain one-step moves take their direction from arena-search-7's per-dragon lookahead
(`search7.cpp`, a verbatim copy).

The move stays gst's own when:
- the rollout judge overrode it;
- the dragon walks to an ally to feed it (`WalkingToAlly`, the one hook added to gst's code);
- it steps into a dead end it chose to farm;
- the reason names feeding or the champion.

Everything else is gst-merge-2's: splits, hot-bed claims and farming, consolidation, trades
and sonar. There is no map-keyed logic.

## Why
- gst-search-3 (the same lever on gst-search-1) beat gst-search-1 90-60 held out, but lost
  autarky 0-10 to arena-search-7.
- gst-merge-2 wins autarky 7-3 and dilemma 10-0 against arena-search-7.

## Measured vs gst-merge-2 (tuning seeds)
| switch round | official seeds 1-5 (150 games) | server seeds 1-3 (60 games) |
|---|---|---|
| r250 (kept) | 94-0-56 | 37-0-23 |
| r150 | 81-0-69 | 33-0-27 |

The copy with no steering plays identically to the tagged gst-merge-2 (same results on
devil/trophy, seeds 3-4).

(gst-merge-2's notes follow.)

# gst-merge-1 (arena round 7, team swarm: the merge)

## What it is
mass-swarm-8 (gst131 line + value-judged trades) plus, one piece at a time:

1. **gst-hunter-1's hot-bed farming (kept).** Code credit: the hunter team. Strategist w's P1/P2.
   - A bed seen to reset its countdown to <= 3 is hot, and is worth a full pearl whenever due.
   - A dead-end pocket gains +1 per hot bed.
   - A 2-3 long dragon may enter a pocket that grows it to 4, then tip-splits out with gst's
     trapped split.
   - A farmer of length <= 5 heading for a hot bed within 8 claims it over sonar (0xA8). Allies
     skip hot beds within 2 of a claim for 30 rounds.
   - Ported verbatim. One fix: the claim tile is cleared at the start of every turn, so an early
     return (feed, split, trade) no longer rebroadcasts last turn's claim.
2. **gst-free-1's P3a opening hold (in the code, OFF: kEarlyHoldRound = 0).** Before r20 a
   dragon of length >= 8 doesn't split with an enemy head within 6. Code credit: the free team.
   It lost vs piece 1 alone, so it is off.
   - gst-free-1's map-type switch was NOT taken: it keys on a size table, which the RULE forbids.

**No map-keyed logic.** Nothing branches on the map's name, exact dimensions or a per-map table.
- Hot beds are detected from observed countdowns.
- The one size-dependent rule is inherited unchanged from v8: gst's opening centre bonus
  applies when width x height <= 1024. It is a generic area threshold, not a table.

Endgame left alone (search is folding in search-7's). mass-swarm-6's consolidation stays as it
was.

## Piece by piece (faithful sim, both seats)
| step | server maps (5), seeds 1-10 | official, seeds 1-10 |
|---|---|---|
| hot-bed farming vs mass-swarm-8 | 61-0-39 | 160-0-140 |
| + opening hold vs hot-bed farming alone | 44-0-56; on all 10 server maps 94-0-106 | 154-0-146 |

For the hot-bed farming step vs mass-swarm-8 on server maps: r50 16.4 v 14.6 dragons, r100 24.7 v
20.9, total at r100 63.3 v 53.5.

## Final build vs the targets
**Official maps, held-out seeds 21-30** (300 games per opponent):

| opponent | W-D-L | win % | final margin (longest / total) | r50 dragons | r100 total |
|---|---|---|---|---|---|
| gather-split-trade-131 (v8) | 185-0-115 | 62% | +2.8 / +16.9 | 13.3 v 12.1 | 59.1 v 52.5 |
| gst-hunter-1 | 182-1-117 | 61% | | | 59.1 v 54.9 |
| gst-search-1 | 150-0-150 | 50% | | | 55.7 v 52.4 |
| mass-swarm-8 | 159-1-140 | 53% | | | 55.8 v 52.5 |
| arena-search-7 (v9) | 144-0-156 | 48% | -2.0 / -7.5 | 13.3 v 13.3 | 55.2 v 58.4 |

**All 10 server maps, seeds 1-10** (the first five maps plus default, devil, portals, trauma and
trophy; 200 games per opponent):

| opponent | W-D-L (first 5 + new 5 = total) | win % |
|---|---|---|
| v8 | 67-33 + 64-36 = 131-69 | 66% |
| gst-hunter-1 | 64-36 + 60-40 = 124-76 | 62% |
| mass-swarm-8 | 61-39 + 54-46 = 115-85 | 58% |
| gst-search-1 | 54-46 + 45-55 = 99-101 | 50% |
| arena-search-7 | 34-66 + 42-1-57 = 76-1-123 | 38% |

Economy on the first five server maps vs v8: r50 17.2 v 13.7 dragons, r100 26.3 v 19.7 (total
67.9 v 50.4). Against arena-search-7 we lead at r50 (17.5 v 13.5) and r100 (25.9 v 20.6, total
66.9 v 50.0) and still lose 34-66, ending -6.9 longest and -40 total. Its endgame beats ours.

**The real M665147 seeds on maps/server** (seats A / B):

| map | vs v8 | vs arena-search-7 |
|---|---|---|
| slithery | W / L | L / L |
| schooltime | L / L | L / L |
| autarky | W / W | L / L |
| dilemma | W / W | W / W |
| queen_of_spades | L / W | W / W |

**Hot-bed pearls** (judge sandbox via unswbc, server seeds, vs gst131, strategist w's hot.py):

| map | seat | gst-merge-1 | gst131 | result | mass-swarm-8 (same games) |
|---|---|---|---|---|---|
| autarky | A | 220 | 6 | won | 8 v 4 |
| autarky | B | 192 | 4 | won | 4 v 4 |
| slithery | A | 1184 | 524 | lost on length | 335 v 433 |
| slithery | B | 789 | 527 | lost on length | 479 v 296 |

- The hot-bed farming works as intended.
- The sandbox's Slithery seat-A result (a loss) disagrees with sim's (a win, 40-31). Still to
  check, e.g. whether the unswbc run seeds the bots' RNG.

**CPU** (judge sandbox): p50 4.4M, p99 5.5M, max 6.1M points per turn. `check`: all checks
passed.

## Schooltime at the server seed (trace, gst-merge-1 vs v8, seat A; mass-swarm-8 is the same)
- **Even to r80:** 18 v 17 dragons, total 48 v 48.
- **From r100 we stall and v8 grows:**

  | round | dragons, us v v8 | total, us v v8 |
  |---|---|---|
  | 100 | 22 v 25 | |
  | 140 | 26 v 47 | 59 v 125 |
  | 220 | 25 v 64 (the cap) | |
  | 320 | 25 v 64 | 67 v 162 |

  From r320 v8 eliminates us, by r386.
- **Whole game:**
  - v8 splits 438 times to our 188, and enters dead ends "worth it" 561 times to our 208.
  - v8 lunges 36 of our short dragons (134 length) against our 20 (54 length). Our value rule
    lets our dragons stand beside heads it rates as fair trades; v8 then trades its 2-3-long
    dragons into our longer ones.
- **In r80-320 deaths:** trades are even (about 26 lunges each way). v8 loses 211 dragons
  "giving up a slot" at the cap: it is so far ahead it can throw them away.
- **Conclusion:** the loss is an economy stall from r100 on this map and seed: fewer splits and
  fewer dead-end entries. Hot-bed farming doesn't reach it: the result is identical with and
  without farming.
- **Next here:** find why our dragons stop splitting from r100. Likely candidates:
  - the lunge losses of 4-5-long dragons, which are the ones that would split;
  - Exposed() under the value rule (making it cautious again for short dragons beside longer
    enemy heads is worth a test).

## Wrapping it with gst-giant-1's rollout judge (gst-merge-2)
Easy. gst-giant-1's plugin.cpp wraps any gst StrategyAgent: it calls gst's Turn(), and when an
enemy head is in view and the action is a non-feeding move, it lets rollout.hpp's Judge review
it. Wrapping this merge needs only:
- rollout.hpp copied in (namespace renamed);
- a plugin.cpp that holds gm1::StrategyAgent instead of gstg1::StrategyAgent.

Its feeding exclusion matches on "feed" / "champion" in the reason, which covers our "feed
champion" and "feed: chop". Our hot-bed reasons need no exclusion. One caution: the judge might
override a farmer's dead-end step when an enemy head is in view.

## Fix (check)
The rollout judge's multi-step lunge override is now taken only if every step is free in gst's memory, with an enemy head allowed only at the end. `check` found one wall death on queen_of_spades from an unchecked lunge; the parent checked only the first step.
