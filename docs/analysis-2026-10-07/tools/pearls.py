"""Pearls lying on the board at r100..r450 (every 50), and beds known. python3 pearls.py games.json out ids"""
import json, sys, os
sys.path.insert(0, os.path.dirname(__file__)); import decode; decode.FAST = True
G = {x['id']: x for x in json.load(open(sys.argv[1]))}
o = open(sys.argv[2], 'w')
for gid in map(int, sys.argv[3:]):
    p = os.path.join(os.path.dirname(sys.argv[2]) or '.', 'rep', f'{gid}.replay.gz')
    if not os.path.exists(p): continue
    d = decode.load(p); out = {}
    for kind, st, info in decode.walk(d):
        if kind == 'round_end' and info % 50 == 0 and info >= 50: out[info] = (len(st.pearls), len(st.beds))
    o.write(json.dumps(dict(id=gid, map=G[gid]['map'], out=out)) + '\n')
