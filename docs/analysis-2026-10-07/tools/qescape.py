"""At the queen's last turn before a head-on death: could she reach (within her move reach, through free tiles) a tile
that no visible enemy head (in her 7x7 window) could reach next turn (each enemy's reach, through free tiles, with her
new body ignored)? Also the same if she had been 1-step-only. THEIRS=1 for enemy queens.
python3 qescape.py games.json out ids"""
import json, sys, os
sys.path.insert(0, os.path.dirname(__file__)); import decode; decode.FAST = True
THEIRS = os.environ.get("THEIRS") == "1"
G = {x['id']: x for x in json.load(open(sys.argv[1]))}
o = open(sys.argv[2], 'w')
def reach(L): return (L + 3) // 4 + max(0, L - 2)
for gid in map(int, sys.argv[3:]):
    p = os.path.join(os.path.dirname(sys.argv[2]) or '.', 'rep', f'{gid}.replay.gz')
    if not os.path.exists(p): continue
    x = G[gid]; us = x['meta']['our_seat']; d = decode.load(p); g = decode.GameMap(d['map'])
    q = next(k for k, (t, _) in enumerate(g.dragons[:2]) if (t == us) != THEIRS)
    snap = None; mover = None
    for kind, st, info in decode.walk(d):
        if kind == 'pre':
            mover = info
            if info == q:
                occ = set(st.occupied()); qh = st.head(q); L = len(st.body[q])
                tail = st.body[q][-1]
                free_occ = occ - {qh}
                mine = g.bfs(qh, blocked=free_occ, limit=reach(L))
                one = g.bfs(qh, blocked=free_occ, limit=1)
                ens = [(st.head(j), len(st.body[j])) for j in st.alive if st.team[j] != st.team[q] and g.in_window(qh, st.head(j), 3)]
                danger = set()
                for h, l in ens:
                    danger |= set(g.bfs(h, blocked=occ - {h}, limit=reach(l)).keys())
                safe = [t for t in mine if t != qh and t not in danger]
                safe1 = [t for t in one if t != qh and t not in danger]
                snap = dict(L=L, ens=len(ens), moves=len(mine) - 1, safe=len(safe), safe1=len(safe1), r=st.round)
        elif kind == 'death' and info[1] == q:
            if info[3] == 'head_to_head' and snap and mover != q:
                snap.update(id=gid); o.write(json.dumps(snap) + '\n')
            break
