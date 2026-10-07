"""Locally observable safety proxy for the queen: rounds since any ENEMY segment was inside her 7x7 window (quiet),
and the same for a 15x15 'neighbourhood' (r=7, what nearby allies could report). Every 10 rounds per queen: quiet3,
quiet7, sealed (no enemy head reaches her through free tiles, unlimited), died-within-30.
python3 qquiet.py games.json out.jsonl ids"""
import json, sys, os
sys.path.insert(0, os.path.dirname(__file__)); import decode; decode.FAST = True
G = {x['id']: x for x in json.load(open(sys.argv[1]))}
o = open(sys.argv[2], 'w')
for gid in map(int, sys.argv[3:]):
    p = os.path.join(os.path.dirname(sys.argv[2]) or '.', 'rep', f'{gid}.replay.gz')
    if not os.path.exists(p): continue
    x = G[gid]; us = x['meta']['our_seat']; d = decode.load(p); g = decode.GameMap(d['map'])
    last3 = {0: 0, 1: 0}; last7 = {0: 0, 1: 0}; died = {}; rows = []
    for kind, st, info in decode.walk(d):
        if kind == 'death' and info[1] in (0, 1): died[info[1]] = info[0]
        if kind != 'round_end': continue
        r = info
        for q in (0, 1):
            if q not in st.alive: continue
            qh = st.head(q); tq = st.team[q]
            en = [t for j in st.alive if st.team[j] != tq for t in st.body[j]]
            if any(g.in_window(qh, t, 3) for t in en): last3[q] = r
            if any(g.in_window(qh, t, 7) for t in en): last7[q] = r
            if r % 10 == 0 and r >= 60:
                occ = set(st.occupied()); occ.discard(qh)
                ehs = {st.head(j) for j in st.alive if st.team[j] != tq}
                reg = g.bfs(qh, blocked=occ - ehs)
                rows.append(dict(r=r, us=tq == us, L=len(st.body[q]), q3=r - last3[q], q7=r - last7[q],
                                 sealed=not any(h in reg for h in ehs), q=q))
    for x2 in rows: x2['d30'] = (x2['q'] in died and x2['r'] < died[x2['q']] <= x2['r'] + 30)
    o.write(json.dumps(dict(id=gid, rows=rows)) + '\n'); o.flush()
