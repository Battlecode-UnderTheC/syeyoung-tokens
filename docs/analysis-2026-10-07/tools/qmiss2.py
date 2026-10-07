"""Queen-kill opportunities in detail: at the start of a non-queen dragon's turn, if the ENEMY queen's head is reachable
through free tiles within its move reach, record: side, hunter length, BFS distance, free steps, did it strike (killed
her this turn), steps it actually took, did it move closer (Chebyshev), round. One row per opportunity.
python3 qmiss.py games.json out ids"""
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
    queens = {g.dragons[k][0]: k for k in (0, 1)}
    cur = None
    for kind, st, info in decode.walk(d):
        if kind == 'pre':
            cur = None; i = info
            if i in (0, 1): continue
            t = st.team[i]; q = queens['B' if t == 'A' else 'A']
            if q not in st.alive: continue
            h = st.head(i); qh = st.head(q); L = len(st.body[i]); R = reach(L)
            if ch(h, qh) > R: continue
            occ = set(st.occupied()); occ.discard(qh)
            dist = g.bfs(h, blocked=occ, limit=R)
            if qh in dist:
                seen = sum(1 for j in st.alive if j != i and st.team[j] == t and g.in_window(st.head(j), qh, 3))
                cur = dict(seen=seen, id=gid, side='us' if t == us else 'th', L=L, dist=dist[qh], free=(L + 3) // 4, r=st.round, q=q, qh=qh, ch0=ch(h, qh))
        elif kind == 'turn' and cur and info['id'] not in (0, 1):
            a = info['action']
            cur['struck'] = cur['q'] in info['killed']
            cur['steps'] = len(a[1]) if a and a[0] == 'move' else 0
            cur['act'] = a[0] if a else None
            cur['died'] = info['died']
            nh = info['path'][-1] if info['path'] else info['head_before']
            cur['closer'] = ch(nh, cur['qh']) < cur['ch0'] if info['died'] is None else None
            cur.pop('qh'); o.write(json.dumps(cur) + '\n'); cur = None
