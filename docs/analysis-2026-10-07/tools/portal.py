"""arena-r18: portal use and opening economy per side. python3 portal.py out.jsonl repdir ids.json"""
import json, sys, os, collections
sys.path.insert(0, os.path.dirname(__file__))
import decode
decode.SKIP = {2, 5, 6, 7, 8, 12}
rows = {r['id']: r for r in json.load(open(os.path.join(os.path.dirname(__file__), '../data/rows.json')))}
outp, repd, ids = sys.argv[1], sys.argv[2], json.load(open(sys.argv[3]))
done = set()
if os.path.exists(outp): done = {json.loads(l)['id'] for l in open(outp)}
fo = open(outp, 'a')
DXY = {'N': (0, -1), 'S': (0, 1), 'E': (1, 0), 'W': (-1, 0)}
for i in ids:
    p = os.path.join(repd, f'{i}.replay.gz')
    if i in done or not os.path.exists(p): continue
    r = rows[i]; us = r['us']
    d = decode.load(p); g = decode.GameMap(d['map'])
    if g.portal_count == 0:
        fo.write(json.dumps(dict(id=i, portals=0)) + '\n'); continue
    side = lambda t: 'us' if t == us else 'th'
    C = {'us': collections.Counter(), 'th': collections.Counter()}
    first = {}
    for kind, st, info in decode.walk(d):
        if kind == 'turn':
            if info['round'] >= 150: break
            a = info['action']; s = side(info['team'])
            if a and a[0] == 'move':
                t = info['head_before']
                for dd in a[1]:
                    n = g.step(t, dd)
                    if n is None: break
                    dx, dy = DXY[dd]
                    if n != ((t[0] + dx) % g.W, (t[1] + dy) % g.H):
                        b = 'p50' if info['round'] < 50 else 'p100' if info['round'] < 100 else 'p150'
                        C[s][b] += 1; first.setdefault(s, info['round'])
                    t = n
            if info['ate']:
                C[s]['ate' + ('50' if info['round'] < 50 else '100' if info['round'] < 100 else '150')] += info['ate']
    fo.write(json.dumps(dict(id=i, portals=g.portal_count, us=dict(C['us']), th=dict(C['th']), first=first)) + '\n'); fo.flush()
