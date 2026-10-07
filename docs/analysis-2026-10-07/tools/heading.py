"""Heading persistence r0-20: for every dragon, per round head displacement vector (torus); share of rounds whose
displacement direction (sign of dx, dy) equals the previous round's. Also net/gross ratio over r0-20 for dragons alive
all along (starters). python3 heading.py games.json out ids"""
import json, sys, os, statistics as S
sys.path.insert(0, os.path.dirname(__file__)); import decode; decode.FAST = True
G = {x['id']: x for x in json.load(open(sys.argv[1]))}
o = open(sys.argv[2], 'w')
for gid in map(int, sys.argv[3:]):
    p = os.path.join(os.path.dirname(sys.argv[2]) or '.', 'rep', f'{gid}.replay.gz')
    if not os.path.exists(p): continue
    x = G[gid]; us = x['meta']['our_seat']; d = decode.load(p); g = decode.GameMap(d['map'])
    def dv(a, b):
        dx = (b[0] - a[0]) % g.W; dx = dx - g.W if dx > g.W // 2 else dx
        dy = (b[1] - a[1]) % g.H; dy = dy - g.H if dy > g.H // 2 else dy
        return dx, dy
    H = {}; team = {}
    for kind, st, info in decode.walk(d):
        if kind == 'round_end':
            for i in st.alive: H.setdefault(i, {})[info] = st.head(i); team[i] = st.team[i]
            if info >= 20: break
    out = {}
    for s in 'AB':
        same = tot = 0; ratios = []
        for i, h in H.items():
            if team[i] != s: continue
            rs = sorted(h)
            prev = None; gross = 0
            for a, b in zip(rs, rs[1:]):
                if b != a + 1: prev = None; continue
                v = dv(h[a], h[b]); sg = ((v[0] > 0) - (v[0] < 0), (v[1] > 0) - (v[1] < 0))
                gross += abs(v[0]) + abs(v[1])
                if prev is not None: tot += 1; same += sg == prev
                prev = sg
            if rs and rs[0] <= 0 and rs[-1] >= 20 and gross:
                n = dv(h[rs[0]], h[20]); ratios.append((abs(n[0]) + abs(n[1])) / gross)
        out[s] = dict(persist=same / tot if tot else 0, straight=S.mean(ratios) if ratios else 0)
    o.write(json.dumps(dict(id=gid, map=x['map'], us=us, won=x['winner'] == us, out=out)) + '\n')
