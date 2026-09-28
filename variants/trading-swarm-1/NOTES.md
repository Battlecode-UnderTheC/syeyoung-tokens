# trading-swarm-1

## Idea

A faithful, simple clone of the first real server opponent (split-v7, battle M489641,
archetype `trading-swarm`), so the league can measure our variants against the style that
beat gather-split-trade-27 on small crowded maps. No sonar, no memory, window-only.

Per dragon, each turn, in order:
1. an enemy head adjacent: step into it (even trade), whatever its length (dragons > 8 only
   trade with enemies at least as long);
2. a 3-6 long dragon lunges 2-4 steps (≤ length-1) along free window tiles onto an enemy head at
   least as long as itself (the longest such target);
3. length ≥ a per-dragon threshold drawn from {4: 30%, 5: 40%, 6: 30%} and ≤ 8: `SPLIT 2`
   (new threshold after each split); longer dragons split 2 with 1/40 chance a turn;
4. greedy BFS to the nearest pearl in view;
5. else toward the visible bed that ripens soonest (countdown - distance ≤ 8, at distance ≥ 2);
6. else drift toward the map centre (the central pearl fields), else straight on / random turn,
   preferring steps with a little room; if boxed in, die anywhere but an ally's head.

## What changed from the parents, and why

Fresh (no parents). Behaviour read from the replay analysis (digest.txt: every B split is
`SPLIT 2` from parents mostly 4-6 long; trades.txt: lunges and step trades; KNOWLEDGE.md
'Real opponents'). The bed/centre fallback is mine: without it the clone sat at length 3
for tens of rounds on default_small and never split.

## What I measured (sim, both sides; official = 11 maps x2, worldgen = 20 fresh maps x2)

| opponent | official | worldgen |
|---|---|---|
| anchor-random | 18-3-1 | 39-1-0 |
| gather-split-trade-27 | 9-0-13 | 5-1-34 |
| late-surge-8 | 3-0-19 | 6-1-33 |
| swarm-hunt-9 | 5-0-17 | 2-2-36 |
| counter-swarm-16 | 5-0-17 | 3-1-36 |

Official maps by name (W-L of the 2 sides) vs gst-27 / late-surge-8 / swarm-hunt-9 / counter-swarm-16:
devil 2-0 / 2-0 / 2-0 / 2-0; trophy 2-0 / 0-2 / 1-1 / 2-0; default_small 0-2 / 0-2 / 2-0 / 0-2;
arena 1-1 / 1-1 / 0-2 / 1-1; Colosseum 0-2 / 0-2 / 0-2 / 0-2.
vs gst-27 it also wins queen_of_spades 2-0 and loses schooltime, big_empty, stronghold, trauma, default.

Real opponent vs gst-27 on the server: 5-5, won Autarky, Devil, Prisoners Dilemma, Schooltime,
Trophy. The clone reproduces the Devil and Trophy wins (by elimination, as the real one did)
but not Schooltime, and loses the big maps on longest (its longest stays 5-6).

## Where it differs from the real one (known)

- It eats less: the real one ate 3-11 pearls/100 moves and split 100-650 times a game; the
  clone splits far fewer times, so it is a weaker swarm, especially on worldgen mazes.
- Its longest never grows much (splits at ≤ 8), where the real one's grew late by eating
  corpses; so it loses big maps on longest instead of winning them by lunging our champion.
- Avoids occupied tiles when moving (less sloppy than the original, for the check gate).

## What should improve

Nothing of ours; it is a sparring partner. Next fidelity steps: let lunges target any long
enemy (≥ 8) from any short dragon, and a better eating loop (the real one's revisit rate was
low) so the split rate reaches ~1/round.
