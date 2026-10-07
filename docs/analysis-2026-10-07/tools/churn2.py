"""Per side per phase: births (splits), deaths by reason/detail, length lost to deaths. python3 churn2.py games.json out ids"""
import json, sys, os, collections
sys.path.insert(0, os.path.dirname(__file__)); import decode; decode.FAST = True
G = {x['id']: x for x in json.load(open(sys.argv[1]))}
o = open(sys.argv[2], 'w')
ph = lambda r: 'r0-100' if r < 100 else 'r100-250' if r < 250 else 'r250-500'
for gid in map(int, sys.argv[3:]):
    p = os.path.join(os.path.dirname(sys.argv[2]) or '.', 'rep', f'{gid}.replay.gz')
    if not os.path.exists(p): continue
    x = G[gid]; us = x['meta']['our_seat']; d = decode.load(p); C = collections.Counter()
    for kind, st, info in decode.walk(d):
        if kind == 'split':
            r, par, c, t, b, cl = info; C[('us' if t == us else 'th', ph(r), 'birth')] += 1
        elif kind == 'death':
            r, i, t, why, ln, det = info; s = 'us' if t == us else 'th'
            C[(s, ph(r), 'death')] += 1; C[(s, ph(r), 'd:' + why + ':' + det)] += 1; C[(s, ph(r), 'lenlost')] += ln
    o.write(json.dumps(dict(id=gid, map=x['map'], won=x['winner'] == us, c=[[*k, v] for k, v in C.items()])) + '\n')
