"""Our queen's last move before a head-on death: nearest pearl BFS distance before and after; ate on it?
python3 qlastfood.py games.json out ids"""
import json, sys, os
sys.path.insert(0, os.path.dirname(__file__)); import decode; decode.FAST = True
G = {x['id']: x for x in json.load(open(sys.argv[1]))}
o = open(sys.argv[2], 'w')
for gid in map(int, sys.argv[3:]):
    p = os.path.join(os.path.dirname(sys.argv[2]) or '.', 'rep', f'{gid}.replay.gz')
    if not os.path.exists(p): continue
    x = G[gid]; us = x['meta']['our_seat']; d = decode.load(p); g = decode.GameMap(d['map'])
    q = next(k for k, (t, _) in enumerate(g.dragons[:2]) if t == us)
    snap = None; mover = None; last = None
    for kind, st, info in decode.walk(d):
        if kind == 'pre':
            mover = info
            if info == q:
                occ = set(st.occupied()) - {st.head(q)}
                dist = g.bfs(st.head(q), blocked=occ, limit=8)
                pv = [t for t in st.pearls if g.in_window(st.head(q), t, 3)]
                snap = dict(pd=min((dist[t] for t in pv if t in dist), default=99), pv=len(pv))
        elif kind == 'turn' and info['id'] == q and snap:
            snap['ate'] = info['ate']
            if info['died'] is None:
                occ = set(st.occupied()) - {st.head(q)}
                dist = g.bfs(st.head(q), blocked=occ, limit=8)
                snap['pd_after'] = min((dist[t] for t in st.pearls if t in dist), default=99)
            last = dict(snap)
        elif kind == 'death' and info[1] == q:
            if info[3] == 'head_to_head' and mover != q and last:
                last.update(id=gid); o.write(json.dumps(last) + '\n')
            break
