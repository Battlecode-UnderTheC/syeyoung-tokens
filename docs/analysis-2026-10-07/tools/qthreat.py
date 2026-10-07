"""Window-local threat signal for a queen: an enemy head inside her 7x7 window that can reach her head through free
tiles INSIDE the window (walls/kelp/portals as the map has them, bodies block) within (its move reach + 2) steps.
Records rounds since last threatened (t), plus quiet (rounds since any enemy segment in window), sealed, died<=30.
python3 qthreat.py games.json out ids"""
import json, sys, os
sys.path.insert(0, os.path.dirname(__file__)); import decode; decode.FAST = True
G = {x['id']: x for x in json.load(open(sys.argv[1]))}
o = open(sys.argv[2], 'w')
def reach(L): return (L + 3) // 4 + max(0, L - 2)
for gid in map(int, sys.argv[3:]):
    p = os.path.join(os.path.dirname(sys.argv[2]) or '.', 'rep', f'{gid}.replay.gz')
    if not os.path.exists(p): continue
    x = G[gid]; us = x['meta']['our_seat']; d = decode.load(p); g = decode.GameMap(d['map'])
    lastT = {0: 0, 1: 0}; lastQ = {0: 0, 1: 0}; died = {}; rows = []
    for kind, st, info in decode.walk(d):
        if kind == 'death' and info[1] in (0, 1): died[info[1]] = info[0]
        if kind != 'round_end': continue
        r = info; occ = None
        for q in (0, 1):
            if q not in st.alive: continue
            qh = st.head(q); tq = st.team[q]
            ens = [j for j in st.alive if st.team[j] != tq]
            vis_seg = any(g.in_window(qh, t, 3) for j in ens for t in st.body[j])
            if vis_seg: lastQ[q] = r
            vh = [j for j in ens if g.in_window(qh, st.head(j), 3)]
            if vh:
                if occ is None: occ = set(st.occupied())
                outside = None
                for j in vh:
                    h = st.head(j); lim = reach(len(st.body[j])) + 2
                    # BFS restricted to the queen's window
                    dist = {h: 0}; fr = [h]; dd = 0; hit = False
                    while fr and dd < lim and not hit:
                        dd += 1; nx = []
                        for a in fr:
                            for dr in 'NESW':
                                b = g._step[(a, dr)]
                                if b is None or b in dist or not g.in_window(qh, b, 3): continue
                                if b == qh: hit = True; break
                                if b in occ: continue
                                dist[b] = dd; nx.append(b)
                            if hit: break
                        fr = nx
                    if hit: lastT[q] = r; break
            if r % 10 == 0 and r >= 60:
                if occ is None: occ = set(st.occupied())
                blk = occ - {qh}; ehs = {st.head(j) for j in ens}
                reg = g.bfs(qh, blocked=blk - ehs)
                rows.append(dict(r=r, us=tq == us, L=len(st.body[q]), t=r - lastT[q], qu=r - lastQ[q], sealed=not any(h in reg for h in ehs), q=q))
    for y in rows: y['d30'] = (y['q'] in died and y['r'] < died[y['q']] <= y['r'] + 30)
    o.write(json.dumps(dict(id=gid, map=x['map'], rows=rows)) + '\n'); o.flush()
