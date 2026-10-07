"""python3 qpos.py game round...  -> our queen head (and enemy queen) at those rounds"""
import json, sys, os
sys.path.insert(0, os.path.dirname(__file__))
import decode
decode.FAST = True
rows = {r['id']: r for r in json.load(open(os.path.join(os.path.dirname(__file__), '../data/rows.json')))}
g_id = int(sys.argv[1]); want = set(int(x) for x in sys.argv[2:])
r = rows[g_id]; us = r['us']
p = os.path.join(os.path.dirname(__file__), f'../data/rep/{g_id}.replay.gz')
d = decode.load(p); g = decode.GameMap(d['map'])
queens = {t: k for k, (t, _) in enumerate(g.dragons[:2])}
for kind, st, info in decode.walk(d):
    if kind == 'round_end' and info in want:
        q = queens[us]; e = queens['B' if us == 'A' else 'A']
        print(g_id, g.name, g.W, g.H, 'r', info, 'q', q, st.head(q) if q in st.alive else None, st.length(q) if q in st.alive else 0,
              'eq', st.head(e) if e in st.alive else None)
