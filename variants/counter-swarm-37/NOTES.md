# counter-swarm-37

## Idea

Crossover of counter-swarm-36 (base) and gather-split-trade-96: port gst96's dead-end
economics onto cs36's already-more-sophisticated dead-end logic. cs36 (over gst90/96) already
has a tail-following escape test (`ProjectedBody` + `EscapeLevel`) that rules out most of
what a flat `RoomAround` flood mistakes for a dead end, so gst96's "room to follow our tail
out" case (its `cost = 0` branch) is redundant here — cs36's escape check already catches it
before we ever price a trap. The remaining, narrower gst96 idea still applies: once a pocket
really is a dead end, price it by expected loss instead of a flat `trap_cost`, and let a
2-3-long dragon in if the pocket's pearls would grow it past the split-capable threshold.

## What changed from the parents, and why

- Renamed `cs36` -> `cs37` (namespace, macros, file names) so the variant loads independently.
- In `ChooseGoal`, once `RoomAround` and the escape check both say a candidate goal is a true
  dead end (not `escapes`), we now count the pocket's pearls (tiles worth >= 0.5) and price
  the trap:
  - a dragon already long enough to split (`length_ >= 4`) pays `trap_stub` (2.0) instead of
    the flat `trap_cost` (3.0) -- entering costs only the stub it leaves behind, not the whole
    body, since it can split out.
  - a 2-3-long dragon that can't split now is let in anyway, at the same `trap_stub` cost, if
    the pocket's pearls would grow it to length >= 4 with a free unit slot to split with
    (`FreeSlots() >= 1`); this sets a new `grow` flag on the goal.
  - anything else (short dragon that stays short, or no free slot) keeps the flat `trap_cost`.
- `SafeStep`'s existing `trap_small` gate (which normally refuses to send a too-short dragon
  into a dead end at all) is relaxed by the new `trap_grow` flag, mirroring gst96's relaxation
  of `SafeStep`'s `trap_small` for this case.
- New params: `trap_loss` (1: on, matches gst96's flag), `trap_stub = 2.0`.

## What was measured

Built and ran `check counter-swarm-37 maps/official`: all checks passed, slowest turn 27.8ms
region is unrelated pre-existing hotspot on schooltime/stronghold (well under the 500ms budget;
same as before the change).

`sim`, `--threads 2`, both seats per map:

- vs **counter-swarm-36** (direct parent), maps/official: **11-0-11** (was the baseline to beat;
  even). By map: Devil A-win/B-loss, Default loss/loss, Trophy loss/win (unchanged, mixed).
- vs counter-swarm-36, 20 worldgen maps (`--seed 41000`, default style), both seats (40 matches):
  **20-0-20**.
- vs counter-swarm-36, 20 worldgen maps (`--seed 42000 --style labyrinth`), both seats (40
  matches): **19-0-21** (2 games worse than parity out of 40 -- within noise).
- vs gather-split-trade-96, maps/official: **13-0-9** (better than the flat-cost cs36 baseline
  would suggest; the pocket-pricing helps against gst-lineage's own trap style).
- vs trading-swarm-6, maps/official: **17-0-5**.

Overall: at least even with the direct parent (counter-swarm-36) on official maps and on
worldgen maps of both styles, and clearly ahead of the other two comparison variants. No
regression found; submitting.

## What we learned

- cs36's tail-following escape check already subsumes gst96's main dead-end fix ("room to
  follow the tail out costs nothing"); porting that half again would have been a no-op or a
  duplicate flag. The part worth porting was the stub-cost pricing and the short-dragon
  grow-in relaxation, which cs36 didn't have.
- The change is a wash on raw margin vs the direct parent (roughly 50/50 both official and
  worldgen), but clearly helps against opponents that build long-lived economics around dead
  ends (gst96, ts6) -- likely because we now enter more dead-end pockets (48 "into a dead end
  worth it" actions this match vs cs36's baseline count) rather than avoiding profitable ones
  out of over-caution from the flat trap_cost.

## What we'd try next

- Tune `trap_stub` and the pearl-growth threshold (currently hardcoded to `length_ + pearls >= 4`)
  against a wider opponent pool; we didn't sweep parameters this task per the budget.
- Look at whether the `grow` relaxation should also consider ally crowding (a 2-3-long dragon
  growing to 4 inside a pocket while an ally needs the same space).
