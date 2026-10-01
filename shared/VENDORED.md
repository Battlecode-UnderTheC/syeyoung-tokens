# Where the shared code came from

Copied on 2026-09-28. Keep in step by hand; record each update here.

| Here | From | Commit |
|---|---|---|
| `engine/dragons/{core.hpp,board.cpp,game.cpp}` | dragons-env `cpp/dragons/` (the port of the official unswbc 1.0.0 engine; plus the `Death.by` field) | dragons-env 7a15f42 + the uncommitted `Death.by` change |
| `protocol/observation.*` | syeyoung-rl `train/native/` | syeyoung-rl e111d4d |
| `bot/protocol_io.*` | syeyoung-rl `strategy/src/` | e111d4d |
| `bot/agent.hpp` | syeyoung-rl `strategy/src/agent.hpp`, plus a virtual `reason()` | e111d4d |
| `lib/board_memory.*`, `lib/pathing.*` | syeyoung-rl `strategy/src/` | e111d4d |

The engine must stay faithful to the official one: the lockstep test that proves it is
`dragons-env/tests/lockstep.py`. Re-run it before trusting results after an engine change
or a new unswbc version (1.1.0 is out; this is 1.0.0).

**2026-10-01:** the engine now follows unswbc 1.2.3 (branch tool/engine-1.2.3, merged): ceil(L/4) free sprint steps, the queen-first win order at round 500, and the team-alternation check on DRAGON lines. Verified 60/60 games identical to `unswbc run --sandbox` 1.2.3 on one bot pair.
