"""Big-queen safety. Every 10 rounds, for each queen with L>=8: enemy-head BFS distance to her head through free tiles
(eb, 99 if unreachable within 40 = sealed), own-body share of map tiles within 5 steps of her head (own5), enemy-body
share (en5), and the round she died (or None). python3 fortress.py games.json out.jsonl [ids]"""
import json, sys, os
sys.path.insert(0, os.path.dirname(__file__)); import decode; decode.FAST = True
def run(p):
    d = decode.load(p); g = decode.GameMap(d['map'])
    died = {}; rows = []
    snaps = []
    for kind, st, info in decode.walk(d):
        if kind == 'death':
            r, i, t, why, ln, det = info
            if i in (0, 1): died[i] = (r, why)
        elif kind == 'round_end' and info % 10 == 0:
            occ = st.occupied()
            for q in (0, 1):
                if q not in st.alive or len(st.body[q]) < MINL: continue
                qh = st.head(q); tq = st.team[q]
                near = g.bfs(qh, limit=5)
                own = sum(1 for t in near if t in occ and st.team[occ[t]] == tq)
                en = sum(1 for t in near if t in occ and st.team[occ[t]] != tq)
                blk = set(occ); blk.discard(qh)
                ehs = {st.head(j) for j in st.alive if st.team[j] != tq}
                dist = g.bfs(qh, blocked=blk - ehs, limit=40)
                eb = min((dist[h] for h in ehs if h in dist), default=99)
                if eb == 99:
                    full = g.bfs(qh, blocked=blk - ehs)
                    eb = 999 if not any(h in full for h in ehs) else 99
                rows.append(dict(r=info, q=q, team=tq, L=len(st.body[q]), eb=eb, own5=own / len(near), en5=en / len(near), nown=sum(1 for j in st.alive if st.team[j] == tq)))
    for x in rows: x['died'] = died.get(x['q'], (None, None))[0]; x['why'] = died.get(x['q'], (None, None))[1]
    return rows
MINL = int(os.environ.get("MINL", "8"))
G = {x['id']: x for x in json.load(open(sys.argv[1]))}
ids = [int(a) for a in sys.argv[3:]] or list(G)
o = open(sys.argv[2], 'a')
for gid in ids:
    p = os.path.join(os.path.dirname(sys.argv[2]) or '.', 'rep', f'{gid}.replay.gz')
    if not os.path.exists(p): continue
    try: rows = run(p)
    except Exception as e: print(gid, e, file=sys.stderr); continue
    x = G[gid]
    o.write(json.dumps(dict(id=gid, map=x['map'], winner=x['winner'], us=x['meta']['our_seat'], opp=x['meta']['opponent'], rows=rows)) + '\n'); o.flush()
