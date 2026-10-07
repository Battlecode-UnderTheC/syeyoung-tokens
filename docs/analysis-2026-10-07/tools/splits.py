"""Split behaviour per side: parent length before split -> (parent kept, child) sizes, by phase; queen splits apart.
python3 splits.py games.json out ids"""
import json, sys, os, collections
sys.path.insert(0, os.path.dirname(__file__)); import decode; decode.FAST = True
G = {x['id']: x for x in json.load(open(sys.argv[1]))}
o = open(sys.argv[2], 'w')
for gid in map(int, sys.argv[3:]):
    p = os.path.join(os.path.dirname(sys.argv[2]) or '.', 'rep', f'{gid}.replay.gz')
    if not os.path.exists(p): continue
    x = G[gid]; us = x['meta']['our_seat']; d = decode.load(p); C = collections.Counter()
    for kind, st, info in decode.walk(d):
        if kind == 'split':
            r, par, c, t, before, cl = info
            s = 'us' if t == us else 'th'; ph = 'early' if r < 60 else 'mid' if r < 300 else 'late'
            who = 'queen' if par in (0, 1) else 'worker'
            C[(s, ph, who, f'{min(before,12)}:{min(cl,12)}')] += 1
    o.write(json.dumps(dict(id=gid, c=[[*k, v] for k, v in C.items()])) + '\n')
