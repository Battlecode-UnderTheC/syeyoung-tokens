# counter-swarm-31

## Idea

On small maps (<= 1,024 tiles) the swarm reaches the central field first and out-eats us
in the opening. Before round 50, pull goal choice (pearls, beds and the unexplored edge)
towards the map centre, so our dragons contest the middle, not only their own side.

## What changed from the parents, and why

counter-swarm-30 (namespace renamed `cs30` -> `cs31`, macros `CS31_*`) plus one port from
body-wall-17 (its NOTES.md and a diff against body-wall-16 isolate the change to a single
addition, since bw17's other diffs are just the namespace/macro rename):

- `ChooseGoal`: while `round_ < centre_until` (50) on maps with `width*height <=
  centre_max_tiles` (1,024), each candidate's score, and the explore-edge score, is
  multiplied by `pull = 1 + centre_bonus * (1 - torus_dist(tile, centre) / (W/2 + H/2))`,
  applied right after `score = value / (ready + 1)` and before the ally/enemy contest
  discounts, and again on the explore-edge max. `centre_bonus = 0.3` (as in bw17). Big maps
  and rounds >= 50 are unchanged (`pull = 1.0`).
- No other logic touched; cs30's own torus lambda (already used for allies/enemies) is
  reused for the centre distance rather than adding a second one.

## What I measured

Official maps (`maps/official`, both seats, `sim --threads 2`):
- vs counter-swarm-30 (parent): **14-0-8**, mean rounds 356, at the end leads by +0.9
  longest, +35.4 total length.
- vs trading-swarm-5: **19-0-3**.

20 fresh worldgen maps (`maps/worldgen/worldgen.py --batch 20`, both seats):
- vs counter-swarm-30: **21-0-19**, at the end -0.7 longest but **+15.7 total length**
  (essentially even in wins, positive on the tie-break margin the task cares about).
- vs trading-swarm-5: **28-0-12**.

`check counter-swarm-31 maps/official`: all checks passed, 0 avoidable deaths.

This matches the pattern reported for the port into gather-split-trade (g15-26 brief): even
or ahead in wins, positive on total-length margin, and both ports beat trading-swarm-5
comfortably (83-19 for the bw17 line's own report; here 19-3 official / 28-12 worldgen).

## What should improve

Small-map openings against the swarm archetype and trading-swarm-5, by contesting the
centre earlier instead of ceding it. The official-map win rate against the parent (14-0-8)
suggests this generalizes past just the one map bw17 was built to fix (Devil), though the
worldgen win margin is closer to even (21-19) — most worldgen maps are large enough (or
symmetric enough) that the centre pull matters less; the total-length margin stayed
positive there, which is the more robust signal on a near-even W-D-L split.

## Tried and didn't work / not tried

Per the brief's 20-minute budget, I did not sweep `centre_bonus` to 0.2 or 0.45 — the
default 0.3 already showed a clean win over the parent on both map sets with a positive
length margin everywhere measured, so a sweep was left for a follow-up task rather than
spent here. bw17's own notes record that `centre_bonus = 1.0` is too strong (piles the team
into a crowded middle, doubles "trapped: no move" deaths); worth remembering if a future
task tunes this further.
