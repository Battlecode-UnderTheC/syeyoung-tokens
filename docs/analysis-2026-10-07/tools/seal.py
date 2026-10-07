"""For sealed queen samples (no enemy head reachable within 40), flood the queen's free region (limit 400) and count the
body tiles on its border by owner, plus region size. python3 seal.py games.json gid..."""
import json, sys, os
sys.path.insert(0, os.path.dirname(__file__)); import decode; decode.FAST = True
G = {x['id']: x for x in json.load(open(sys.argv[1]))}
for gid in map(int, sys.argv[2:]):
    d = decode.load(f'top/rep/{gid}.replay.gz'); g = decode.GameMap(d['map'])
    for kind, st, info in decode.walk(d):
        if kind != 'round_end' or info % 30 or info < 150: continue
        occ = st.occupied()
        for q in (0, 1):
            if q not in st.alive or len(st.body[q]) < 8: continue
            qh = st.head(q); tq = st.team[q]
            blk = set(occ); blk.discard(qh)
            reg = g.bfs(qh, blocked=blk, limit=60)
            ehs = {st.head(j) for j in st.alive if st.team[j] != tq}
            border = {}
            for t in reg:
                for dd in 'NESW':
                    n = g._step[(t, dd)]
                    if n is not None and n in occ and n not in reg: border[n] = occ[n]
            own = sum(1 for t, i in border.items() if st.team[i] == tq); en = len(border) - own
            ownheads = sum(1 for j in st.alive if st.team[j] == tq and j != q and st.head(j) in border or st.head(j) in reg and st.team[j]==tq and j!=q)
            print(gid, info, 'q', q, 'L', len(st.body[q]), 'region', len(reg), 'full' if max(reg.values()) < 60 else 'open', 'border own/en', own, en, 'own heads in/at region', ownheads)
