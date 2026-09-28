# gather-split-trade-39

## Idea

gather-split-trade-38 with obituaries: a long ally that loses its length (a trapped split hands
the mass to a child under a new id) or dies stops being named champion, because the demotion is
now relayed instead of only ever relaying the (stale) champion. Plus: split where the ping from
our new tail runs straight into the child, so it is born knowing the champion.

## What changed from the parent, and why

Diagnosis (trace, r460): in the worldgen games where almost nobody named the true champion,
most dragons named a *dead* ally (e.g. 39 of 48 naming one id): a champion that trap-split
(stub keeps the id, child carries the mass) or died. Its last report survives up to 80 rounds
with a 0.25/round fade (gst38's late settings), and the stub's exact small self-report is never
relayed because only champions are pinged.

- `obit_*`: a report that cuts a known >= 10-long ally to less than half (exact, newer) is an
  obituary: the dragon that learns it pings it in all four directions on odd rounds for 20
  rounds (the champion on even ones). Sources: (a) our own length fell below half of our last
  self-report (we split the mass off); (b) an ally >= 5 segments in view, its head beside ours
  last turn, is gone now while we moved one plain step (it died): noted as length 0.
  A wrong obituary is overwritten by the ally's next relayed self-report (newer round).
- `heard_split`: growth splits try n = L/2, L/2+1, L/2-1 for one where the body runs straight
  through the cut (so the tail ping, opposite the facing, hits the child's tail); trapped
  splits of a >= 8 dragon keep one more segment if that makes the cut straight.
  Effect was small: children hearing nothing on their first turn fell only 22% -> 19%
  (half the growth splits are at length 4, where n = 2 is the only choice; neighbours cover most).
- Namespace/macros gst38 -> gst39. Trace (`GST39_TRACE`: champion per dragon every 10 rounds
  from 400, child first-turn hearing, split straightness, obituaries) compiled out.

## Measured (sim vs gather-split-trade-38, both seats)

| maps | W-D-L | longest margin | total margin |
|---|---|---|---|
| official (22) | 14-0-8 | +4.3 | +26.0 |
| worldgen set 1 (20 maps, 40) | 21-0-19 | +0.1 | +15.0 |
| worldgen set 2 (seed 8808, 40) | 20-0-20 | -2.2 | +11.9 |

Dragons naming the true champion at r460 (our side, threads 1):

| maps | gst38 behaviour | heard_split only | + obituaries |
|---|---|---|---|
| official | 131/178 = 74% | 113/163 = 69% | 112/154 = 73% |
| worldgen set 1 | 204/305 = 67% | 189/328 = 58% | 242/334 = 72% |
| worldgen set 2 | 129/268 = 48% | - | 166/260 = 64% |

heard_split alone did not raise knowledge (the difference is game noise); obituaries did, on
worldgen. One official match (0/30-37) and one worldgen match (1/48) stay at ~0 with and
without: the named dragon is dead but no one saw it die.
check: all checks passed (slowest turn ~16 ms under load average ~140; parent 23 ms same run).

## What should improve

Fewer feeders walking to ghost champions late; W-L within noise of the parent, totals up.

## Next

Death detection is narrow (only an ally beside our head). A champion that stops being heard
could be dropped faster once a *newer* report of a nearly-as-long ally arrives; or the champion
could mark its self-report with a heartbeat bit so relays can tell alive-and-far from dead.
