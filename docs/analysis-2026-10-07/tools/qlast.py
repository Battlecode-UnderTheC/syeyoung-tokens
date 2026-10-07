"""Our queen's last move before a head-on death: at her last turn start, the killer's head distance (Chebyshev, and
BFS over free tiles); after her move, the same; her length; how many free tiles she had to step to (mobility); whether
any reachable tile within her reach was out of the killer's reach next turn. python3 qlast.py games.json out ids"""
import json, sys, os
sys.path.insert(0, os.path.dirname(__file__)); import decode; decode.FAST = True
G = {x['id']: x for x in json.load(open(sys.argv[1]))}
o = open(sys.argv[2], 'w')
def reach(L): return (L + 3) // 4 + max(0, L - 2)
for gid in map(int, sys.argv[3:]):
    p = os.path.join(os.path.dirname(sys.argv[2]) or '.', 'rep', f'{gid}.replay.gz')
    if not os.path.exists(p): continue
    x = G[gid]; us = x['meta']['our_seat']; d = decode.load(p); g = decode.GameMap(d['map'])
    def ch(a, b):
        dx = abs(a[0] - b[0]); dy = abs(a[1] - b[1]); return max(min(dx, g.W - dx), min(dy, g.H - dy))
    q = next(k for k, (t, _) in enumerate(g.dragons[:2]) if t == us)
    hist = []; mover = None
    for kind, st, info in decode.walk(d):
        if kind == 'pre':
            mover = info
            if info == q:
                hist.append(dict(r=st.round, qh=st.head(q), L=len(st.body[q]), heads={j: (st.head(j), len(st.body[j])) for j in st.alive if st.team[j] != us}))
        elif kind == 'turn' and info['id'] == q and hist:
            hist[-1]['after'] = st.head(q) if info['died'] is None else None
            hist[-1]['steps'] = len(info['action'][1]) if info['action'] and info['action'][0] == 'move' else 0
        elif kind == 'death' and info[1] == q:
            if info[3] == 'head_to_head' and mover is not None and mover != q and hist:
                h = hist[-1]; k = mover
                b = h['heads'].get(k)
                if b and h.get('after'):
                    res = dict(id=gid, L=h['L'], kl=b[1], before=ch(h['qh'], b[0]), after=ch(h['after'], b[0]), steps=h['steps'], round=info[0],
                               kreach=reach(b[1]), qreach=reach(h['L']))
                    o.write(json.dumps(res) + '\n')
            break
