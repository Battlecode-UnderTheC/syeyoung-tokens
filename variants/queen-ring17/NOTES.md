# queen-ring17 (import)

This is our teammate bu-gye-jeong's arena bot `queen-ring17-first-then-grow-1`, imported
unchanged so round 10 (g35) workers can build on it. It is live on the competition server
(77-0-75) and #1 on the arena (1591).

How it was imported:
- the source comes from `tokenz get` (via strategist B's scratch copy);
- only the bot-specific files are kept;
- the shared files are byte-identical to `shared/`, except for the `core.hpp` include path.

Strategy: a gst-derived economy. The queen parks in a 6-cell ring and grows late (exit at
r430, nearby donors from r450).
