# swarm-hunt-26

## Idea

Crossover of swarm-hunt-25 (base) and body-wall-21: port bw21's idle-stub
slot give-up at the unit cap (its step 4b in Turn) into swarm-hunt-25's Turn
loop. Every trapped death of length >= 4 happens at the cap where no escape
split is possible (league/KNOWLEDGE.md, "The cap trap is set before the cap
is reached"). bw21 fixed this by having an idle short stub resign to free a
slot for a longer, trapped ally; the same port into gather-split-trade-70
cut cap-trapped deaths ~7x on one worldgen set while staying even with its
parent.

## What changed from the parents, and why

- Renamed the whole namespace `swh25` -> `swh26` and every `SWH25_*` macro
  to `SWH26_*` so the variant doesn't clash with swarm-hunt-25 when both are
  loaded into one simulator (parents copied as-is otherwise).
- Added bw21's 4b block verbatim (constants and logic), placed exactly where
  bw21 has it: right after the goal (`target`/`goal`) is computed in Turn,
  before the loop breaker. Condition: length 2-3, not a keeper (swh25's
  equivalent of bw21's `leader_`), <= 1 free slot, > 32 units, not the
  champion, not already feeding an ally, and idle (no goal, or a goal more
  than 6 steps away). It resigns if it sees an ally of length >= 6 within
  Manhattan distance 5, or 3+ other heads at all. `UNIT_COUNT` (via
  `FreeSlots()`) is live within the round so this naturally throttles to
  about 2 resignations/round, matching bw21.
  - swarm-hunt-25 already had a narrower, adjacency-only cap mechanism
    ("cap: feed ally": a short hunter next to a longer ally gives up its
    slot). This new check is broader (Manhattan 5, or just a crowd of
    heads, no ally requirement) and only fires when idle, so it's additive,
    not a replacement; both actions show up in the reason tally.
  - `keeper_` (swh25's leaders-at-start) is the mapping used for bw21's
    `leader_`.

## What I measured

- `check swarm-hunt-26 maps/official`: all checks passed, 0 avoidable
  deaths, slowest turn well under the 0.5 ms budget on the maps that
  matter (the two >10 ms readings on `big_empty`/`stronghold` are the
  memory-audit build's overhead and matched on swarm-hunt-25's own
  `check` run too, not a regression from this change).
- `sim --a swarm-hunt-26 --b swarm-hunt-25 --maps maps/official --threads 2`
  (22 matches, both seats): **12-0-10**, swarm-hunt-26 leads by 2.4 longest,
  1.6 total at game end on average. New reason tags fired: `give up slot:
  crowd` (~8/match) and `give up slot: long ally` (~1/match).
- `sim --a swarm-hunt-26 --b swarm-hunt-25` on 20 fresh worldgen maps, both
  seats (40 matches): **20-0-20** (exactly even in W-D-L, as expected on
  mirrored seats), total-length margin +2.9, longest-length margin -1.6.
  Cap-trapped fatal turns (`trapped: no move/split *@cap`) dropped from
  ~12.3/match on swarm-hunt-25 to ~2.6/match on swarm-hunt-26 — the same
  direction and rough size of effect the gather-split-trade-70 port saw.
- Did not run trading-swarm-5 in this pass (time budget); the brief's
  headline bw21 result (83-19 vs trading-swarm-5) was measured on a
  different archetype and I didn't have time to re-verify it here.

## What should improve

Fewer cap-trapped deaths for swarm-hunt's longer dragons late-game when the
team is near the 32-unit floor and the cap is binding, without giving up
growth splits (the brief's earlier finding that skipping growth splits to
keep slots free was worse was respected: this change only touches idle
stubs after the goal/split decision, never suppresses `split`).

## What I'd try next

- Run the trading-swarm-5 matchup and a longer worldgen batch to confirm
  the effect holds outside the swarm-hunt/bw lineage.
- The longest-length margin went slightly negative on worldgen (-1.6) even
  though total length improved (+2.9) and cap-trapped deaths fell sharply;
  worth checking with `--trace` whether resigned stubs are occasionally
  ones that would have grown into the eventual champion, which the
  `champ.id != spawn_.id` guard doesn't catch mid-round before a new
  champion is decided.
