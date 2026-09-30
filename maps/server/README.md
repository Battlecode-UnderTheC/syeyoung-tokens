# Server maps

Rebuilt from the map text inside real server replays (battle M665147, 2026-09-30) by strategist w. They carry the server's dragon placement: e.g. Prisoners Dilemma has 10 dragons here against 6 in `maps/official/dilemma.map`, and on 4 of 5 maps the server's team 0 is the local file's team 1. Bed ranges follow the local files.

Use them with several seeds and both seats. Report economy numbers (hot-bed and corridor pearls, dragon count at r50/r100, total length) next to W-D-L: a local win on longest dragon alone has hidden server losses. The real server seed of each game is in its replay (`decode.load(p)['seed']`).

## Added 2026-09-30 (territory team)
default, devil, portals, trauma and trophy come from battle M588431's replays (M588432/33/34/39/40): the local official file with its DRAGON_COUNT and DRAGON lines replaced by the replay's. Across all 72 server replays, TILE and EDGE text never changes between battles. Only the dragon placement varies, and on most maps only by the two teams swapping seats (sim's both-seats covers that). Prisoners Dilemma is the exception: 6 dragons in early battles, and 10 (4 extra length-2 dragons) since M561834. Replays show bed ranges as "0 0", so ranges come from the local files. The server has never played Colosseum, arena, big_empty, default_small or stronghold in our battles; for those, use maps/official.
