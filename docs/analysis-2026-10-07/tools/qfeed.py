"""Where the drops a queen eats come from: at each death record the drop tiles (every 2nd segment from the head) with
the dead dragon's side relative to the queen-eater, reason/detail, round. When a queen eats a non-bed pearl, look the
tile up. python3 qfeed.py games.json out.jsonl ids"""
import json, sys, os, collections
sys.path.insert(0, os.path.dirname(__file__)); import decode; decode.FAST = True
G = {x['id']: x for x in json.load(open(sys.argv[1]))}
o = open(sys.argv[2], 'w')
for gid in map(int, sys.argv[3:]):
    p = os.path.join(os.path.dirname(sys.argv[2]) or '.', 'rep', f'{gid}.replay.gz')
    if not os.path.exists(p): continue
    x = G[gid]; us = x['meta']['our_seat']; d = decode.load(p)
    src = {}; C = collections.Counter(); pearls = set()
    for kind, st, info in decode.walk(d):
        if kind == 'death':
            r, i, t, why, ln, det = info
            for k, tile in enumerate(st.body[i]):
                if k % 2 == 0: src[tile] = (t, why + ':' + det, r)
        elif kind == 'pre' and info in (0, 1):
            pearls = set(st.pearls)
        elif kind == 'turn' and info['id'] in (0, 1) and info['round'] >= 200:
            qt = info['team']
            for tile in info['path']:
                if tile in pearls and tile not in st.beds:
                    s = src.get(tile)
                    side = 'us' if qt == us else 'th'
                    if s is None: C[(side, '?')] += 1
                    else: C[(side, ('own' if s[0] == qt else 'enemy') + ' ' + s[1], )] += 1
    o.write(json.dumps(dict(id=gid, c=[[k[0], k[1], v] for k, v in C.items()])) + '\n')
