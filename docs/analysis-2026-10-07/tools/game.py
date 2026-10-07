"""arena-r16: one pass over a replay -> per-game feature dict (JSON line).
python3 game.py <ids...>   (reads ../data/rep/<id>.replay.gz, rows from ../data/rows.json)
"""
import json, sys, os, collections
sys.path.insert(0, os.path.dirname(__file__))
import decode
decode.FAST = True

PH = [0, 30, 60, 100, 200, 300, 400, 430, 500]


def ph(r):
    for i in range(len(PH) - 1):
        if r < PH[i + 1]:
            return f"{PH[i]}"
    return "430"


def tdist(g, a, b):
    dx = abs(a[0] - b[0]); dy = abs(a[1] - b[1]); dx = min(dx, g.W - dx); dy = min(dy, g.H - dy)
    return dx + dy, max(dx, dy)


def reach(L):
    # max steps a dragon of length L can take in one move without dying: ceil(L/4) free, then paid while len>2
    free = (L + 3) // 4
    return free + max(0, L - 2)


def analyse(path, us):
    d = decode.load(path); g = decode.GameMap(d["map"])
    th = 'B' if us == 'A' else 'A'
    side = lambda t: 'us' if t == us else 'th'
    queens = {t: k for k, (t, _) in enumerate(g.dragons[:2])}
    starts = {s: [b[0] for t, b in g.dragons if side(t) == s] for s in ('us', 'th')}
    # bed zone by multi-source BFS distance from each side's starting heads
    du = {}; dt = {}
    for s, D in (('us', du), ('th', dt)):
        for h in starts[s]:
            for t, v in g.bfs(h).items():
                if v < D.get(t, 1 << 30): D[t] = v
    def zone(t):
        a, b = du.get(t, 999), dt.get(t, 999)
        return 'own' if a < b - 2 else 'far' if b < a - 2 else 'mid'
    F = {s: collections.defaultdict(lambda: collections.Counter()) for s in ('us', 'th')}
    snaps = {}
    qinfo = {'us': dict(turns=0, multi=0, ate=collections.Counter(), tiles=collections.deque(maxlen=30)),
             'th': dict(turns=0, multi=0, ate=collections.Counter(), tiles=collections.deque(maxlen=30))}
    qlast = {}
    qdeath = {}
    bigkills = collections.Counter()   # our 8+ dragons killed head-on by shorter enemy movers, by phase
    first_contact = None
    pre_q = {}
    qexp = {'us': collections.defaultdict(collections.Counter), 'th': collections.defaultdict(collections.Counter)}
    for kind, st, info in decode.walk(d):
        if kind == 'pre':
            i = info
            for t, q in queens.items():
                if i == q and q in st.alive:
                    s = side(t); qh = st.head(q)
                    en = [(j, st.head(j), st.length(j)) for j in st.alive if st.team[j] != t]
                    near = sorted((tdist(g, h, qh)[0], tdist(g, h, qh)[1], L, j) for j, h, L in en)
                    own_segs = sum(1 for j in st.alive if st.team[j] == t and j != q for x in st.body[j] if g.in_window(qh, x))
                    occ = st.occupied()
                    room = len(g.bfs(qh, blocked=set(occ) - {qh}, limit=8)) - 1
                    pre_q[s] = dict(round=st.round, head=qh, len=st.length(q), near=near[:6], own_segs=own_segs, room=room,
                                    allies3=sum(1 for j in st.alive if st.team[j] == t and j != q and tdist(g, st.head(j), qh)[0] <= 3),
                                    en_vis=sum(1 for n in near if n[1] <= 3), en_reach=sum(1 for n in near if n[0] <= reach(n[2])))
                    X = qexp[s][ph(st.round)]
                    X['turns'] += 1
                    if near and near[0][0] <= 3: X['en3'] += 1
                    if near and near[0][0] <= 6: X['en6'] += 1
                    if any(n[0] <= reach(n[2]) and n[1] <= 3 for n in near): X['reach_vis'] += 1
                    X['own_segs'] += own_segs; X['room'] += room
        elif kind == 'turn':
            s = side(info['team']); p = ph(info['round']); a = info['action']
            C = F[s][p]
            if a and a[0] == 'move':
                C['moves'] += 1
                if len(a[1]) > 1: C['multi'] += 1
            elif a and a[0] == 'split':
                C['splits'] += 1
            if info['ate']:
                C['ate'] += info['ate']
                if info['round'] < 60:
                    for t in info['path'][-1:]:
                        C['ate_' + zone(t)] += info['ate']
            q = queens[info['team']]
            if info['id'] == q:
                Q = qinfo[s]; Q['turns'] += 1
                if a and a[0] == 'move' and len(a[1]) > 1: Q['multi'] += 1
                Q['ate'][p] += info['ate']
                if info['died'] is None: Q['tiles'].append(st.head(q))
                qlast[s] = dict(act=(a[0], len(a[1]) if a and a[0] == 'move' else (a[1] if a and len(a) > 1 else None)) if a else None,
                                path=len(info['path']))
            for k in info['killed']:
                pass
        elif kind == 'death':
            rnd, i, team, reason, length, detail = info
            s = side(team); p = ph(rnd)
            C = F[s][p]
            key = reason + ('/' + detail if detail else '')
            C['dead_n'] += 1; C['dead_len'] += length
            C['d:' + key] += 1
            # head-on: who initiated
            if reason == 'head_to_head' and detail.startswith('hit_by_enemy'):
                F['th' if s == 'us' else 'us'][p]['trade_init'] += 1
            if queens[team] == i:
                q = pre_q.get(s, {})
                o = dict(round=rnd, reason=key, length=length, pre=q, last=qlast.get(s),
                         ring_tiles=len(set(qinfo[s]['tiles'])))
                if q and q.get('round') == rnd and q.get('near'):
                    o['nearest'] = q['near'][0]
                qdeath[s] = o
        elif kind == 'round_end':
            r = info
            if first_contact is None:
                for i in st.alive:
                    if st.team[i] != us: continue
                    h = st.head(i)
                    if any(st.team[j] != us and g.in_window(h, st.head(j)) for j in st.alive):
                        first_contact = r; break
            if r in (30, 60, 100, 150, 200, 300, 360, 400, 430, 460, 499):
                snaps[r] = {side(t): sorted((st.length(i) for i in st.alive if st.team[i] == t), reverse=True)[:6] +
                            [len([i for i in st.alive if st.team[i] == t]), sum(st.length(i) for i in st.alive if st.team[i] == t)]
                            for t in ('A', 'B')}
                for t, q in queens.items():
                    snaps[r]['q' + side(t)] = st.length(q) if q in st.alive else 0
    # big kills: our long dragons killed by enemy head-on (derived from death detail & length) recorded above in F
    out = dict(F={s: {p: dict(c) for p, c in F[s].items()} for s in F}, snaps=snaps, qdeath=qdeath,
               qinfo={s: dict(turns=v['turns'], multi=v['multi'], ate=dict(v['ate'])) for s, v in qinfo.items()},
               first_contact=first_contact, qexp={s: {p: dict(c) for p, c in qexp[s].items()} for s in qexp}, W=g.W, H=g.H, starters=len(starts['us']), name=g.name)
    return out, d['map']


if __name__ == '__main__':
    rows = {r['id']: r for r in json.load(open(os.path.join(os.path.dirname(__file__), '../data/rows.json')))}
    outp = sys.argv[1]
    ids = [int(x) for x in sys.argv[2:]] if len(sys.argv) > 2 else None
    repd = os.path.join(os.path.dirname(__file__), '../data/rep')
    mapd = os.path.join(os.path.dirname(__file__), '../data/maps')
    done = set()
    for op in (os.path.join(os.path.dirname(outp), 'games1.jsonl'), os.path.join(os.path.dirname(outp), 'games2.jsonl')):
        if os.path.exists(op):
            done |= {json.loads(l)['id'] for l in open(op) if l.strip()}
    fo = open(outp, 'a')
    files = sorted(os.listdir(repd))
    shard = int(os.path.basename(outp).split('games')[1].split('.')[0]) % 2
    for f in files:
        if not f.endswith('.replay.gz'): continue
        i = int(f.split('.')[0])
        if i in done or (ids and i not in ids) or i not in rows: continue
        if i % 2 != shard and i not in done: continue
        r = rows[i]
        try:
            o, mtext = analyse(os.path.join(repd, f), r['us'])
        except Exception as e:
            print(i, 'ERR', e, file=sys.stderr); continue
        mp = os.path.join(mapd, r['map'] + '.map')
        if not os.path.exists(mp) and r['suite'] != 'holdout':
            open(mp, 'w').write(mtext)
        o.update(id=i, res=r['res'], opp=r['opp'], map=r['map'], suite=r['suite'], us=r['us'])
        fo.write(json.dumps(o) + '\n'); fo.flush()
