# ring-sprint-qdead (g44-4; parent ring-sprint-1; arena ring-sprint-8)

## Idea
arena-r18 S1: 172 losses (C2) where our queen dies after the enemy queen is already dead. Once
she is dead any living queen of ours wins the queen tiebreak, so only our queen's survival counts.
The base still fed her by resigns, walked her out of the ring at r430 and let her roam for food.
Code read: EnemyQueenRace() is true for a never-seen queen; enemy_queen_dead_ is set only when she
vanishes beside a head of ours; nothing parks the queen.

## Change (one behaviour: no live race -> the queen stays safe)
- Death detection (strategy.cpp Remember): she is dead when her HEAD was in view last turn (new
  `Sighting::head_round`; an older head from a partial sighting gave false deaths in sim), that
  tile is inside our 7x7 window now, and no segment of hers is in view. This replaces the g42-4
  beside-the-head check. In sim (50 games on server/official maps) there were 7 detections, all real.
- Relay (ring.cpp RingWord, strategy.cpp Decide): a known death rides in bits 26-34 of the ring
  word with the valid bit (35) clear; receivers keep the max.
- EnemyQueenDead() (a death later than any sighting) ends the race: no Ring4Exit, no ring4
  supply, no qrace feeding.
- Parking (QueenMove): without a live race (known dead, or last seen before kQGateFrom), outside
  the opening harvest, the queen's beam gets no food term, a 1-step limit unless a threat covers
  her head, and a soft leash (1.0 per tile beyond 2) to her park tile. Label `qdead: parked`.
- Unlike the brief's literal fix, a never-seen queen still counts as a race (see v1).

## Iterations (duels v ring-sprint-1, server + official, random:2)
- v1 (brief-literal: never seen = no race, park whenever there is no race): 36-32 / 9-11. The
  target behaviour was fixed in games where the enemy queen died first (our queen v the base's
  queen in the same games): multi-step moves 0.8 v 11.2, roaming 4.9 v 7.4 tiles, C2-type losses
  2 v 6. But queen-race (B) losses rose 16 v 10. Races where she was never seen by r430 went 3-9,
  against the base's 9-3: stronghold x4, trauma, maze, where the base grows its queen from 4 to
  50-68 after r430. v1 also had the false deaths from stale heads.
- v2 (park only when known dead, old detection): every duel game matched the base exactly.
- v4 (SUBMITTED as ring-sprint-8): fresh-head detection over the whole window; never seen = race;
  park when there is no race. Duels: server 35-33 (3+ starters 20-20, <=2 15-13), official 9-11.
- v5 (local only: park only when known dead): fresh maps 128-4-132, every family 10-14 or closer.
  It is nearly inert, so it was not submitted.

## Arena verdict (ring-sprint-8)
- vs ring-sprint-1 52-0-60 (46.4%, loses within noise); server 33-35, custom 14-20,
  official 5-5. Decided by: elimination 27-30, longer queen 16-18, longest 9-12.
- v ring-sprint-6 49-63, v ring-contest-4 60-52, v ring-sched-17 66-46.
- CPU p99 24.2M, max 41.6M. Mean eval P(win) v parent: r100 .486, r200 .486, r300 .490.
- Target, from the same 112 games (our queen v the parent's queen):
  - games where the enemy queen died first: 40 v 49; our queen died afterwards in 12 v 17;
  - multi-step moves after her death: 1.3 v 2.9; roaming 5.0 v 5.7 tiles;
  - in those games' losses: roaming 7.5 v 10.1, multi-step moves 0.2 v 7.9;
  - C2 losses 3 v 3, B 8 v 8, D1 10 v 8.
- Replays:
  - islands 197385 (ours) v 197382 (parent): enemy queen died at r340. Ours grew 4->13, roamed 9
    tiles with no multi-step moves and died at r466. Theirs grew 4->19, roamed 23 tiles with 40
    multi-step moves and died at r442.
  - autarky 197273 = 197249: enemy queen died at r441 with no surviving witness. Our queen
    behaved exactly like the parent's: race on, 24 multi-step moves.
  - fixed-centre-trap 197145: tiny queen, threats kept moving her park tile; roamed 22 tiles.

## Why the target is only partly fixed
- Most enemy queen deaths are head-on trades with our attacker, which dies too. In sim, no
  surviving dragon of ours had her in view in the 3 rounds before 18 of 25 deaths. So the death
  stays unknown, and a sighting from r300 on still reads as a live race.
- Parking acts only in the QueenMove fallback. A queen in her ring keeps cycling, and a threat
  moves her park tile.

## Local (fresh mapgen maps seed 44417 + 44707, 66 maps, seeds 1,2, v4)
127-4-133; queen tiebreak 16-29 (side effect: parking on a stale sighting while the enemy queen
lives). Per family (24 games each): anti-centre 10-0-14, crowded 12-0-12, maze 9-4-11,
mixed 9-0-15 (queen 3-7), off-axis 13-0-11, open 14-0-10, rich-near-home 13-0-11, ring 14-0-10,
rooms 12-0-12, sealed-spawn 10-0-14, sparse 11-0-13. check maps/official passes.
