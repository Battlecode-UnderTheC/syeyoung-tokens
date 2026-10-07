"""Our-queen enclosures: every 30 rounds r150-450, if no enemy head can reach our queen through free tiles, report region
size (free tiles reachable, unlimited), border body tiles by owner (own/enemy), beds inside the region, own heads inside.
python3 seal2.py games.json out.jsonl ids"""
import json, sys, os
sys.path.insert(0, os.path.dirname(__file__)); import decode; decode.FAST = True
G = {x['id']: x for x in json.load(open(sys.argv[1]))}
o = open(sys.argv[2], 'w')
for gid in map(int, sys.argv[3:]):
    p = os.path.join(os.path.dirname(sys.argv[2]) or '.', 'rep', f'{gid}.replay.gz')
    if not os.path.exists(p): continue
    x = G[gid]; d = decode.load(p); g = decode.GameMap(d['map'])
    beds = None; rows = []
    for kind, st, info in decode.walk(d):
        if kind != 'round_end' or info % 30 or info < 150: continue
        occ = st.occupied()
        for q in (0, 1):
            if q not in st.alive: continue
            qh = st.head(q); tq = st.team[q]
            blk = set(occ); blk.discard(qh)
            ehs = {st.head(j) for j in st.alive if st.team[j] != tq}
            reg = g.bfs(qh, blocked=blk)
            if any(h in reg for h in ehs):
                rows.append(dict(r=info, q=q, us=tq == x['meta']['our_seat'], sealed=False)); continue
            border = {}
            for t in reg:
                for dd in 'NESW':
                    n = g._step[(t, dd)]
                    if n is not None and n in occ and n not in reg: border[n] = occ[n]
            own = sum(1 for t, i in border.items() if st.team[i] == tq)
            # adjacent enemy heads count as reachable-in-one: border enemy heads
            ownh = sum(1 for j in st.alive if j != q and st.team[j] == tq and st.head(j) in reg)
            rows.append(dict(r=info, q=q, us=tq == x['meta']['our_seat'], sealed=True, region=len(reg), own=own,
                             en=len(border) - own, beds=sum(1 for t in reg if t in st.beds), pearls=sum(1 for t in reg if t in st.pearls), qd=min(reg.get(t,999) for t in st.pearls) if st.pearls else 999, ownh=ownh, L=len(st.body[q])))
    o.write(json.dumps(dict(id=gid, map=x['map'], won=x['winner'] == x['meta']['our_seat'], rows=rows)) + '\n'); o.flush()
