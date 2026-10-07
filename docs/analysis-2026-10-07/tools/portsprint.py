"""Per side, r0-60 and r0-100: true portal crossings (consecutive path tiles not torus-adjacent), multi-step moves
(sprints: >1 step in the action), and paid steps (steps beyond ceil(L/4)). python3 portsprint.py games.json out ids"""
import json, sys, os, collections
sys.path.insert(0, os.path.dirname(__file__)); import decode; decode.FAST = True
G = {x['id']: x for x in json.load(open(sys.argv[1]))}
o = open(sys.argv[2], 'w')
for gid in map(int, sys.argv[3:]):
    p = os.path.join(os.path.dirname(sys.argv[2]) or '.', 'rep', f'{gid}.replay.gz')
    if not os.path.exists(p): continue
    x = G[gid]; us = x['meta']['our_seat']; d = decode.load(p); g = decode.GameMap(d['map'])
    def adj(a, b):
        dx = abs(a[0] - b[0]); dy = abs(a[1] - b[1]); return min(dx, g.W - dx) + min(dy, g.H - dy) <= 1
    C = collections.Counter()
    for kind, st, info in decode.walk(d):
        if kind != 'turn' or info['round'] >= 100: continue
        s = 'A' if info['team'] == 'A' else 'B'; ph = '60' if info['round'] < 60 else '100'
        a = info['action']
        if not a or a[0] != 'move': continue
        n = len(a[1]); free = (info['len_before'] + 3) // 4
        prev = info['head_before']
        for t in info['path']:
            if not adj(prev, t): C[(s, ph, 'portal')] += 1
            prev = t
        if n > 1: C[(s, ph, 'multi')] += 1
        if n > free: C[(s, ph, 'paid')] += n - free
        C[(s, ph, 'moves')] += 1
    o.write(json.dumps(dict(id=gid, map=x['map'], us=us, won=x['winner'] == us, c=[[*k, v] for k, v in C.items()])) + '\n')
