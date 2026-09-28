#!/usr/bin/env python3
"""The league: every tagged variant, built frozen, played against the others, rated.

    league serve [--once]          ingest new tags, gate them, play, rate, write reports; loop
    league status                  the leaderboard and what is queued
    league report <variant>        a variant's report (what agents read to improve)
    league quick <plugin> [--vs V ...] [--maps N]
                                   an untagged build (a path to its .so) against league
                                   variants on public + practice maps; touches nothing
    league trace <A> <B> <map>     one match, round by round (sim --trace)
    league rate                    recompute ratings and reports from the results alone
    league pending                 tags not yet ingested or gated, and matches still owed

Standard library only. Everything it owns lives in ../syeyoung-tokens.league/ (the data
dir, never in git): league.db (SQLite: variants, matches), plugins/, tools/, maps/,
reports/. Configuration is the constants below, in git; results are in the database and
the reports it writes, never in anyone's head.
"""
import argparse
import concurrent.futures as cf
import datetime as dt
import hashlib
import json
import math
import os
import random
import re
import shutil
import sqlite3
import subprocess
import sys
import tempfile
import time
from pathlib import Path

# the main checkout, also when run from a task worktree (which shares its .git)
REPO = Path(subprocess.run(['git', '-C', str(Path(__file__).resolve().parent), 'rev-parse', '--path-format=absolute',
                            '--git-common-dir'], check=True, capture_output=True, text=True).stdout.strip()).parent
DATA = REPO.parent / (REPO.name + '.league')

# ---- configuration (change it here, in a commit) ----
PARALLEL = 6                 # matches at once: this Mac has 8 performance cores
MATCH_TIMEOUT = 180          # seconds; a hung match is an error, not a result
BATCH = 120                  # matches between rating updates
ROTATING_MAPS = 20           # worldgen maps in each epoch's rotating suite
EPOCH_DAYS = 7               # the rotating suite changes this often
EPOCH_START = dt.date(2026, 9, 28)
HOLDOUT_POOL, HOLDOUT_WORLDGEN = 60, 40
HOLDOUT_TOP = 4              # the top this many by rating play each other on the holdout
HOLDOUT_SHARE = 0.25         # share of each batch kept for the holdout while rating games are owed
NEIGHBOURS = 8               # past this many variants, each plays its rating neighbours only
FULL_ROUND_ROBIN = 12        # ... up to this many, everyone plays everyone (low keeps a new tag rated in minutes)
GATE_VS_RANDOM = 0.75        # share of public matches a new variant must win against anchor-random
ANCHORS = ('anchor-random', 'anchor-greedy')
ANCHOR_ELO = {'anchor-random': 1000.0}   # fixes the rating scale
# the Agent ABI: plugins built from sources whose copies of these differ can't be played by the tools
ABI_FILES = ('shared/bot/agent.hpp', 'shared/bot/plugin.hpp', 'shared/protocol/observation.hpp',
             'shared/engine/dragons/core.hpp')


# ---- plumbing ----
def log(*a):
    print(time.strftime('%H:%M:%S'), *a, flush=True)


def git(*args, check=True):
    return subprocess.run(['git', '-C', str(REPO), *args], check=check, capture_output=True, text=True).stdout.strip()


def db():
    DATA.mkdir(exist_ok=True)
    con = sqlite3.connect(DATA / 'league.db', timeout=60)
    con.row_factory = sqlite3.Row
    con.executescript('''
      CREATE TABLE IF NOT EXISTS variants (
        name TEXT PRIMARY KEY, commit_sha TEXT, tagged TEXT, ingested TEXT, abi TEXT,
        status TEXT,            -- building, gating, active, rejected
        detail TEXT,            -- why rejected, build errors
        meta TEXT,              -- variant.toml as JSON
        elo REAL, elo_sd REAL, games INTEGER DEFAULT 0);
      CREATE TABLE IF NOT EXISTS matches (
        id INTEGER PRIMARY KEY, a TEXT, b TEXT,       -- a plays team A
        suite TEXT, map TEXT,                         -- public, rotating/e<N>, holdout, gate
        winner TEXT,                                  -- a, b, draw; NULL on error
        rounds INTEGER, result TEXT, error TEXT, tools TEXT, played TEXT);
      CREATE INDEX IF NOT EXISTS matches_pair ON matches(a, b, suite, map);
      CREATE TABLE IF NOT EXISTS kv (k TEXT PRIMARY KEY, v TEXT);''')
    return con


def kv(con, k, default=None):
    r = con.execute('SELECT v FROM kv WHERE k=?', (k,)).fetchone()
    return r[0] if r else default


def set_kv(con, k, v):
    con.execute('INSERT OR REPLACE INTO kv VALUES (?, ?)', (k, v))
    con.commit()


def abi_of(ref):
    h = hashlib.sha256()
    for f in ABI_FILES:
        h.update(git('show', f'{ref}:{f}', check=False).encode())
    return h.hexdigest()[:12]


def export(ref, dest):
    """The tree at `ref` into `dest`, fresh."""
    if dest.exists():
        shutil.rmtree(dest)
    dest.mkdir(parents=True)
    archive = subprocess.run(['git', '-C', str(REPO), 'archive', ref], check=True, capture_output=True).stdout
    subprocess.run(['tar', '-x', '-C', str(dest)], input=archive, check=True)


def cmake_build(src, build, targets):
    r = subprocess.run(['cmake', '-S', str(src), '-B', str(build), '-DCMAKE_BUILD_TYPE=Release'],
                       capture_output=True, text=True)
    if r.returncode == 0:
        r = subprocess.run(['cmake', '--build', str(build), '-j', str(PARALLEL), '--target', *targets],
                           capture_output=True, text=True)
    return r.returncode == 0, (r.stdout + r.stderr)[-4000:]


# ---- the tools: match, check, sim, built from main ----
def tools(con):
    """Builds the tools from main when main moved; returns their commit."""
    head = git('rev-parse', '--short', 'main')
    if kv(con, 'tools_commit') == head and (DATA / 'tools' / 'match').exists():
        return head
    log(f'building tools at main {head}')
    src, build = DATA / 'src' / '_tools', DATA / 'build' / '_tools'
    export('main', src)
    ok, out = cmake_build(src, build, ['match', 'check', 'sim'])
    if not ok:
        raise SystemExit(f'tools do not build at {head}:\n{out}')
    (DATA / 'tools').mkdir(exist_ok=True)
    for t in ('match', 'check', 'sim'):
        shutil.copy2(build / 'tools' / t, DATA / 'tools' / t)
    set_kv(con, 'tools_commit', head)
    set_kv(con, 'tools_abi', abi_of('main'))
    return head


def plugin(name):
    return DATA / 'plugins' / f'lib{name}.so'


# ---- maps ----
def epoch(today=None):
    return ((today or dt.date.today()) - EPOCH_START).days // EPOCH_DAYS


def worldgen(seed, out):
    if not out.exists():
        subprocess.run([sys.executable, str(REPO / 'maps' / 'worldgen' / 'worldgen.py'), '--seed', str(seed),
                        '-o', str(out)], check=True, capture_output=True)


def pool_records(path):
    data, i = path.read_bytes(), 0
    while i < len(data):
        j = data.index(b'\n', i)
        n = int(data[i:j])
        yield data[j + 1:j + 1 + n].decode()
        i = j + 1 + n
        while i < len(data) and data[i:i + 1] in b'\n ':
            i += 1


def suites(con):
    """{suite: [map paths]}: public (the official maps), this epoch's rotating worldgen maps,
    and the holdout (hidden from agents: pool maps plus worldgen maps from a secret seed range)."""
    maps = DATA / 'maps'
    public = maps / 'public'
    if not public.exists():
        shutil.copytree(REPO / 'maps' / 'official', public)
    e = epoch()
    rot = maps / 'rotating' / f'e{e}'
    rot.mkdir(parents=True, exist_ok=True)
    with cf.ThreadPoolExecutor(PARALLEL) as ex:
        list(ex.map(lambda i: worldgen(e * 1000 + i, rot / f'wg{e * 1000 + i}.map'), range(ROTATING_MAPS)))
    hold = maps / 'holdout'
    if not hold.exists():
        log('making the holdout suite')
        tmp = maps / 'holdout.tmp'
        tmp.mkdir(exist_ok=True)
        rng = random.SystemRandom()
        pool = [t for t in pool_records(maps / 'pool_maps.txt') if re.search(r'MAP_NAME pool-', t)]
        for k, t in enumerate(rng.sample(pool, HOLDOUT_POOL)):
            (tmp / f'h{k:03d}.map').write_text(t)
        salt = rng.randrange(10**9, 2 * 10**9)   # far from any seed anyone would try
        with cf.ThreadPoolExecutor(PARALLEL) as ex:
            list(ex.map(lambda k: worldgen(salt + k, tmp / f'h{HOLDOUT_POOL + k:03d}.map'), range(HOLDOUT_WORLDGEN)))
        tmp.rename(hold)
    return {'public': sorted(public.glob('*.map')), f'rotating/e{e}': sorted(rot.glob('*.map')),
            'holdout': sorted(hold.glob('*.map'))}


# ---- matches ----
def play(a, b, suite, map_path):
    """One match in its own process: (a, b, suite, map, winner, rounds, result json, error)."""
    cmd = [str(DATA / 'tools' / 'match'), str(plugin(a)), str(plugin(b)), str(map_path)]
    try:
        r = subprocess.run(cmd, capture_output=True, text=True, timeout=MATCH_TIMEOUT)
        out = json.loads(r.stdout.strip().splitlines()[-1]) if r.stdout.strip() else {}
        if r.returncode != 0 or 'error' in out or 'winner' not in out:
            err = out.get('error') or f'exit {r.returncode}: {r.stderr[-500:]}'
            return (a, b, suite, map_path.stem, None, None, None, err)
        w = {'A': 'a', 'B': 'b', 'draw': 'draw'}[out['winner']]
        return (a, b, suite, map_path.stem, w, out['rounds'], json.dumps(out), None)
    except subprocess.TimeoutExpired:
        return (a, b, suite, map_path.stem, None, None, None, f'timeout after {MATCH_TIMEOUT}s')


def run(con, jobs, tools_commit):
    """Plays (a, b, suite, map path) jobs, PARALLEL at once, storing each result as it lands."""
    if not jobs:
        return
    with cf.ThreadPoolExecutor(PARALLEL) as ex:
        for res in ex.map(lambda j: play(*j), jobs):
            con.execute('INSERT INTO matches (a, b, suite, map, winner, rounds, result, error, tools, played) '
                        'VALUES (?,?,?,?,?,?,?,?,?,?)', (*res, tools_commit, dt.datetime.now().isoformat(' ', 'seconds')))
            con.commit()


def played(con, a, b, suite):
    """Maps already played (or failed) between a and b in `suite`, by who was team A."""
    rows = con.execute('SELECT a, map FROM matches WHERE suite=? AND ((a=? AND b=?) OR (a=? AND b=?))',
                       (suite, a, b, b, a)).fetchall()
    return {(r['a'], r['map']) for r in rows}


def pair_jobs(con, a, b, suite, maps):
    done = played(con, a, b, suite)
    return [(x, y, suite, m) for m in maps for x, y in ((a, b), (b, a)) if (x, m.stem) not in done]


# ---- ingest and gates ----
def ingest(con, tools_commit, public):
    tags = [t for t in git('tag', '--list', 'variant/*').split('\n') if t]
    known = {r['name'] for r in con.execute('SELECT name FROM variants')}
    for tag in tags:
        name = tag[len('variant/'):]
        if name in known:
            continue
        commit = git('rev-parse', '--short', f'{tag}^{{commit}}')
        tagged = git('log', '-1', '--format=%ci', f'{tag}^{{commit}}')
        meta = parse_toml(git('show', f'{tag}:variants/{name}/variant.toml', check=False))
        abi = abi_of(tag)
        con.execute('INSERT INTO variants (name, commit_sha, tagged, ingested, abi, status, meta) VALUES (?,?,?,?,?,?,?)',
                    (name, commit, tagged, dt.datetime.now().isoformat(' ', 'seconds'), abi, 'building', json.dumps(meta)))
        con.commit()
        log(f'ingesting {name} at {commit}')
        if abi != kv(con, 'tools_abi'):
            reject(con, name, f'Agent ABI {abi} differs from the tools\' {kv(con, "tools_abi")}: '
                              f'its {", ".join(ABI_FILES)} are not main\'s')
            continue
        src, build = DATA / 'src' / name, DATA / 'build' / name
        export(tag, src)
        ok, out = cmake_build(src, build, [name])
        if not ok:
            reject(con, name, 'does not build:\n' + out)
            continue
        (DATA / 'plugins').mkdir(exist_ok=True)
        shutil.copy2(build / 'variants' / f'lib{name}.so', plugin(name))
        shutil.rmtree(build, ignore_errors=True)    # the frozen plugin is what matters
        con.execute("UPDATE variants SET status='gating' WHERE name=?", (name,))
        con.commit()
    for (name,) in con.execute("SELECT name FROM variants WHERE status='gating'").fetchall():
        gate(con, name, tools_commit, public)


def gate(con, name, tools_commit, public):
    """check (no crashes, no avoidable wall or self deaths, board memory right), then it must
    beat anchor-random. Anchors pass by definition."""
    if name in ANCHORS:
        return activate(con, name)
    r = subprocess.run([str(DATA / 'tools' / 'check'), str(plugin(name)), str(DATA / 'maps' / 'public')],
                       capture_output=True, text=True, timeout=MATCH_TIMEOUT * 4)
    if r.returncode != 0 or 'all checks passed' not in r.stdout:
        return reject(con, name, 'check failed:\n' + (r.stdout + r.stderr)[-3000:])
    if con.execute("SELECT status FROM variants WHERE name='anchor-random'").fetchone() is None:
        return   # the anchor isn't ingested yet; try again next pass
    run(con, pair_jobs(con, name, 'anchor-random', 'gate', public), tools_commit)
    rows = con.execute("SELECT a, winner FROM matches WHERE suite='gate' AND "
                       "((a=? AND b='anchor-random') OR (b=? AND a='anchor-random'))", (name, name)).fetchall()
    wins = sum(1 for r in rows if (r['winner'] == 'a') == (r['a'] == name) and r['winner'] in ('a', 'b'))
    errors = sum(1 for r in rows if r['winner'] is None)
    if errors:
        return reject(con, name, f'{errors} matches against anchor-random crashed or timed out (see league report)')
    if wins < GATE_VS_RANDOM * len(rows):
        return reject(con, name, f'won {wins} of {len(rows)} public matches against anchor-random; '
                                 f'the gate is {GATE_VS_RANDOM:.0%}')
    activate(con, name)


def activate(con, name):
    con.execute("UPDATE variants SET status='active', detail=NULL WHERE name=?", (name,))
    con.commit()
    log(f'{name} is active')


def reject(con, name, why):
    con.execute("UPDATE variants SET status='rejected', detail=? WHERE name=?", (why, name))
    con.commit()
    log(f'{name} rejected: {why.splitlines()[0]}')
    write_report(con, name)


def parse_toml(text):
    """The flat `key = value` subset variant.toml uses."""
    out = {}
    for line in text.splitlines():
        m = re.match(r'\s*(\w+)\s*=\s*(.+?)\s*$', line)
        if m:
            k, v = m.groups()
            try:
                out[k] = json.loads(v)
            except json.JSONDecodeError:
                out[k] = v
    return out


# ---- scheduling ----
def schedule(con, suite_maps, limit=BATCH):
    """The next batch: pairs short of a full set of games on the rating suites first, fewest
    games first; then the holdout among the top."""
    active = [r['name'] for r in con.execute(
        "SELECT name FROM variants WHERE status='active' ORDER BY elo DESC NULLS LAST, name")]
    pairs = set()
    if len(active) <= FULL_ROUND_ROBIN:
        pairs = {(a, b) for i, a in enumerate(active) for b in active[i + 1:]}
    else:
        for i, a in enumerate(active):
            for b in active[max(0, i - NEIGHBOURS // 2): i + NEIGHBOURS // 2 + 1] + list(ANCHORS) + active[:1]:
                if a != b and b in active:
                    pairs.add(tuple(sorted((a, b))))
    todo = []
    for a, b in sorted(pairs):
        # the rating suites interleaved within a pair, so a partly played pair has games on
        # both the public and the rotating maps (otherwise the longer suite goes first and a
        # new variant's early rating comes from one kind of map only)
        per_suite = [pair_jobs(con, a, b, s, maps) for s, maps in suite_maps.items() if s != 'holdout']
        mixed = [js[k] for k in range(max(map(len, per_suite), default=0)) for js in per_suite if k < len(js)]
        if mixed:
            todo.append(mixed)
    todo.sort(key=len, reverse=True)   # the pairs missing the most games first
    jobs = [j for js in todo for j in js]
    # the holdout among the top: each other, and anchor-greedy (a fixed yardstick: holdout vs
    # public rate against it shows overfitting). It gets a share of every batch, so it keeps
    # up while new tags keep arriving; pairs are interleaved so partial coverage stays even.
    top = [a for a in active if a not in ANCHORS][:HOLDOUT_TOP]
    held = []
    for i, a in enumerate(top):
        for b in top[i + 1:] + (['anchor-greedy'] if 'anchor-greedy' in active else []):
            held.append(pair_jobs(con, a, b, 'holdout', suite_maps['holdout']))
    held = [js[k] for k in range(max(map(len, held), default=0)) for js in held if k < len(js)]
    if limit is None:
        return held + jobs
    share = limit if not jobs else int(limit * HOLDOUT_SHARE)
    return held[:share] + jobs[:limit - min(share, len(held))]


# ---- ratings: Bradley-Terry, draws half a win each way ----
def rate(con):
    active = [r['name'] for r in con.execute("SELECT name FROM variants WHERE status='active'")]
    if not active:
        return
    idx = {n: i for i, n in enumerate(active)}
    wins = [[0.0] * len(active) for _ in active]
    for r in con.execute("SELECT a, b, winner FROM matches WHERE winner IS NOT NULL AND suite NOT IN ('holdout', 'gate')"):
        if r['a'] not in idx or r['b'] not in idx:
            continue
        i, j = idx[r['a']], idx[r['b']]
        if r['winner'] == 'a':
            wins[i][j] += 1
        elif r['winner'] == 'b':
            wins[j][i] += 1
        else:
            wins[i][j] += .5
            wins[j][i] += .5
    n = len(active)
    for i in range(n):              # a prior: one virtual draw with every other variant keeps unbeaten ratings finite
        for j in range(n):
            if i != j:
                wins[i][j] += .5 / max(1, n - 1)
    p = [1.0] * n
    for _ in range(2000):           # the MM iteration (Hunter 2004)
        new = []
        for i in range(n):
            w = sum(wins[i])
            d = sum((wins[i][j] + wins[j][i]) / (p[i] + p[j]) for j in range(n) if j != i)
            new.append(w / d if d else p[i])
        g = math.exp(sum(math.log(x) for x in new) / n)
        new = [x / g for x in new]
        done = max(abs(math.log(x / y)) for x, y in zip(new, p)) < 1e-9
        p = new
        if done:
            break
    elo = [400 * math.log10(x) for x in p]
    shift = 0.0
    for name, fixed in ANCHOR_ELO.items():
        if name in idx:
            shift = fixed - elo[idx[name]]
    for name, i in idx.items():
        games = sum(wins[i][j] + wins[j][i] for j in range(n) if j != i) - 1.0   # minus the prior
        # the Fisher information of log p_i, other ratings held fixed, in Elo
        info = sum((wins[i][j] + wins[j][i]) * p[i] * p[j] / (p[i] + p[j]) ** 2 for j in range(n) if j != i)
        sd = 400 / math.log(10) / math.sqrt(info) if info > 0 else None
        con.execute('UPDATE variants SET elo=?, elo_sd=?, games=? WHERE name=?',
                    (elo[i] + shift, sd, round(games), name))
    con.commit()


# ---- reports ----
def rows_of(con, name, suites_like=None):
    q = 'SELECT * FROM matches WHERE (a=? OR b=?)'
    return [r for r in con.execute(q, (name, name)) if suites_like is None or suites_like(r['suite'])]


def side_of(r, name):
    return 0 if r['a'] == name else 1


def outcome(r, name):
    if r['winner'] is None:
        return 'error'
    if r['winner'] == 'draw':
        return 'draw'
    return 'win' if (r['winner'] == 'a') == (r['a'] == name) else 'loss'


def wdl(rs, name):
    c = {'win': 0, 'draw': 0, 'loss': 0, 'error': 0}
    for r in rs:
        c[outcome(r, name)] += 1
    s = f"{c['win']}-{c['draw']}-{c['loss']}"
    return s + (f" ({c['error']} errors)" if c['error'] else '')


def write_report(con, name):
    v = con.execute('SELECT * FROM variants WHERE name=?', (name,)).fetchone()
    meta = json.loads(v['meta'] or '{}')
    L = [f'# {name}', '',
         f"commit {v['commit_sha']} (tag variant/{name}), status **{v['status']}**"
         + (f", Elo {v['elo']:.0f} ± {v['elo_sd'] or 0:.0f} over {v['games']} games" if v['elo'] is not None else ''),
         f"archetype: {meta.get('archetype') or '-'}; operator: {meta.get('operator') or '-'}; "
         f"parents: {', '.join(meta.get('parents') or []) or 'none'}", '']
    if v['detail']:
        L += ['## Why it is not active', '', '```', v['detail'].strip(), '```', '']
    visible = rows_of(con, name, lambda s: s != 'holdout')
    hold = rows_of(con, name, lambda s: s == 'holdout')
    if visible:
        L += ['## Against each opponent (W-D-L, public + rotating)', '', '| opponent | W-D-L | as A | as B | mean length margin at the end |',
              '|---|---|---|---|---|']
        opps = {}
        for r in visible:
            if r['suite'] == 'gate':
                continue
            opps.setdefault(r['b'] if r['a'] == name else r['a'], []).append(r)
        for o, rs in sorted(opps.items(), key=lambda kv_: kv_[0]):
            margins = []
            for r in rs:
                if r['result']:
                    res, s = json.loads(r['result']), side_of(r, name)
                    margins.append(res['longest'][s] - res['longest'][1 - s])
            L.append(f"| {o} | {wdl(rs, name)} | {wdl([r for r in rs if r['a'] == name], name)} | "
                     f"{wdl([r for r in rs if r['b'] == name], name)} | "
                     f"{sum(margins) / len(margins) if margins else 0:+.1f} longest |")
        L.append('')
        L += ['## By map (public and rotating; W-D-L over every opponent)', '']
        by_map = {}
        for r in visible:
            if r['suite'] != 'gate':
                by_map.setdefault(f"{r['suite']}/{r['map']}", []).append(r)
        worst = sorted(by_map.items(), key=lambda kv_: sum(outcome(r, name) == 'win' for r in kv_[1]) / len(kv_[1]))
        L += [f"- {m}: {wdl(rs, name)}" for m, rs in worst]
        L.append('')
        deaths, reasons, fatal, turns, n = {}, {}, {}, [], 0
        for r in visible:
            if not r['result']:
                continue
            res, s = json.loads(r['result']), side_of(r, name)
            n += 1
            for k, x in res['deaths'][s].items():
                deaths[k] = deaths.get(k, 0) + x
            for k, x in res['reasons'][s].items():
                reasons[k] = reasons.get(k, 0) + x
            for k, x in res['fatal'][s].items():
                fatal[k] = fatal.get(k, 0) + x
            turns.append(res['turn_us'][s])
        if n:
            L += ['## How its dragons die and why they act (per match)', '',
                  'deaths: ' + ', '.join(f'{k} {x / n:.1f}' for k, x in sorted(deaths.items(), key=lambda t: -t[1])), '']
            if reasons:
                L += ['| reason | actions | fatal turns |', '|---|---|---|']
                L += [f'| {k} | {reasons[k] / n:.0f} | {fatal.get(k, 0) / n:.1f} |'
                      for k in sorted(reasons, key=lambda k: -reasons[k])]
                L.append('')
            p99 = max(t['p99'] for t in turns)
            mx = max(t['max'] for t in turns)
            L += [f'turn time: worst p99 {p99:.0f} µs, slowest turn {mx:.0f} µs (native; the judge allows '
                  '100M wasm points a turn, measure with the judge before trusting big numbers)', '']
    errors = [r for r in visible if r['error']]
    if errors:
        L += ['## Errors', ''] + [f"- vs {r['b'] if r['a'] == name else r['a']} on {r['map']}: {r['error']}"
                                   for r in errors[:20]] + ['']
    if hold:
        L += ['## Holdout', '', f'{wdl(hold, name)} over {len(hold)} hidden-map matches against the top variants. '
              'The maps stay hidden: tune to the game, not to them.', '']
    (DATA / 'reports').mkdir(exist_ok=True)
    (DATA / 'reports' / f'{name}.md').write_text('\n'.join(L))


def write_leaderboard(con):
    vs = con.execute('SELECT * FROM variants ORDER BY status, elo DESC').fetchall()
    L = ['# Leaderboard', '', f"{dt.datetime.now():%Y-%m-%d %H:%M}, tools at main {kv(con, 'tools_commit')}, "
         f'rotating suite e{epoch()}', '',
         '| # | variant | Elo | ± | games | holdout W-D-L | archetype | parents |', '|---|---|---|---|---|---|---|---|']
    k = 0
    for v in vs:
        if v['status'] != 'active':
            continue
        k += 1
        meta = json.loads(v['meta'] or '{}')
        hold = rows_of(con, v['name'], lambda s: s == 'holdout')
        L.append(f"| {k} | {v['name']} | {v['elo'] or 0:.0f} | {v['elo_sd'] or 0:.0f} | {v['games']} | "
                 f"{wdl(hold, v['name']) if hold else '-'} | {meta.get('archetype', '')} | "
                 f"{', '.join(meta.get('parents') or [])} |")
    others = [v for v in vs if v['status'] != 'active']
    if others:
        L += ['', '## Not active', ''] + [f"- {v['name']}: {v['status']}"
                                           + (f" ({v['detail'].splitlines()[0]})" if v['detail'] else '') for v in others]
    (DATA / 'reports').mkdir(exist_ok=True)
    (DATA / 'reports' / 'LEADERBOARD.md').write_text('\n'.join(L) + '\n')


def refresh(con):
    rate(con)
    for (name,) in con.execute('SELECT name FROM variants').fetchall():
        write_report(con, name)
    write_leaderboard(con)


# ---- commands ----
def serve(args):
    con = db()
    while True:
        tools_commit = tools(con)
        suite_maps = suites(con)
        ingest(con, tools_commit, suite_maps['public'])
        jobs = schedule(con, suite_maps)
        if jobs:
            log(f'playing {len(jobs)} matches')
            t0 = time.time()
            run(con, jobs, tools_commit)
            log(f'  {time.time() - t0:.0f}s')
        refresh(con)
        if args.once:
            break
        if not jobs:
            time.sleep(60)          # nothing to play: wait for new tags


def status(args):
    con = db()
    write_leaderboard(con)
    print((DATA / 'reports' / 'LEADERBOARD.md').read_text())
    n = con.execute('SELECT count(*), sum(error IS NOT NULL) FROM matches').fetchone()
    last = con.execute('SELECT max(played) FROM matches').fetchone()[0]
    print(f'{n[0]} matches ({n[1] or 0} errors), last at {last}; data in {DATA}')


def report(args):
    p = DATA / 'reports' / f'{args.variant}.md'
    if not p.exists():
        raise SystemExit(f'no report for {args.variant} (is it tagged and ingested? see league status)')
    print(p.read_text())


def quick(args):
    """An untagged build against league variants, on public maps plus practice worldgen maps."""
    so = Path(args.plugin).resolve()
    if not so.exists():
        raise SystemExit(f'{so}: no such plugin (build it: cmake --build build, then build/variants/lib<name>.so)')
    con = db()
    opponents = args.vs or [r['name'] for r in con.execute(
        "SELECT name FROM variants WHERE status='active' ORDER BY elo DESC LIMIT 3")] or list(ANCHORS)
    practice = DATA / 'maps' / 'practice'
    practice.mkdir(parents=True, exist_ok=True)
    with cf.ThreadPoolExecutor(PARALLEL) as ex:   # seeds 900000+: never a rotating or holdout seed
        list(ex.map(lambda i: worldgen(900000 + i, practice / f'wg{900000 + i}.map'), range(args.maps)))
    maps = sorted((DATA / 'maps' / 'public').glob('*.map')) + sorted(practice.glob('*.map'))[:args.maps]
    with tempfile.TemporaryDirectory() as tmp:
        me = Path(tmp) / 'libcandidate.so'        # a fixed name, so the match tool can't confuse it with a league one
        shutil.copy2(so, me)
        for o in opponents:
            jobs = [(m, x, y) for m in maps for x, y in ((me, plugin(o)), (plugin(o), me))]

            def one(j):
                m, x, y = j
                r = subprocess.run([str(DATA / 'tools' / 'match'), str(x), str(y), str(m)], capture_output=True,
                                   text=True, timeout=MATCH_TIMEOUT)
                out = json.loads(r.stdout.strip().splitlines()[-1]) if r.stdout.strip() else {'error': r.stderr[-300:]}
                return m, x == me, out
            c = {'win': 0, 'draw': 0, 'loss': 0, 'error': 0}
            lost = []
            with cf.ThreadPoolExecutor(PARALLEL) as ex:
                for m, mine_a, out in ex.map(one, jobs):
                    if 'winner' not in out:
                        c['error'] += 1
                        lost.append(f"{m.stem} error: {out.get('error')}")
                    elif out['winner'] == 'draw':
                        c['draw'] += 1
                    elif (out['winner'] == 'A') == mine_a:
                        c['win'] += 1
                    else:
                        c['loss'] += 1
                        lost.append(f"{m.stem} as {'A' if mine_a else 'B'}")
            print(f"vs {o}: {c['win']}-{c['draw']}-{c['loss']}" + (f" ({c['error']} errors)" if c['error'] else '')
                  + (f"; lost: {', '.join(lost[:12])}" if lost else ''))


def trace(args):
    def arg(v):
        return str(plugin(v)) if plugin(v).exists() else str(Path(v).resolve())
    m = Path(args.map)
    subprocess.run([str(DATA / 'tools' / 'sim'), '--a', arg(args.a), '--b', arg(args.b),
                    '--maps', str(m.parent), '--trace', m.stem])


def pending(args):
    """What the league still owes: 0 tags and 0 matches means every variant is fully rated."""
    con = db()
    known = {r['name']: r['status'] for r in con.execute('SELECT name, status FROM variants')}
    tags = [t[len('variant/'):] for t in git('tag', '--list', 'variant/*').split('\n') if t]
    waiting = [t for t in tags if known.get(t) in (None, 'building', 'gating')]
    jobs = schedule(con, suites(con), limit=None)
    print(f"tags waiting: {len(waiting)}{' (' + ', '.join(waiting) + ')' if waiting else ''}")
    print(f'matches owed: {len(jobs)}')


def rerate(args):
    refresh(db())
    print((DATA / 'reports' / 'LEADERBOARD.md').read_text())


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n\n')[0])
    sub = ap.add_subparsers(dest='cmd', required=True)
    s = sub.add_parser('serve')
    s.add_argument('--once', action='store_true', help='one pass, then exit')
    s.set_defaults(fn=serve)
    sub.add_parser('status').set_defaults(fn=status)
    s = sub.add_parser('report')
    s.add_argument('variant')
    s.set_defaults(fn=report)
    s = sub.add_parser('quick')
    s.add_argument('plugin', help='path to a built lib<name>.so')
    s.add_argument('--vs', action='append', help='a league variant (default: the top 3)')
    s.add_argument('--maps', type=int, default=10, help='practice worldgen maps besides the public ones')
    s.set_defaults(fn=quick)
    s = sub.add_parser('trace')
    s.add_argument('a')
    s.add_argument('b')
    s.add_argument('map', help='a .map file')
    s.set_defaults(fn=trace)
    sub.add_parser('rate').set_defaults(fn=rerate)
    sub.add_parser('pending').set_defaults(fn=pending)
    args = ap.parse_args()
    args.fn(args)


if __name__ == '__main__':
    main()
