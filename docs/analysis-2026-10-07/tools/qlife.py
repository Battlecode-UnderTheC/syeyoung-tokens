"""Queen economy per phase: steps moved, pearls eaten (bed tile vs other = death drop), splits made by the queen and
length given to children. python3 qlife.py games.json out.jsonl ids"""
import json, sys, os
sys.path.insert(0, os.path.dirname(__file__)); import decode; decode.FAST = True
G = {x['id']: x for x in json.load(open(sys.argv[1]))}
o = open(sys.argv[2], 'w')
PH = ((0, 60), (60, 150), (150, 250), (250, 350), (350, 450), (450, 501))
def ph(r):
    for k, (a, b) in enumerate(PH):
        if a <= r < b: return k
for gid in map(int, sys.argv[3:]):
    p = os.path.join(os.path.dirname(sys.argv[2]) or '.', 'rep', f'{gid}.replay.gz')
    if not os.path.exists(p): continue
    x = G[gid]; us = x['meta']['our_seat']; d = decode.load(p)
    S = {q: [dict(steps=0, bed=0, drop=0, splits=0, gave=0, alive=0) for _ in PH] for q in (0, 1)}
    team = {}; pearls = set()
    for kind, st, info in decode.walk(d):
        if kind == 'pre':
            if info in (0, 1): pearls = set(st.pearls); team[info] = st.team[info]
        elif kind == 'turn' and info['id'] in (0, 1):
            k = ph(info['round']); s = S[info['id']][k]; s['steps'] += len(info['path']); s['alive'] += 1
            for t in info['path']:
                if t in pearls:
                    if t in st.beds: s['bed'] += 1
                    else: s['drop'] += 1
        elif kind == 'split':
            r, par, c, t, before, cl = info
            if par in (0, 1): k = ph(r); S[par][k]['splits'] += 1; S[par][k]['gave'] += cl
    out = {('us' if team.get(q, 'A' if q == 0 else 'B') == us else 'th'): S[q] for q in (0, 1)}
    o.write(json.dumps(dict(id=gid, map=x['map'], won=x['winner'] == us, q=out)) + '\n'); o.flush()
