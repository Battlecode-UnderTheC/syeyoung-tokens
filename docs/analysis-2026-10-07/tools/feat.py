"""deep: one pass over a replay -> per-game timeline features (JSON line), both sides.
python3 feat.py <games.json> <out.jsonl> [ids...]
Sides: 'us' = our seat from meta.our_seat (server games) or a/b == OURS name; 'th' = opponent.
Zones from multi-source BFS distance to each side's starting heads: own / mid (|du-dt|<=2) / far.
"""
import json, sys, os, collections
sys.path.insert(0, os.path.dirname(__file__))
import decode
decode.FAST = True

SNAP = 10


def run(path, us):
    d = decode.load(path)
    g = decode.GameMap(d["map"])
    side = lambda t: 'us' if t == us else 'th'
    queens = {}
    for k, (t, _) in enumerate(g.dragons[:2]):
        queens[side(t)] = k
    starts = {s: [b[0] for t, b in g.dragons if side(t) == s] for s in ('us', 'th')}
    D = {}
    for s in ('us', 'th'):
        D[s] = {}
        for h in starts[s]:
            for t, v in g.bfs(h).items():
                if v < D[s].get(t, 1 << 30):
                    D[s][t] = v
    def zone(s, t):
        o = 'th' if s == 'us' else 'us'
        a, b = D[s].get(t, 999), D[o].get(t, 999)
        return 'own' if a < b - 2 else 'far' if b < a - 2 else 'mid'
    def tor(a, b):
        dx = abs(a[0] - b[0]); dy = abs(a[1] - b[1])
        return min(dx, g.W - dx) + min(dy, g.H - dy)
    C = {s: collections.Counter() for s in ('us', 'th')}
    tl = []          # snapshots
    deaths = []
    qdist_max = {'us': 0, 'th': 0}
    for kind, st, info in decode.walk(d):
        if kind == 'turn':
            s = side(info['team'])
            if info['ate']:
                C[s]['eat'] += info['ate']
                # attribute eats along path by zone (approx: zone of final head)
                hd = info['path'][-1] if info['path'] else info['head_before']
                C[s]['eat_' + zone(s, hd)] += info['ate']
            p = info['path']
            if len(p) > 1:
                C[s]['multi'] += 1
            prev = info['head_before']
            for t in p:
                if tor(prev, t) > 1:
                    C[s]['portal'] += 1
                prev = t
            if info['killed']:
                C[s]['kills_by_mover'] += len(info['killed'])
        elif kind == 'split':
            C[side(info[3])]['split'] += 1
        elif kind == 'death':
            r, i, t, why, ln, det = info
            s = side(t)
            key = why + (':' + det if det else '')
            C[s]['d_' + key] += 1
            C[s]['dlen'] += ln
            hd = st.body[i][0] if st.body[i] else None
            deaths.append(dict(r=r, s=s, id=i, len=ln, why=key, q=(i == queens.get(s)),
                               z=(zone(s, hd) if hd else None)))
        elif kind == 'round_end':
            r = info
            for s in ('us', 'th'):
                q = queens.get(s)
                if q is not None and q in st.alive:
                    qdist_max[s] = max(qdist_max[s], D[s].get(st.head(q), 0))
            if r % SNAP == 0 or r == 499:
                snap = {'r': r}
                heads = {s: [] for s in ('us', 'th')}
                for i in st.alive:
                    heads[side(st.team[i])].append(i)
                for s in ('us', 'th'):
                    L = [len(st.body[i]) for i in heads[s]]
                    q = queens.get(s)
                    ql = len(st.body[q]) if (q is not None and q in st.alive) else 0
                    zc = collections.Counter(zone(s, st.head(i)) for i in heads[s])
                    sp = [D[s].get(st.head(i), 0) for i in heads[s]]
                    snap[s] = dict(n=len(L), tot=sum(L), lon=max(L, default=0), q=ql,
                                   qd=(D[s].get(st.head(q), 0) if ql else -1),
                                   spread=(sum(sp) / len(sp) if sp else 0), far=max(sp, default=0),
                                   z_own=zc['own'], z_mid=zc['mid'], z_far=zc['far'],
                                   **{k: v for k, v in C[s].items()})
                # contact: our heads within 2 of an enemy head
                eh = [st.head(i) for i in heads['th']]
                snap['contact'] = sum(1 for i in heads['us'] if any(tor(st.head(i), h) <= 2 for h in eh))
                tl.append(snap)
    res = d['result']
    return dict(map=g.name, W=g.W, H=g.H, starters=sum(1 for t, b in g.dragons if t == us),
                portals=g.portal_count, kelp=g.kelp_count, tl=tl, deaths=deaths, qdmax=qdist_max,
                end=res['endReason'])


def main():
    games = {x['id']: x for x in json.load(open(sys.argv[1]))}
    out = sys.argv[2]
    ids = [int(a) for a in sys.argv[3:]] or list(games)
    done = set()
    if os.path.exists(out):
        for l in open(out):
            try: done.add(json.loads(l)['id'])
            except Exception: pass
    with open(out, 'a') as f:
        for i in ids:
            if i in done: continue
            x = games[i]
            p = os.path.join(os.path.dirname(out), 'rep', f'{i}.replay.gz')
            if not os.path.exists(p): continue
            us = x['meta']['our_seat'] if 'meta' in x and x['meta'].get('our_seat') else ('A' if x['a'].startswith('ring') else 'B')
            try:
                r = run(p, us)
            except Exception as e:
                print(i, 'ERR', e, file=sys.stderr); continue
            me = 'A' if us == 'A' else 'B'
            r.update(id=i, won=(x['winner'] == us), reason=x['reason'], opp=x['meta'].get('opponent') if 'meta' in x else x['b'],
                     ranked=x.get('meta', {}).get('ranked'), mapkey=x['map'])
            f.write(json.dumps(r) + '\n'); f.flush()


if __name__ == '__main__':
    main()
