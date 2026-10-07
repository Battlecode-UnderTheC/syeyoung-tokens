"""Per side: pearls eaten inside small portal-only rooms (<=8 tiles via non-portal steps), outside them, deaths inside
rooms (by reason), queen eats in rooms, per phase. python3 roomeat.py games.json out.jsonl ids"""
import json, sys, os, collections
sys.path.insert(0, os.path.dirname(__file__)); import decode; decode.FAST = True
G = {x['id']: x for x in json.load(open(sys.argv[1]))}
o = open(sys.argv[2], 'w')
def rooms(g, N=8):
    def adj(a, b):
        dx = (a[0] - b[0]) % g.W; dy = (a[1] - b[1]) % g.H
        return (min(dx, g.W - dx) + min(dy, g.H - dy)) == 1
    room = {}
    for y in range(g.H):
        for x in range(g.W):
            t = (x, y)
            if t in room: continue
            seen = {t}; fr = [t]
            while fr and len(seen) <= 400:
                nx = []
                for a in fr:
                    for dd in 'NESW':
                        b = g._step[(a, dd)]
                        if b is not None and adj(a, b) and b not in seen: seen.add(b); nx.append(b)
                fr = nx
            for s in seen: room[s] = len(seen)
    return {t for t, n in room.items() if n <= N}
for gid in map(int, sys.argv[3:]):
    p = os.path.join(os.path.dirname(sys.argv[2]) or '.', 'rep', f'{gid}.replay.gz')
    if not os.path.exists(p): continue
    x = G[gid]; us = x['meta']['our_seat']; d = decode.load(p); g = decode.GameMap(d['map']); small = rooms(g)
    if not small: continue
    C = collections.Counter(); pearls = set()
    for kind, st, info in decode.walk(d):
        if kind == 'pre': pearls = set(st.pearls)
        elif kind == 'turn':
            s = 'us' if info['team'] == us else 'th'; ph = 'early' if info['round'] < 150 else 'mid' if info['round'] < 350 else 'late'
            for t in info['path']:
                if t in pearls:
                    C[(s, ph, 'room' if t in small else 'open')] += 1
                    if info['id'] in (0, 1) and t in small: C[(s, ph, 'queen_room')] += 1
        elif kind == 'death':
            r, i, t, why, ln, det = info
            if st.body[i] and st.body[i][0] in small:
                C[('us' if t == us else 'th', 'early' if r < 150 else 'mid' if r < 350 else 'late', 'die_in_room')] += 1
    o.write(json.dumps(dict(id=gid, map=x['map'], won=x['winner'] == us, small=len(small), c=[[*k, v] for k, v in C.items()])) + '\n'); o.flush()
