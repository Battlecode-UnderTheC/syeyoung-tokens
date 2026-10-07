# How dev test 1 and aAah play (watched, 2026-10-07)

**Source.** 30 recent battles per team against opponents rated ≥2150, played on 10-07 (dev test 1: 386 games; aAah: 389). I watched whole-game contact sheets of 10 games each (one win and one loss per map type where available) plus opening films. The studied team is always **cyan** (`render.py --us`). Every claim below names the film it comes from, in `img/style/`. The only numbers are the ones printed in the frame titles (dragon count n, longest L) and a queen-length lookup used to tell which ringed dragon was the queen.

## dev test 1: a heavy spearhead with the queen at its tip, a flood, then a late champion
1. **Starters stay heavy and fan out at once.**
   - `dt_devil_open` r0-r12: the five starters keep length 4-6 instead of splitting to stubs.
   - They split off in different directions in the first 6 rounds: one up a column, one down, the rest along the centre row.
2. **The queen leads the main push into the densest bed field.**
   - `dt_devil_open`: queen `0:4` heads straight along the centre row (r6, r12) and is eating at the edge of the central field by r18 (`0:5`).
   - From r24 to r48 she is the longest dragon on the board (red ring, `0:6`, `0:7`), in the middle of the field, with her own bodies around her.
   - `dt_trophy_open`: the queen walks under the cup's left arm and is inside the cup by r18 among her own dragons. She splits (`0:4` → `0:2` at r30) and stays in the cup's lower half.
3. **It reaches the bed clusters by the shortest committed route, including across the torus.**
   - `dt_trophy_open` r0-r12: starter 2 runs along the bottom edge, wraps to the top of the board and enters the cup through its top portal (`2:4` inside the cup at r12).
   - By r30 the cup holds 9 cyan dragons.
4. **The mid-game is a dense mass that floods outward.**
   - `dt_Devil_W_1334135`: a solid column of cyan in the central lanes by r27-r54, then the whole map. Eliminates at r203 with 54 dragons.
   - `dt_Trophy_W_1334151`: the cup first, then outward to 60 dragons by r128; eliminates at r160.
   - The mass is a blob, not a grid: bodies touch, and the front is a wall.
5. **The late game: if no queen race is on, it builds one champion by sacrifice.**
   - `dt_QueenOfSpa_W_1329975` (v forgot to mention): about 38 dragons cover the map until r299. Then the count falls 21 → 8 → 3 while one dragon grows L19 (r333) → 29 → 43 → 54, running along the board edges among red death marks. It wins the longest tiebreak.
   - The same in `dt_Portals_W_1334156` (L21 r399 → 45) and `dt_Schooltime_W_1334150`, where the champion lives in the walled bottom bed room (L28 r299 → 62 r399).
   - In these games dev test 1's own queen is already dead (queen-length lookup: 0 by r300-r400). The champion is a worker.
6. **Sometimes it gives up the contested middle and builds at home.** `dt_Trauma_W_1335380` (v bread first search): it contests the maze r100-r200, then pulls back into its open base on the right while bread first search floods the maze (59 dragons). It grows a 39-long dragon there and wins.
7. **How it loses.**
   - `dt_Devil_L_1334152` (v Preyas): both sides rush the centre; Preyas wins the contact fights at r46-r76, and dev test is pushed out and flooded.
   - `dt_Trophy_L_1330002` (v SSS): SSS is in the cup first (r15) and dev test never gets in.
   - `dt_Portals_L_1331393` (v forgot to mention): forgot grows its queen in its half from r266 (12 → 41). Dev test's queen is dead and its champion starts too late (15 at r399 → 24), so it loses on queen length.
   - **It loses the race for the cluster, or it loses the queen tiebreak once its queen has died at the front.**

## aAah: atomise, carpet the whole board, keep the queen tiny inside it
1. **It splits everything to length 2 at once.**
   - `aa_devil_open` r6: "C n6 L2". Every starter has already split down to 2.
   - The dragons then spread out singly over separate lanes: by r12-r18, heads sit one per column across its whole half.
2. **The queen goes in with the swarm, small and behind its tip.**
   - `aa_trophy_open`: queen `1:3` crosses toward the cup by r6-r12 and is inside the cup by r18. At r24-r48 she sits near the cup's middle and top (`1:4`, `1:2`), surrounded by 10-20 cyan dragons.
   - `aa_devil_open`: queen `1:2` stays on the right through r18, then moves into the centre-right lanes inside its swarm (r30-r48), never at the front.
   - She stays 2-4 long in every film (queen lookup: 2-4 at every checkpoint in all 20 games). Its queen never grows.
3. **It carpets, not blobs.**
   - `aa_Devil_W_1334743`, `aa_QueenOfSpa_W_1334748`, `aa_Schooltime_W_1335033`: single dragons, mostly 3-5 long, evenly spaced over the whole board, with gaps between them.
   - On Schooltime (60x40) the carpet covers nearly the whole map by r200 (55-64 dragons); SSS is reduced to its spawn corner.
   - It takes bed rooms early: `aa_QueenOfSpa_W` holds the bottom room from r16.
4. **Its dragons survive alone.** `aa_Devil_L_1334397` (v Cultery): from r178 its last dragon, a 2-long worker (its queen was already dead by r100), survives alone at the left edge among 40-59 Cultery dragons until r382, about 200 rounds.
5. **How it loses: territory without a finish.**
   - `aa_Trauma_L_1334753` (v forgot): aAah carpets the maze (30-59 dragons). Forgot fences a solid block of bodies in its open base and grows its queen inside it, from 7 (r233) to 56. aAah never breaks in and loses on queen length (its queen 2).
   - `aa_Portals_L_1334746`: the same; forgot's queen 13 (r266) → 43 inside a solid orange block.
   - `aa_SlitheryFi_L_1334750`: aAah reduces forgot to 5 dragons but never kills the queen hiding on the right edge, and loses on length.
   - `aa_Schooltime_W` shows the same failure to finish even in a win: SSS's last dragon survives in its corner from r299 to r499.

## The two side by side
| | dev test 1 | aAah |
|---|---|---|
| Early dragon size | Keeps starters 4-6 long and fans them out | Splits everything to 2 at once |
| Queen early | Leads the push into the bed field at 4-7, the longest dragon on the board | Goes in with the swarm, 2-4 long, behind the tip |
| Formation | Dense blob with a wall-like front | Even carpet of singles with gaps |
| Bed clusters | Shortest committed route, including torus wraps and portals; in the cup by r12-r18 | In the cup and rooms by r16-r18 |
| Late game | Converts to one champion by sacrifice (r330-r400 on); sometimes retreats to build at home | Keeps the carpet to r499; never grows queen or champion |
| Typical loss | Loses the cluster race or the contact fight at the front; queen dies at the front, then loses the queen tiebreak | Out-territories the opponent but loses queen length to a fenced-in fortress queen it can't break or hunt |

## What this means for us
- **Both teams do what our report found missing: they go to the dense bed field in the first 10-20 rounds, with the queen inside the moving swarm.** Their mid-games differ (blob v carpet), but the opening is the same idea. On Devil and Trophy the game is decided by who gets there first (`dt_Trophy_L`, `dt_Devil_L`, `aa_Devil_L`).
- **Neither has an answer to the fortress queen.** forgot to mention beats both on queen maps by growing its queen inside a solid block of its own bodies (`aa_Trauma_L`, `aa_Portals_L`, `dt_Portals_L`). That is the Living Queen idea (REPORT §8), and the two best carpet/flood teams lose to it.
- **There is a gap between them that nobody fills:** aAah's territory plus a fortress queen grown behind it. aAah already holds the ground a fortress needs; it just never feeds a queen.
