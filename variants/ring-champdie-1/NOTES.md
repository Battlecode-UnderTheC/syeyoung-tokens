# ring-champdie-1

## Idea
Late deaths fall next to ONE champion. Observed cause (server-r16 C2): in r400-499 the
opponent's champion eats 13.8 pearls from its own team's dead, ours 5.8, with bed pearls equal.
Their late deaths happen beside the champion; ours happen wherever the dragon is.

A debug log of the base (ring-champfood-1 / ring-sched-26) on 44 fresh mapgen maps showed two
causes:
- **The team names many champions.** Games average 11-18 distinct champion names between r440
  and r499, and the majority name is held by only 33-61% of dragons. g40-3's pick stuck until the
  champion died, and a newborn split child, knowing only itself, named itself champion. About 40%
  of late resigns were stubs that thought they were the champion.
- **Stubs die trapped where they stand.** Most late deaths are 2-3 long stubs ("trapped: no
  move"), and only ~21% of late resigns fall within 3 tiles of the champion. 61% of the 2-3 long
  non-champions alive at r440 resign before r500.

## Change (one mechanism, strategy.cpp/.hpp only)
Gated on the base's lateconc condition (our queen not known alive), from kDieNearFrom = r400:
- **One champion.** Every non-queen dragon names the longest live report (fresh within
  kReserveFresh, ties to the lower id), re-decided every round. This is a total order, so groups
  that hear the same reports name the same dragon. Champion() relays it from r400, not r430.
  late_conc_ (growth stop, champion harvest, food reservation) still starts at r430.
- **Doomed stubs die beside it.** A dragon of length <= kDoomedLen (3) that has eaten nothing for
  p_.starve_rounds (35, the team's existing "starving" mark; added in iteration 2) is exempt from g42-3's
  hold-back until r480. It takes the existing feed walk to the agreed champion and resigns
  within kLateFeedDistance of its head, where the harvest sprint eats the drop. There is no new
  feeding of longer dragons. Trapped dragons with no move still die in place, since they can't
  move.

## Iteration 1 (ring-sched-31: every stub of 3 or less counts as doomed)

### Local numbers (fresh maps, mapgen seed 80842, 44 maps, sim seeds 1-2, vs ring-sched-m3)
- New 99-4-73, base 96-4-76.
- Decided by longest: new 46-21, base 37-25.
- Queen-dead games played to r499: longest 43.3 v base 39.2. Total 362 v base 379 (-4.5%).
- Agreement (r440/r480/r499): distinct champion names per game 6.1/6.1/6.0 (base 11.2/16.6/17.6).
  The majority share is 0.69/0.64/0.67 (base 0.61/0.37/0.33). Late resigns within 3 tiles of the
  champion went from 540 to 1241 (seed 1), and self-named stubs from 1130 to 68. The remaining
  fragmentation is groups out of sonar contact, each with its own 20-25 long dragon.
- Per family (W-D-L), new v base:

  | family | new | base |
  |---|---|---|
  | anti-centre | 10-0-6 | 8-0-8 |
  | crowded | 8-0-8 | 9-0-7 |
  | maze | 7-4-5 | 6-4-6 |
  | mixed | 8-0-8 | 9-0-7 |
  | off-axis | 10-0-6 | 10-0-6 |
  | open | 8-0-8 | 8-0-8 |
  | rich-near-home | 9-0-7 | 8-0-8 |
  | ring | 8-0-8 | 9-0-7 |
  | rooms | 13-0-3 | 11-0-5 |
  | sealed-spawn | 11-0-5 | 10-0-6 |
  | sparse | 7-0-9 | 8-0-8 |

- Stronghold, 4 seeds: 3-1-4, base 3-0-5. Both are queen-length coin flips.

### Arena duels (eph-dd1b51d5, random:2)
- Server v ring-sched-26: 34-0-34. Longest 8-4, queen 14-17, elimination 12-13. Starters: 3+
  went 20-20, 2 or fewer 14-14.
- Server v ring-sched-17: 33-0-35. Longest 9-7, queen 13-17, elimination 11-11.
- Official v ring-sched-26: 7-0-13. Queen 0-5 (4 on stronghold, where both queens are alive),
  longest 2-3, elimination 5-5.
- CPU max 35-39M.

Replays (duel-5410715c, Australia, same seed with the seats swapped). Measured: end champion's
pearls eaten in r400-499 by origin.
- **Game 20 (ours A):** our champion 778 grows 16 -> 30 in r474-490 inside a dense cyan cluster.
  It eats the drops of a wave of stub deaths right around it. It ate 20 own-dead pearls (parent
  5). Longest 37 v 15, total 176 v 162.
- **Game 21 (ours B):** 8 own-dead pearls v 3. Longest 35 v 28, but total 130 v 231: the team
  shrank 44 -> 18 dragons.
- **Game 5:** 6 v 0.

### Arena verdict: ring-sched-31
- Against ring-sched-26: 55-0-57, even (within noise).
- By suite: server 38-30, custom 12-22, official 5-5.
- Decided by: elimination 24-23, queen 21-20, longest 10-14 (custom 1-7).
- Games with both queens dead, to r500:
  - Server: longest 26.0 v 21.8, total 145 v 160, won 7 of 10.
  - Custom: longest 24.8 v 31.0, total 194 v 267, won 1 of 8.
- In custom game 160156 (walled-centre) the team was already at half the parent's mass at r380
  (185 v 361). Our stubs then vanished: from 40 dragons at r412 to 19 at r443, against the
  parent's 54 to 43. Our champion ate 11 own-dead pearls (parent 1) but finished 23 v 42.
- A re-run custom duel (eph-dd1b51d5) came out 34-34, longest 10-7, so 12-22 is partly seeds.
- Against the others: ring-sched-17 60-52, cs-queen-split-tail-risk-v1-7 70-42, ring-rush-1 74-38.
- CPU max 47.8M.

## Iteration 2 (ring-sched-34, the code here): only starving stubs are doomed
Why: on custom maps late stubs still forage and grow, and feeding them all stripped the team.

Local, same 44 fresh maps, seeds 1-2:
- 93-4-79 (it1 99-4-73, parent 96-4-76).
- Decided by longest: 41-28.
- Queen-dead games to r499: longest 40.0 v parent 39.2, total 359 v 379.
- Per family: anti-centre 8-8, crowded 8-8, maze 8-4-4, mixed 8-8, off-axis 7-9, open 8-8,
  rich-near-home 8-8, ring 10-6, rooms 11-5, sealed-spawn 10-6, sparse 7-9.

Duels against ring-sched-26 (eph-3e9056d4):
- Custom 35-33 (longest 10-8).
- Server 31-37 (longest 6-4, queen 12-21).

### Arena verdict: ring-sched-34
- Against ring-sched-26: 70-0-60, beats the parent (within noise). On the server's maps 47-34.
- By suite: server 47-34, custom 17-21, official 6-5.
- Decided by:
  - Server: longest 12-6, queen 19-15, elimination 15-13.
  - Custom: longest 2-6.
- Games with both queens dead, to r500:
  - Server (n 12): longest 24.2 v 20.6, total 154 v 156, won 9.
  - Custom (n 8): longest 27.2 v 29.9, total 215 v 278, won 2.
- Eval P(win) against the parent: 0.504 at r100, 0.508 at r200, 0.497 at r300. The change acts
  from r400.
- Against the others: ring-sched-17 71-60, ring-s17-1 69-61, ring-rush-1 78-54.
- CPU: p99 27.3M, max 41.1M.

## Next
- Custom maps (3+ starters, big boards) still lose the longest tiebreak. There the team ends with
  several 20-25 long dragons in separate groups that never hear each other's sonar. One champion
  per game is not reachable by relaying the champion's report alone. Next: relay the second-best
  report too, or let a group's champion walk toward the team's longest.
