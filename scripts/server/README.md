# bcweb: client for the competition website

`bcweb.py` reads https://game.battlecode.au as our team (UnderTheC, team id 328) and can
request battles. It uses only the Python 3.11 standard library.

```
python3 scripts/server/bcweb.py <command> [options]      # -h on any command for options
```

## Auth and secrets

- The login is the web email and password in `~/.unswbc-web/credentials` (line 1 email,
  line 2 password, mode 0600). It is read at runtime and never printed.
- `login` signs in (`POST /api/auth/sign-in/email`, better-auth) and caches the session
  cookie in `~/.unswbc-web/session.json` (0600). Every other command signs in again by
  itself if the cookie has lapsed.
- The tool does not use the toolkit's API key (`~/.unswbc/keys.json`) or the `/api/v1`
  JSON API that needs it. Don't create a new API key on the Team page either: making one
  replaces the key that `unswbc submit` uses.
- The tool never uploads, activates or deletes submissions, and never changes any settings.

## Commands

| command | what it does |
|---|---|
| `login` | sign in, cache the session, print the team |
| `whoami` | our user, team, rating and battle-request settings |
| `submissions [--limit N] [--maps]` | our versions (id, name, upload time UTC, status, game W-D-L, language). `*` marks the live one (status `active`). `--maps` adds each version's per-map record |
| `leaderboard [--pages N]` | ranks, 10 per page (default 3 pages). Our row is always shown and marked `*` |
| `battles [--limit N] [--maps]` | our battles, newest first: id, time, kind, opponent and rating, our side, W-L, result, rating change, per-game results, queue position if still queued. `--maps` makes one more request per battle and lists each game's map and result |
| `fetch <battle>` | download every finished game of a battle to `syeyoung-tokens.league/replays/M<battle>/M<game>.replay`, write `battle.json` next to them, and print one line per game |
| `wait <battle> [--interval S] [--timeout MIN]` | poll every 60 s or more until every game is done, then `fetch` |
| `challenge <opponent> [--maps A,B,..] [--ranked] [--dry-run]` | request a battle. Unranked by default |
| `quota [opponent]` | games left in the site's hourly allowance, plus our local log |

Battle ids can be written `727546` or `M727546`. An opponent can be given as a team name
(case-insensitive; a unique prefix works), as `#RANK` or `rank:N` (leaderboard rank), or as
`id:TEAMID`.

### Kinds of battle

- **ranked**: 5 games on random maps, and the result changes ratings. `challenge --ranked`
  is the only way this tool requests one.
- **unranked**: the requester picks 1 to 10 maps, one game per map, and ratings don't move.
  This is the default for `challenge`. `--maps` takes names or prefixes
  (`--maps Default,Trophy,Devil`) or `all`. Without `--maps` it plays every map in play
  (currently 10, so it costs 10 games).
- In `battles`, the kind column shows `ranked` or `unranked`, then `/scrim` when a team
  requested the battle or `/auto` for the site's automatic scrims (autoscrim).

### Hourly allowance (the site enforces it)

The site allows **60 games an hour** (a ranked battle counts as 5 games, an unranked one as
1 per map). The allowance is shared across all non-dev opponents; dev (organiser) teams have
a separate pool of 60. It also counts battles that teammates start on the website. `quota`
asks the site (`GET /teams/<id>/availability`, which returns `{dev, remaining, freeIn}`).
`challenge` checks it before sending. If a request would go over, the tool refuses with the
number of minutes until more games free up. If the site refuses a request, the tool prints
the site's message and does not retry.

Every battle the tool starts is also appended to `~/.unswbc-web/challenges.jsonl`
(time, battle, opponent, ranked or not, maps, cost). This log is only a record: the site's
count is the one that applies.

### Which submission plays

The site always plays our **active** submission. It has no way to pick another version for
a single battle, so `challenge --submission` is refused. To test another version you would
have to activate it, which counts as a submission change and is out of scope here.

## Example output

```
$ bcweb.py submissions
id      name                   uploaded(UTC)  status  W-D-L     lang
*12534  v9-arena-search-7      09-30 04:34    active  112-0-86  cpp
 11820  v8                     09-29 15:00    idle    77-0-79   cpp
...
$ bcweb.py battles --limit 3
battle   at(UTC)      kind            opponent (elo)           side  W-L  result  elo  games
M732336  09-30 17:07  unranked/scrim  Thermal Throttle (1481)  A     2-0  live    +0   WW.    queued: 0 ahead, ~3s
M730683  09-30 17:07  ranked/auto     python.io (1782)         B     3-1  live         WL.WW  queued: 68 ahead, ~16s
M727546  09-30 16:18  ranked/scrim    pizza (1780)             A     2-3  loss    -2   LLWLW
$ bcweb.py challenge 'Thermal Throttle' --maps Default,Trophy,Devil
allowance: 55 game(s) left this hour; more free in 11 min; this request costs 3
queued unranked vs Thermal Throttle (team 234) on Default, Trophy, Devil
battle id: M732336
$ bcweb.py wait M732336
[2026-09-30T17:07:58Z] M732336: completed, 3/3 games finished
M732336: UnderTheC vs Thermal Throttle  unranked  status completed  -> .../syeyoung-tokens.league/replays/M732336
  M732336 Default: us=A (12534 v9-arena-search-7) vs Thermal Throttle  winner=A WIN  end=teamEliminated  A total 60 / B total 0
  M732337 Trophy: us=A (12534 v9-arena-search-7) vs Thermal Throttle  winner=A WIN  end=teamEliminated  A total 86 / B total 0
  M732338 Devil: us=A (12534 v9-arena-search-7) vs Thermal Throttle  winner=A WIN  end=roundLimit  A total 90 / B total 19
```

The per-game line comes from `replays/league-2/analysis/decode.py` in the league checkout.
Our side is whichever of botA and botB holds one of our submission ids. If `decode.py`
can't be found, the line falls back to the site's winner.

## Endpoints used (all with the session cookie)

| purpose | request |
|---|---|
| sign in | `POST /api/auth/sign-in/email` `{email, password}` with `Origin` set → `__Secure-better-auth.session_token` cookie |
| page data | `GET <page>/__data.json` (SvelteKit, devalue-encoded) for `/team`, `/submissions`, `/leaderboard?page=N`, `/battles?mine=1&page=N`, `/teams/<id>`, `/visualiser?match=<battle>` (the battle's games and maps) |
| replay | `GET /api/matches/<game>/replay`, gzip-compressed. It is saved decompressed, matching the existing files |
| allowance | `GET /teams/<id>/availability` → `{dev, remaining, freeIn}` |
| request battle | `POST /teams/<id>?/challenge`, form fields `kind` (ranked or unranked), `ranked` (true or false) and `mapId` (repeated, unranked only), with header `x-sveltekit-action: true` → `{type: "success", data: {battleId}}` |

Pages come 10 rows at a time. The tool leaves at least 1 s between requests and backs off
on 429 and 5xx responses. `wait` polls no faster than once a minute.
The documented key-based API (`/docs/api`) limits a key to 120 requests a minute; this tool
does not use that API.
