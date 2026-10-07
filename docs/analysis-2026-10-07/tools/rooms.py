"""Small rooms: tiles whose region, walking only through non-portal steps, has <= N tiles (pockets reachable only by
portal or closed). For a replay: share of pearls on board lying in such rooms at r100..400, and room count.
python3 rooms.py replay.gz [N]"""
import sys, os
sys.path.insert(0, os.path.dirname(__file__)); import decode; decode.FAST = True
d = decode.load(sys.argv[1]); g = decode.GameMap(d['map']); N = int(sys.argv[2]) if len(sys.argv) > 2 else 8
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
small = {t for t, n in room.items() if n <= N}
print(g.name, 'tiles in rooms <=', N, ':', len(small))
for kind, st, info in decode.walk(d):
    if kind == 'round_end' and info in (100, 200, 300, 400):
        p = st.pearls; print(' r', info, 'pearls', len(p), 'in small rooms', sum(1 for t in p if t in small), 'beds in rooms', sum(1 for t in st.beds if t in small), 'of', len(st.beds))
