# ring-shield-fieldbeds (g53-11)

Base: ring-shield-bastion (arena ring-shield-3). Brief: g53-7's trace of 261181 + strategist A g52 #8.

## Idea
The field walker's arrival test (field.cpp) counted only LIVE pearls in its 7x7 (needs 4). On 261181 (wg40004,
seed 1592614637; reproduced by `build/tools/match ring-contest-m1 <us> map seed`, us as B) walker id5 stood at
(22,17) at r12 with 2 live pearls and beds due in 2, 15, 20, 25 rounds (the bastion column x=19 and its own side),
walked on south, and the opponent's explorer filled the bastion from r16.

## Change (field.cpp/strategy.hpp; FIELDBEDS_MODE=0 is the parent)
- Arrival: a bed on NEW ground (not seen at spawn) that is not ripe but whose observed countdown says it ripens
  within kSoonDue=30 rounds (BedWeight's soon-due horizon) counts 1 - due/31 of a pearl. Linear so a slow-bed lane's
  chance few count about half each (the kFieldBeds devil trap).
- Home beds (seen at spawn) are excluded: counting them settled walkers 3-4 tiles out at r3-6 on fresh mixed and
  rich-near-home maps.
- Tried and left off (FIELDBEDS_BARREN, default 0): the same part-pearls in the barren test (home rate and new
  ground). Fresh mapgen seed 531107: it kept walkers out to r18-r28 on sealed spawns (sealed-spawn 6-10, r40 total
  14.8 v 16.1; mixed 6-10; gained only on anti-centre). Off: those families 8-8 and identical at r50.
- First horizon tried: 12 rounds. On 261181 only one bed fell inside it at r12-14, nothing changed.

## Traces (FIELD_DBG build, FIELDT per-turn line)
- 261181: parent id5 never arrives (walks on, no head of ours inside x10-18 to r44). Change: id5 "arrived" at r12
  at (22,17), farms, enters the bastion at r30 (18,16), splits inside r27-38; heads inside x10-18: 2 at r31, 4 at r37,
  6 at r40-43; team dragons r40 24 v 13. id9 now arrives at r6 at (23,5). Game still lost on queen (our queen dies
  r459 instead of r71; total at end 10 v 84).
- mg40506 (seed 1592614637, us as A v ring-contest-m1): all 6 walkers still exit "barren heading" at r12, none in
  field mode at r13-30; identical to the parent. The change does nothing there (the barren variant kept 3 walkers
  out to r13/r16/r22 and did not help).

## Local (v ring-shield-bastion)
- maps/server seed 1: 19-0-15; r50 dragons/total 14.9/36.2 v 14.5/35.2.
- Fresh mapgen seed 531107 (44 maps x seeds 1,2 x both seats): 84-8-84. By family: anti-centre 8-8, crowded 7-9,
  maze 6-4-6, mixed 8-8 (r50 total 26.2 v 29.4), off-axis 8-8, open 10-6, rich-near-home 7-9, ring 8-8, rooms 6-4-6,
  sealed-spawn 8-8, sparse 8-8. Most families identical to the parent at r50 (the rule rarely fires).
- check maps/official: all passed.

## Arena
- `tokenz ephemeral` timed out (400 s, no output) from the worker sandbox, so no duels and no submit: handed back
  to the orchestrator. Bot dir: /tmp/g53-11/bot (scripts/make-bot.sh).
