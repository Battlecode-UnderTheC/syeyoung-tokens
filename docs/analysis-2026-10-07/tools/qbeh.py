"""arena-r18: our queen's behaviour after the enemy queen is dead.
python3 qbeh.py out.jsonl repdir ids.json"""
import json, sys, os, collections
sys.path.insert(0, os.path.dirname(__file__))
import decode
decode.FAST = True
from game import tdist
rows = {r['id']: r for r in json.load(open(os.path.join(os.path.dirname(__file__), '../data/rows.json')))}
outp, repd, ids = sys.argv[1], sys.argv[2], json.load(open(sys.argv[3]))
done = set()
if os.path.exists(outp): done = {json.loads(l)['id'] for l in open(outp)}
fo = open(outp, 'a')
for i in ids:
    p = os.path.join(repd, f'{i}.replay.gz')
    if i in done or not os.path.exists(p): continue
    r = rows[i]; us = r['us']
    if not r['qth']: continue
    t0 = r['qth']['round']
    d = decode.load(p); g = decode.GameMap(d['map'])
    queens = {t: k for k, (t, _) in enumerate(g.dragons[:2])}
    q = queens[us]
    C = collections.Counter(); anchor = None; maxd = 0; L0 = None; Lmax = 0; ph = collections.Counter()
    for kind, st, info in decode.walk(d):
        if kind == 'pre' and info == q and q in st.alive and st.round > t0:
            qh = st.head(q); L = st.length(q)
            if anchor is None: anchor = qh; L0 = L
            maxd = max(maxd, tdist(g, anchor, qh)[0]); Lmax = max(Lmax, L)
            en = [tdist(g, st.head(j), qh)[0] for j in st.alive if st.team[j] != us]
            C['turns'] += 1
            if en and min(en) <= 3: C['en3'] += 1
            if en and min(en) <= 6: C['en6'] += 1
            C['room_sum'] += len(g.bfs(qh, blocked=set(st.occupied()) - {qh}, limit=4)) - 1
        elif kind == 'turn' and info['id'] == q and info['round'] > t0:
            a = info['action']
            if a and a[0] == 'move':
                n = len(a[1]); free = (info['len_before'] + 3) // 4
                C['moves'] += 1; C['steps'] += n
                if n > 1: C['multi'] += 1
                C['paid'] += max(0, n - free)
                if n > 1: ph[info['round'] // 50 * 50] += 1
            elif a and a[0] == 'split':
                C['splits'] += 1
            C['ate'] += info['ate']
    o = dict(id=i, res=r['res'], t0=t0, qd=(r['qus'] or {}).get('round'), L0=L0, Lmax=Lmax, maxd=maxd, multi_by_round=dict(ph), **C)
    fo.write(json.dumps(o) + '\n'); fo.flush()
