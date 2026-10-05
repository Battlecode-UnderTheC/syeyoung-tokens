# ring-sched-m4 (g41 merge step)

This is ring-sched-m3 (ring-sched-17, the live bot) with the two g41 changes that didn't lose, applied as exact diffs with `patch`:
- **rescue.cpp dangling-else fix** (ring-rescuefix-1 = ring-sched-18, 63-55 v ring-sched-17). `would_close` now checks moves.
- **Late queen pocket guard** (ring-qpocket-1 = ring-sched-21, 56-56). From r250 the harvest pick avoids 1-exit pockets, and the queen11 wait splits at most once per 8 rounds.

The other g41 drafts were within noise or inert, so they're not stacked: opening runs, middle trades, mirrored raids, the consolidation gate and frontier-2.
