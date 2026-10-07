"""Where uneaten pearls sit (r100-400 every 50): tile kelp-side count (0..4), and per side the eats r100-400 by tile
kelp-side count, plus dragon-turns. python3 pearlwhere.py games.json out ids"""
import json, sys, os, collections
sys.path.insert(0, os.path.dirname(__file__)); import decode; decode.FAST = True
G = {x['id']: x for x in json.load(open(sys.argv[1]))}
o = open(sys.argv[2], 'w')
for gid in map(int, sys.argv[3:]):
    p = os.path.join(os.path.dirname(sys.argv[2]) or '.', 'rep', f'{gid}.replay.gz')
    if not os.path.exists(p): continue
    x = G[gid]; us = x['meta']['our_seat']; d = decode.load(p); g = decode.GameMap(d['map'])
    kel = {}
    for yy in range(g.H):
        for xx in range(g.W):
            kel[(xx, yy)] = sum(1 for dd in 'NESW' if g._step[((xx, yy), dd)] is None)
    lying = collections.Counter(); eat = collections.Counter(); turns = collections.Counter(); pearls = set()
    for kind, st, info in decode.walk(d):
        if kind == 'round_end' and info in (100, 150, 200, 250, 300, 350, 400):
            for t in st.pearls: lying[kel[t]] += 1
        elif kind == 'pre': pearls = set(st.pearls)
        elif kind == 'turn' and 100 <= info['round'] < 400:
            s = 'us' if info['team'] == us else 'th'; turns[s] += 1
            for t in info['path']:
                if t in pearls: eat[(s, kel[t])] += 1
    o.write(json.dumps(dict(id=gid, map=x['map'], won=x['winner'] == us, lying=lying, eat=[[a, b, v] for (a, b), v in eat.items()], turns=turns)) + '\n')
