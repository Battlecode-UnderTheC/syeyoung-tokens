"""arena-r18: our queen's death, in detail. python3 qkill.py out.jsonl shard"""
import json, sys, os, collections
sys.path.insert(0, os.path.dirname(__file__))
import decode
decode.FAST = True
from game import tdist
rows = {r['id']: r for r in json.load(open(os.path.join(os.path.dirname(__file__), '../data/rows.json')))}
outp, shard = sys.argv[1], int(sys.argv[2])
repd = os.path.join(os.path.dirname(__file__), '../data/rep')
done = set()
if os.path.exists(outp): done = {json.loads(l)['id'] for l in open(outp)}
fo = open(outp, 'a')
for f in sorted(os.listdir(repd)):
    i = int(f.split('.')[0])
    if i % 2 != shard or i in done: continue
    r = rows[i]; us = r['us']
    if not r['qus']: continue
    d = decode.load(os.path.join(repd, f)); g = decode.GameMap(d['map'])
    queens = {t: k for k, (t, _) in enumerate(g.dragons[:2])}
    q = queens[us]; eq = queens['B' if us == 'A' else 'A']
    hist = collections.deque(maxlen=12)   # per our-queen pre-turn: (round, qlen, [(dist,cheb,len,id) enemies within 10])
    pre_k = None; out = None; qlast = None; eq_dead = None; maxq = 0
    for kind, st, info in decode.walk(d):
        if kind == 'pre':
            if info == q and q in st.alive:
                qh = st.head(q)
                en = sorted((tdist(g, st.head(j), qh) + (st.length(j), j)) for j in st.alive if st.team[j] != us)
                al = sum(1 for j in st.alive if st.team[j] == us and j != q)
                hist.append((st.round, st.length(q), [e for e in en if e[0] <= 12][:8], al,
                             len(g.bfs(qh, blocked=set(st.occupied()) - {qh}, limit=6)) - 1))
                maxq = max(maxq, st.length(q))
            elif q in st.alive and st.team[info] != us:
                pre_k = (info, st.length(info), tdist(g, st.head(info), st.head(q)), st.round)
        elif kind == 'turn':
            if info['id'] == q:
                a = info['action']
                qlast = dict(round=info['round'], act=a[0] if a else None, steps=len(a[1]) if a and a[0] == 'move' else 0, ate=info['ate'])
        elif kind == 'death':
            rnd, j, team, reason, length, detail = info
            if j == eq: eq_dead = rnd
            if j == q:
                k = None
                if pre_k and pre_k[3] == rnd and detail.startswith('hit_by_enemy'):
                    k = dict(id=pre_k[0], len=pre_k[1], dist=pre_k[2][0], cheb=pre_k[2][1])
                # was the killer seen by the queen (in window) in her last turns? first sighting
                seen = [h[0] for h in hist if k and any(e[3] == k['id'] and e[1] <= 3 for e in h[2])]
                near5 = [h[0] for h in hist if k and any(e[3] == k['id'] and e[0] <= 5 for e in h[2])]
                last = hist[-1] if hist else None
                out = dict(id=i, round=rnd, reason=reason + '/' + detail, qlen=length, maxq=maxq, killer=k, eq_dead=eq_dead,
                           seen_rounds=len(seen), near5_rounds=len(near5), qlast=qlast,
                           last=dict(round=last[0], en=last[2][:4], allies=last[3], room=last[4]) if last else None)
                break
    if out: fo.write(json.dumps(out) + '\n'); fo.flush()
