# ring-sched-fg

ring-sched-6's code (`variants/ring-sched-m1`) with two gates rewritten from the g39 gate audit (see `variants/ring-clean-1/AUDIT.md`). This is the audit's "control" build.

- **Forage gate.** Before: `units_<=2 && length_<=3` at r0. Now the queen forages when she can't split (length < 4), no ally in view can split, and the team has fewer than 6 dragons.
- **Give-up-slot.** It no longer assumes UNIT_LIMIT 64 (kGiveUpMinUnits=32 removed); it uses free slots only.
- **The centre pull is kept,** flagged as a map-shape assumption, by the user's decision (2026-10-05: keep it for now).

Light duel vs ring-sched-6 (server, random:2): 37-31. Removing the centre pull instead cost about 13 points (28-40).
