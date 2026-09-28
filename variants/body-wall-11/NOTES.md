# body-wall-11

## Idea

Sonar threat warnings. Before r330 a dragon that sees an enemy head announces the longest one
(enemy id, head tile, visible length, round) as its own word type (magic 0x5C) on the two
directions not carrying the champion word (alternating N/S and E/W by round; the portal probe
and obituary pings still override). An ally that hears of an enemy head it can't see, heard
at most 1 round ago and within (its length - 1) + 6 path steps, walks at it (`threat: approach`)
when it is worth a trade (at least as long, same rules as the kamikaze) or when the ally is at
least 2 longer and >= wall_min_length (to wall it); once in view, the usual lunge / wall logic
takes over. Only a safe step (SafeStep says "toward goal") is taken, else normal play.

## What changed from the parents, and why

- namespace/macros bw10 -> bw11.
- Encode takes a magic; Decode stores threat words in `threats_`; `Threat()` (step 2d in
  Decide, after the wall, before splitting); `WorthTradingLength()`.
- Params: threat_ping, threat_until 330, threat_slack 6, threat_age 1, threat_hunt_shorter 1.
- Debug-only (BW11_DEBUG, compiled out) death tally in the agent destructor.

## Measurements

Headroom (BW11_DEBUG, 40 worldgen matches vs body-wall-10): of deaths not caused by our own
act (not feed/trapped/kamikaze/lunge/trade/split/wall on the fatal turn), 84% had an enemy
head in view the turn before (2405 of 2861) and 16% had none (456, ~11 a match); only 55 of
those 456 had heard a threat ping in the last round. So the "blind" death headroom is small,
and threat pings reach the dying dragon rarely.

With the brief's radius (slack 2, trade-worthy heads only) `threat: approach` fired 0 times in
40 matches: a 2-4 long worker's reach + 2 is mostly inside its own 7x7 view already. Slack 6
plus hunting shorter heads (the shipped setting) fires ~48 times a match.

Shipped settings, both seats:
- vs body-wall-10: official 13-0-9 (+1.2 longest, +21.4 total); worldgen set 1 (20 maps)
  23-0-17 (+4.7, +27.9); worldgen set 2 (20 maps) 21-0-19 (-1.2, +22.9). Total 57-45.
- vs gather-split-trade-41: official 10-0-12 (-0.5, +5.4); set 1 17-0-23 (+0.1, +0.8);
  set 2 19-0-21 (+0.3, -7.5). Total 46-56. (body-wall-10 vs gst41: official 12-10, set 1 18-22.)
- slack 2 / no shorter hunting (debug build) vs body-wall-10, set 1: 22-0-18 (feature
  never fired, so this is the sonar split alone: champion on 2 directions is free, as
  sonar-pack-2 found).

## What should improve

Slightly more lunges/walls on enemy heads just out of view; mostly neutral. Expect about
parent strength; the length margins vs the parent are positive on all three sets.

## Didn't work / open

- The brief's radius doesn't trigger at all for small workers. Safety from heard heads is a
  no-op in this line: heads at least as long are trade-worthy (no avoidance), and Exposed only
  looks at adjacent tiles, which are always in view.
- Relaying threats (multi-hop) was not tried; the heard rate at death is ~12%.
