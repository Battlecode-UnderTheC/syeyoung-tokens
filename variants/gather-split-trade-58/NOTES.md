# gather-split-trade-58

## Idea

Assassinate the enemy champion late. A head-to-head kills both dragons whatever their lengths,
so when the longest enemy dragon we know of is longer than our champion, a short expendable
dragon trading into it can flip the "longest dragon" verdict. The strongest server opponent
did this to us (lunges and pincers on our champion); this is the same weapon pointed back.

## What changed from the parents, and why

gather-split-trade-51 (namespace gst51 -> gst58), plus:

- Every dragon remembers `enemy_`: the longest enemy dragon whose head it has seen (id, head
  tile, length as a lower bound, kept as the max over partial sightings of the same id, exact
  when its whole body is in view). If its last known head tile is back in view and it isn't
  there, the memory is dropped (relays up to that round are ignored after).
- Sonar relay: from r380, on rounds % 4 == 2 (not on obit rounds), a dragon with a fresh
  (<= 10 rounds) enemy champion report pings `0xA8 | team | id | x | y | length | round`
  (same layout as the ally report, own magic) instead of the ally champion word.
- `WorthTrading` from r400: true for the known enemy champion when it is longer than our
  champion (by our report), we aren't the champion, and we're shorter than it. So kamikaze,
  lunge (up to L-1 steps) and `Exposed` (no longer shies away) all go for it.
- From r420 a non-champion of length <= 8, not feeding a champion in view, within 30 path
  steps of the enemy champion's last known head (seen <= 20 rounds ago) walks there
  ("assassin: walk"). Pincers aren't coordinated explicitly: several short dragons converge.
- Debug counters (GST58_DEBUG, compiled out): late (r400+) trades into enemies >= 10 long,
  trades into the enemy champion, assassin-walk turns, and how often the trigger holds.

## Measurements (sim, both seats)

| opponent | official (22) | worldgen A (40) | worldgen B (40) | total |
|---|---|---|---|---|
| gather-split-trade-51 | 11-0-11 | 20-0-20 | 22-0-18 | 53-49 |
| late-surge-23 | 9-0-13 (51: 11-11) | 20-0-20 (51: 21-19) | 21-0-19 (51: 16-24) | 50-52 (51: 48-54) |
| trading-swarm-5 | 14-0-8 (51: 14-8) | 22-0-18 (51: 23-17) | 31-0-9 (51: 32-8) | 67-35 (51: 69-33) |

Even with the parent within noise. The trigger is rare against league opponents: of ~30-70k
late non-champion turns per 40-match set, the enemy champion is known (fresh) ~55% of the
time but longer than our champion only 0.2-1% (57-394 turns); assassin walks 0-41 turns per
set; late trades into enemies >= 10 long 5-17 per set, 0-5 of them into the enemy champion.
Result flips on longest were not measured separately (too few events to see).

## What should improve

Against opponents whose longest outgrows ours late (the server's trading swarm, which grows by
eating our champion's corpse), short dragons now go and trade into it instead of feeding a
champion that is already behind. In the league's sims our champion is almost always the
longest, so the change is mostly dormant there.

## Tried / not done

No parameter sweep (short task). Next: measure against a replayed/real-opponent-like bot where
the enemy champion is longer; a lower-bound length from sightings underestimates the enemy, so
trusting a relayed exact length or triggering at "within k of ours" may fire more usefully.
