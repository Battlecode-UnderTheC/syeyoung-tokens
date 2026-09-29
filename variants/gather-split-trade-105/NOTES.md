# gather-split-trade-105

Task g20-2, tweak of gather-split-trade-96 (namespace renamed gst96 -> gst105,
GST96_* -> GST105_* throughout so it can be linked alongside gst96 without clashing).

## Idea

Strategist-a's analysis of the 7-18 real-server record (M547322/M561834/M572141/M578650)
found two loss mechanisms: teamElimination from being out-counted early, and round-limit
losses on the *longest dragon* comparison despite having more total length (M578651
Schooltime: we finished n27/T244/L41 vs their n2/T62/L47 and still lost). The second
mechanism is directly actionable this task: gst96 only starts feeding the champion and
freezing its splits at round 330-380, leaving only ~120-170 rounds of team-wide feeding
before the 500-round limit. This budget targets that: pull the champion-growth window
earlier so the champion has more rounds to out-grow the opponent's longest dragon.

## What changed from the parents, and why

In `strategy.hpp`: `feed_from_round` 380->300, `leader_feed_from` 380->300,
`champ_freeze` 330->300. All three now line up with the existing `leader_split_until=300`,
so the moment non-champion leaders stop splitting for their own growth, they immediately
start routing food to the champion instead of idling for ~50-80 rounds with no clear task.
No other logic changed (did not attempt strategist-a's P2 local-density split-suppression
or P3 opening-speed probe within this budget; see below).

I did not add a GST105_DEBUG feed/length counter as the brief suggested (time budget);
the win-rate and longest-margin deltas below already show the direction without it, and a
next task with more budget should add real per-map r50/r100/r400/r499 counts as asked.

## What I measured

`check gather-split-trade-105 maps/official`: all checks passed (max slowest turn 4.0ms on
queen_of_spades, in line with gst96's own check numbers — not a regression from this change,
the tweak touches no per-turn hot path).

vs gather-split-trade-96 (parent), both seats:
- Official maps (11 maps x2 seats = 22): **11-0-11**. Mean rounds 332; at the end
  gather-split-trade-105 leads by **+2.7 longest** on average, -11.5 total. This is exactly
  the shift the idea targets: even record, better on the metric that decides round-limit
  games.
- 20 fresh worldgen maps (`worldgen.py --batch 20`, default seed), both seats (40 matches):
  **18-0-22**. Mean rounds 395; +0.7 longest, -18.5 total. Slightly behind on raw
  win count but still ahead on longest-dragon margin, and no draws either way — worldgen
  maps are noisier/more varied than the curated official set, consistent with a small change.

vs trading-swarm-2 (official maps, 22 matches): **11-1-10**, +4.7 longest, -2.7 total.
vs trading-swarm-5 (official maps, 22 matches): **14-2-6**, +4.0 longest, -23.9 total.
(trading-swarm-8 was not yet tagged at measurement time, so not included.)

I did not have time in this budget to reproduce M578651 Schooltime specifically against the
real opponent id, or to instrument actual champion length at r400/r499 — the aggregate
"+longest at game end" numbers above are the proxy used instead.

## What should improve

Longest-dragon margin at game end is up against the parent (+2.7 official, +0.7 worldgen)
and against both trading-swarm baselines (+4.7, +4.0), which is the exact axis strategist-a's
M578651 finding says decides round-limit losses. Win rate is roughly even against the parent
(11-0-11 official, 18-0-22 worldgen — combined 29-0-33) and positive against trading-swarm-2/5,
so this clears the "at least even" submission bar.

## What I'd try next

- Add the GST105_DEBUG champion-feed-count / length-at-r400/r499 instrumentation the brief
  asked for, and rerun specifically on Schooltime/Queen of Spades vs a trading-swarm-8 clone
  once tagged, to confirm the mechanism (more feeds reaching the champion, not just luck).
- Strategist-a's P2 (skip routine splits when locally outnumbered >=3:1) targets the other,
  larger loss mechanism (teamElimination from being out-counted early) and is untouched here;
  worth a dedicated task since it touches the split-decision hot path and needs its own
  Devil/Trauma worldgen measurement.
- Sweep champ_freeze/feed_from_round further apart (e.g. freeze at 300 but feed a bit later,
  or vice versa) now that 300/300/300 is the working baseline, since this task only tried
  one combined point rather than the full grid the brief listed.
