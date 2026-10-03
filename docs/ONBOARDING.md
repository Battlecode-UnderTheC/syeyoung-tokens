# Onboarding: improving our dragons bot

For teammates, Codex, and any LLM agent joining the work. Read this first; it is self-contained.

## The game in one paragraph

UNSW Battlecode "dragons" (toolkit **unswbc 1.2.9**; full rules in [`docs/game.md`](game.md)). Two teams of
snake-like dragons play 500 rounds. Dragons eat pearls to grow, split to make more dragons (max 64 per team), and
die by moving into walls (kelp), bodies, or head-to-head trades. Your **queen** is your dragon with id 0 or 1. At
round 500 the winner is the team with the **longer queen** (a dead queen counts as 0), then the longest dragon, then
total length. A team with no dragons left loses immediately. In a multi-step move, the first ceil(L/4) steps are
free and each later step costs a segment. Each bot gets 100M CPU points a turn and 48 MB, runs as WebAssembly, and
may not read environment variables.

## What predicts winning

From ~8,500 arena games and ~900 real competition games (details on the arena's `/docs` → METRICS):

- **Rounds 50-350:** board control (the share of tiles your heads reach first) and how many dragons you have alive.
- **Rounds 400+:** the longest dragon decides about 95% of games that reach round 500, when both queens are dead.
- **Among the top teams, the queen matters.** A quarter of their games are decided by the queen, and a team whose
  queen is the only one alive at round 300 wins about 84% of the time. In our games, both queens die about 93% of the
  time.
- Kills and deaths on their own predict nothing.

## Where things live

| What | Where |
|---|---|
| The team arena (verdicts, ratings, replays, metrics, every bot's source) | https://tokenz-arena.lonk-kelvin.ts.net (Tailscale only; ask syeyoung for access) |
| Its guide for agents | `/docs/agents` on the arena, and `/llms.txt` (the whole API on one page) |
| The arena client | `curl -fsSL https://tokenz-arena.lonk-kelvin.ts.net/tokenz.py -o ~/.local/bin/tokenz && chmod +x ~/.local/bin/tokenz`, then `tokenz --help` |
| What we've learned (findings, dead ends, measurement pitfalls) | [`league/KNOWLEDGE.md`](../league/KNOWLEDGE.md) in this repo. Read its last sections before starting. |
| Bot source in this repo | `variants/<name>/` on branches; every finished bot is tagged `variant/<name>` |

## Versions: how they are managed

- **In git:** a bot is a folder `variants/<name>/` holding the code, `variant.toml` (parents, operator, author) and
  `NOTES.md` (idea and measurements). Names never change: a changed strategy is a new name. Work happens on your own
  branch, ideally in a git worktree, and a finished bot gets the tag `variant/<name>` (made by `scripts/submit.sh`,
  which checks it). Most variants live on branches and tags, not on `main`.
- **In the arena:** every upload is an immutable submission `<line>-<n>` with a parent and a note. Drafts play their
  parent and the top bots on official, server, tournament, custom, weekly-rotating and hidden maps with the official
  judge. A draft is **promoted** (ranked) only if it beats its parent, and promoting retires the previous version of
  that line. The arena is the source of truth for "which bot is better".
- **On the competition server:** our live version is linked in the arena (`tokenz server versions`). Uploads happen
  only through the arena's auto-submit gate or through syeyoung, at most once every 4 hours.

## The loop (no repo needed)

1. `tokenz leaderboard`, and pick a ranked bot to build on.
2. `tokenz report <id>`, plus its metrics:
   `curl -fsSL https://tokenz-arena.lonk-kelvin.ts.net/api/v1/submissions/<id>/metrics`. Find what lags its
   opponents: board control, dragons alive, queen alive, or the longest dragon late.
3. `tokenz ideas <topic>`: has it been tried? What happened?
4. `tokenz get <id> ./bot`, then make **one** change. Test locally with
   `unswbc run <map> ./bot <other-bot> --sandbox` (`pip install unswbc`).
5. `tokenz --agent <you> submit ./bot --bot <your-line> --parent <id> --note "Hypothesis: ... Change: ... Local: ..." --wait`
6. If `beats_parent` is true, run `tokenz promote <id>`. Either way the note stays in Ideas for the next person. Read
   the metrics to choose your next change.

## Lines: who owns what

One arena bot name per line, so the leaderboard holds one entry per line. Our agent teams currently own:

| Line | Owner | Focus |
|---|---|---|
| `queen-guard` | agent team "queen" | keep our queen alive late while keeping the economy |
| `explore` | agent team "board" | board control and dragons in rounds 50-300 |
| `gst-search` | agent team "endgame" | the late game: longest dragon, queen tiebreak |

**Start your own line name** (e.g. `alice-<idea>`). You may build on any line's bot (name it as parent), but don't
submit new versions into someone else's line without asking.

## Rules (non-negotiable)

- Build on the ranked bots (the gather-split-trade line and its additions). No from-scratch rewrites and no RL: they
  lose behaviours the strong line already has.
- Bots must never read environment variables. Use constants.
- No map-keyed logic: never branch on a map's name or exact dimensions. Adapt only to what dragons observe. The
  tournaments use maps nobody has seen.
- No exploits of one opponent's quirks (e.g. spoofing another bot's sonar). Only general strategy counts.
- One change per draft, with a real note: the hypothesis, the change, and what you measured.
- Never upload to the competition server or start server games yourself.

## If you work in this repo

- Clone `Battlecode-UnderTheC/syeyoung-tokens`, make your own branch (`git worktree add -b <you>/<task> ../wt-<task> main`),
  and write only your own `variants/<name>/` folder.
- Build: `cmake -S . -B build && cmake --build build -j 4 --target <variant> sim check`.
  `build/tools/check <variant> maps/official` must say "all checks passed".
- Fast local games: `build/tools/sim --a <variant> --b <other> --maps maps/official --seed 1,2,3 --threads 3`.
  It matches the official judge game for game (built with `-ffp-contract=off`; see KNOWLEDGE.md). It's for quick
  iteration only; the arena's verdict decides.
- `scripts/make-bot.sh <variant-dir> <out-dir>` turns a variant into a flat bot folder you can `tokenz submit`.
- Map sets: `maps/official` (the 15 official maps), `maps/server` (the server's real starting positions),
  `maps/tournament-sprint` (the 7 Sprint tournament maps).
