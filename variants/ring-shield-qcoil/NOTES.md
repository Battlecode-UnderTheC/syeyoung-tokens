# ring-shield-qcoil (g53-21, arena ring-shield-16)

Base: ring-shield-rush (arena ring-shield-10). The parent's notes are in variants/ring-shield-rush/NOTES.md.

## Idea (strategist C g53 #2: the late queen doesn't harvest into a pocket)
Trace first. Arena 276351 reproduced exactly (match, maps wg40013 seed 1592614637, A v ring-field-1; the
base plays it identically): at r465 her 5-step harvest sprint ended at (6,15), the top edge of her 7x7
view, and QueenSpace scored 63 (= need, the cap). Debug counters on that flood: 54 of the 63 tiles were
out of view, 21 of those held a REMEMBERED ALLY body, 0 unseen edges. The flood leaked through row y=13
(out of view) across remembered ally bodies (ids 1900/1909/1891 "bed: step around its tick" / "toward
goal"). At r466, seeing that row, every move scored 1-11 (stale 0): an 11-tile pocket sealed by those
allies and kelp. She coiled in it r466-470 and split 21 -> 7 (r471-477, own body boxing her).
So the over-count is stale memory: QueenSpace treats every out-of-view tile as free, whatever was last
seen there. (Unseen-edge credit was not involved here; in two other cascades it was: wg40010 B r450
unk3 = 12 of 20 tiles; arena duel game 59 maze r470 unk15 = 60 of 90.)

## Change (strategy.cpp QueenSpace; QCOIL_MODE=0 is the parent)
From kRaceFrom (r430) an out-of-view tile whose last sighting (at r430 or later) held an ally body
blocks the queen's room flood, as a body in view does. Enemy bodies and pre-r430 memories still read free.

## Local
- 276351 repro: "queen: keep room" at r465 instead of the pocket; queen 20 -> 62 at r499 (lost 62-67).
- C's 10 cascade maps x 2 seeds x 2 seats v ring-field-1 (40 games): queen split-2 turns r440+ 55 -> 34,
  games with 3+ 8 -> 6; BUT wins 27 -> 17, queen deaths 11 -> 16, q499 mean 31.6 -> 22.8. All 10 games
  whose result changed went to the base. Mechanism (maze 8619.. B r448 trace): remembered ally bodies
  near her are usually transient (moving workers, conveyor feeders who lay down as pearl lines); blocking
  them vetoed (space < L+2) the route the base took north and grew 20 -> 68 on.
- Fresh mapgen (seed 5321, 44 maps, all families) x seeds 1,2 v ring-shield-rush: 86-4-86, identical
  stats (most games end ~r300; the change acts only r430+).
- check maps/official: pass.

## Arena
- Duel server (random:2, 68 g) v ring-shield-10: 35-0-33 (noise); queen alive@end 61.8% v 58.8%,
  queen length 12.0 v 11.5. Queen split-2 r440+ (decoded): 19 v 5, but 16 are one game (59, maze: base's
  queen already dead at r450; ours grew to 58 in a maze dead end counted open by unseen-edge credit, shed
  to 26 and still won 26-0). r470-499 growth (both alive): +7.1 v +8.3.
- Duel official (random:2, 20 g): 11-0-9; queen length@end 12.7 v 9.0.
- Draft ring-shield-16 v ring-shield-10: 55-1-56 (49.6%, "loses to parent, within noise"); server 33-0-35,
  custom 17-0-17, official 5-1-4; v ring-sealed-2 67-45, ring-field-3 62-50, ring-qforage-1 68-44.
  Eval P(win) v parent 0.500/0.500/0.500 at r100/200/300. Queen split-2 r440+ over 112 decoded games: 7 v 7
  (one 3+ cascade each, the mirrored weakhold pair 287144/287173); r470-499 growth +6.7 v +6.7;
  longer-queen 21-23. CPU p99 26.6M, max 50.4M.
- Verdict: the over-count is real, but this fix doesn't move the arena, and locally it costs games
  (ally bodies near her are transient). Next: the unseen-edge credit in dead ends (game 59, wg40010), or
  an ally-freshness test, not blanket blocking.
