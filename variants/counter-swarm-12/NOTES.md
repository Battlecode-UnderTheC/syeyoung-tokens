# counter-swarm-12

Mutate (task g6-9) of counter-swarm-11: close the worldgen gap to gather-split-trade-23.

## Idea

Diagnosis (cs11 vs gst23, worldgen): trades are one-for-one and trapped deaths per match are
about equal (173 vs 178), so the base safety layer is not the gap. Two loss shapes:
(1) small 16x16 mazes where the gst swarm out-gathers and eliminates us by round ~100-200;
(2) big maps where cs11 holds 64 dragons at the cap until 300, then stops splitting at 300
while gst workers keep splitting to 350: our short workers die to lunges and traps with no
replacements (traced pg_545736: 64 dragons at 299, 8 at 350, 1 at 374; the lone champion was
then lunged), and we lose by elimination at ~320-480.

## What changed from the parents, and why

- Namespace cs11 -> cs12 (files, macros).
- `split_until_round` 300 -> 350 (gst's value; KNOWLEDGE g4-4 had 350 > 300). Fixes loss shape 2.
- Growth splits only where both halves can get out (`SplitSafe` + `EscapeLevel` ported from
  gst23, `check_split = 1`). Only the split check, not gst's step look-ahead.
- `contest_factor = 0.3` (gst): a target an enemy head in view reaches first is worth less.
- Ally sharing by path distance (`x3 = 100`: ally torus distance < our path distance, as gst).
- Cap feed (gst20) off (`cap_feed_max_length = 0`): ~100 resigns a match, no measurable gain here.

## Measured (native sim, both sides; practice/practice2 used while choosing, practice3 fresh)

| A vs B | practice (20) | practice2 (20) | practice3 fresh (20) | official (11) |
|---|---|---|---|---|
| cs11 vs gst23 | 16-0-24 | 12-0-28 | 18-0-22 | 16-0-6 |
| cs12 vs gst23 | 21-0-19 | 21-0-19 | 19-0-21 | 10-0-12 |
| cs12 vs cs11  | 20-0-20 | 23-0-17 | 24-0-16 | 10-0-12 |

Ablations vs gst23: without check_split 33-47 on practice+practice2 (vs 42-38); cap feed on
39-41; split 300 17-23 on practice3 (vs 19-21); no contest 15-25 on practice3. On official
every single ablation lands 9-11 wins of 22, so the drop from cs11's 16-6 is not attributable
to one change (22 official games are chaotic).

check: all checks passed, slowest turn 0.1 ms.

## What should improve

No 300-350 collapse on big worldgen maps. Official vs gst23 regressed 16-6 -> 10-12; the
league should confirm or refute it.

## Next

Guard the champion against lunges after 340 (it dies once the swarm has fed it); the small-maze
economy gap (gst out-gathers with the same number of dragons) is still unexplained.
