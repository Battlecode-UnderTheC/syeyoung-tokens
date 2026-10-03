# Server maps

These are the 17 maps the competition server plays, as bundled with **unswbc 1.2.9**:
`unswbc maps <dir>` writes all 22 bundled maps. They replace the copies rebuilt from Sep 30
replays, which went stale when 1.2.7 changed 8 maps.

**Checked against 292 server replays from 2026-10-03:**
- The walls (EDGE lines) are identical on 16 of the 17 maps. Schooltime differs by 8 edges,
  probably a change after Oct 3.
- The dragon lines differ on some maps only by which team holds which seat, so run both seats.

**Relation to the other map sets:**
- These files are byte-identical to the same names in `maps/official` (1.2.7 = 1.2.9) and
  `maps/tournament-sprint`, where `around_unsw.map` = `unsw.map`. This folder just selects
  the maps the server plays.
- Not played on the server in our battles: Colosseum, arena, big_empty, default_small,
  stronghold. Those are in `maps/official`.

**To refresh after a toolkit update:** `uv tool install unswbc==<ver> --force`, then
`unswbc maps <tmpdir>`, and copy the server-played maps here. Spot-check them against a
recent replay's map text with `decode.load(p)['map']`.
