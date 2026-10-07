"""Per queen-turn situation -> died-before-next-turn, for both queens, every game (r>=60).
python3 qhaz.py out.jsonl shard(0|1)"""
import json, sys, os, collections
sys.path.insert(0, os.path.dirname(__file__))
import decode
decode.FAST = True
from game import tdist, reach

rows = {r['id']: r for r in json.load(open(os.path.join(os.path.dirname(__file__), '../data/rows.json')))}
outp, shard = sys.argv[1], int(sys.argv[2])
repd = os.path.join(os.path.dirname(__file__), '../data/rep')
done = set()
if os.path.exists(outp):
    done = {json.loads(l)['id'] for l in open(outp) if l.strip()}
fo = open(outp, 'a')


def bucket_d(d):
    return str(d) if d <= 6 else '7-8' if d <= 8 else '9+'


for f in sorted(os.listdir(repd)):
    i = int(f.split('.')[0])
    if i % 2 != shard or i in done or i not in rows:
        continue
    r = rows[i]; us = r['us']
    d = decode.load(os.path.join(repd, f)); g = decode.GameMap(d['map'])
    queens = {t: k for k, (t, _) in enumerate(g.dragons[:2])}
    C = collections.Counter()
    pend = {}
    for kind, st, info in decode.walk(d):
        if kind == 'pre':
            k = info
            for t, q in queens.items():
                if k != q or st.round < 60:
                    continue
                s = 'us' if t == us else 'th'
                qh = st.head(q); L = st.length(q)
                best = None
                for j in st.alive:
                    if st.team[j] == t:
                        continue
                    dd, ch = tdist(g, st.head(j), qh)
                    if best is None or dd < best[0]:
                        best = (dd, ch, st.length(j))
                allies3 = sum(1 for j in st.alive if st.team[j] == t and j != q and tdist(g, st.head(j), qh)[0] <= 3)
                inreach = sum(1 for j in st.alive if st.team[j] != t and tdist(g, st.head(j), qh)[0] <= reach(st.length(j)) and tdist(g, st.head(j), qh)[0] <= 8)
                ph = '060-299' if st.round < 300 else '300-429' if st.round < 430 else '430+'
                key = (s, ph, bucket_d(best[0]) if best else 'none', 'vis' if best and best[1] <= 3 else 'hid',
                       min(inreach, 2), min(allies3, 2), '2-3' if L <= 3 else '4-9' if L < 10 else '10+')
                pend[s] = key
        elif kind == 'turn':
            for t, q in queens.items():
                if info['id'] == q and st.round >= 60:
                    s = 'us' if t == us else 'th'
                    if s in pend and info['died'] is None:
                        steps = len(info['action'][1]) if info['action'] and info['action'][0] == 'move' else 0
                        pend[s] = pend[s] + ('m%d' % min(steps, 2),)
        elif kind == 'death':
            rnd, k, team, reason, length, detail = info
            for t, q in queens.items():
                if k == q:
                    s = 'us' if t == us else 'th'
                    if s in pend:
                        key = pend.pop(s)
                        if len(key) == 7:
                            key = key + ('m?',)
                        C[json.dumps(list(key) + ['died', reason + '/' + detail])] += 1
        elif kind == 'round_end':
            for s, key in list(pend.items()):
                if len(key) == 7:
                    key = key + ('m?',)
                C[json.dumps(list(key) + ['alive', ''])] += 1
            pend = {}
    fo.write(json.dumps(dict(id=i, C=C)) + '\n'); fo.flush()
