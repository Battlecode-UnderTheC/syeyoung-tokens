# body-wall-24

## Idea

body-wall-23 plus gather-split-trade-64's split_announce, crossed in. On a trapped/doomed split
that sends at least half the dragon's length off, it pings its own id at the new stub length that
same turn (instead of the stale pre-split full length), and starts its obituary relay right away.
The child of that split is born right behind the parent's tail and hears that ping first, so it
was being seeded with a ghost champion (its own soon-dead parent) every trapped split; gst64
measured this cutting trapped-split L10+ ghosts from 186 to 52 at r450 and raising the longest
dragon's share of team length at r499 from .32 to .385 (league/KNOWLEDGE.md 'Ghost champions').

## What changed from the parents, and why

- Namespace `bw23` -> `bw24`, macros `BW23_*` -> `BW24_*` (mechanical; a few comments/debug
  strings still say "bw23"/"BW23" but those are just text, not identifiers, so left alone).
- New param `split_announce = 1` (`strategy.hpp`, alongside the existing obit params).
- In `Turn()` (`strategy.cpp`), right after the existing obituary relay block and before the
  never-portal-emit block (so it sits at the same priority the obituary relay already had: it
  overrides the champion/threat/portal-cross pings that were set earlier in the turn, and a
  never-emit portal warning still overrides it): if the action this turn is a `SPLIT` sending at
  least half our length off (`2 * a.split >= length_`), the reason starts `t` or `d` (`"trapped:
  split"` / `"doomed: split"` — growth splits, reason `"split"`, are excluded per the brief: they
  are routine and announcing them would burn sonar turns on nothing), and we already had a report
  of ourselves at champion length (`reports_[spawn_.id].length >= p_.obit_champion`), then set our
  own report and the obituary to our new stub length this round and ping it in all four
  directions. This is a direct, minimal port of gst64's block (verified by diffing gst64 against
  gst62 to isolate exactly this change) — body-wall already shares the `Report`/`Encode`/
  `reports_`/`obit_`/`obit_until_`/champion/obituary machinery with the gst line, and body-wall's
  `Action`, `Split(n)` semantics (`n` = length handed to the child) and reason strings for
  trapped/doomed split match gst64's exactly, so no adaptation was needed beyond the port itself.

## What measured

`sim --threads 2`, both seats.
| opponent | maps | W-D-L | longest/total margin |
|---|---|---|---|
| body-wall-23 | official (22) | 10-0-12 | -0.5 / -2.3 |
| body-wall-23 | worldgen, fresh batch of 20 (40) | 22-0-18 | |
| trading-swarm-5 | official (22) | 18-0-4 | |
| trading-swarm-5 | worldgen, same batch (40) | 31-0-9 | |

`check body-wall-24 maps/official`: all checks passed, slowest turn 0.1-0.3 ms (one cold-cache
run showed 0.8 ms on queen_of_spades; reran 3x steady at 0.1 ms, same as parent).

## What should improve

Fewer ghost champions from trapped/doomed splits: the split child and its neighbours should hear
the parent's real (short) post-split length immediately instead of chasing a stub that's about to
die, per gst64's probe numbers on the gst line. Didn't re-run the CDBG probe here (out of budget
for this task; body-wall has no equivalent probe wired up) — the win-rate numbers above are even
or ahead of parent on both map sets, which is the bar this task set.

## What I tried that didn't work

Nothing else attempted; this was a single, focused port per the brief's 20-minute budget.
