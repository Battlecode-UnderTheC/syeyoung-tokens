"""Non-queen deaths by length band and cause, per side, plus length lost. python3 workerdeath.py games.json out ids"""
import json, sys, os, collections
sys.path.insert(0, os.path.dirname(__file__)); import decode; decode.FAST = True
G = {x['id']: x for x in json.load(open(sys.argv[1]))}
o = open(sys.argv[2], 'w')
for gid in map(int, sys.argv[3:]):
    p = os.path.join(os.path.dirname(sys.argv[2]) or '.', 'rep', f'{gid}.replay.gz')
    if not os.path.exists(p): continue
    x = G[gid]; us = x['meta']['our_seat']; d = decode.load(p); C = collections.Counter()
    for kind, st, info in decode.walk(d):
        if kind == 'death':
            r, i, t, why, ln, det = info
            if i in (0, 1): continue
            s = 'us' if t == us else 'th'; lb = 'L5+' if ln >= 5 else 'L2-4'
            C[(s, lb, why + ':' + det)] += 1; C[(s, lb, 'n')] += 1; C[(s, lb, 'len')] += ln
    o.write(json.dumps(dict(id=gid, c=[[*k, v] for k, v in C.items()])) + '\n')
