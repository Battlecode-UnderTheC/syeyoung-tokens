"""Track both queens every 10 rounds: pos, len, dist from own spawn, own heads within 3, enemy heads within 5,
nearest enemy head distance, free tiles reachable within 6 steps (room), whether any enemy head within Chebyshev 3.
python3 qtrack.py games.json out.jsonl [ids]
"""
import json, sys, os, collections
sys.path.insert(0, os.path.dirname(__file__))
import decode
decode.FAST = True


def run(p, us):
    d = decode.load(p); g = decode.GameMap(d['map'])
    side = lambda t: 'us' if t == us else 'th'
    queens = {side(t): k for k, (t, _) in enumerate(g.dragons[:2])}
    starts = {s: [b[0] for t, b in g.dragons if side(t) == s] for s in ('us', 'th')}
    D = {}
    for s in starts:
        D[s] = {}
        for h in starts[s]:
            for t, v in g.bfs(h).items():
                if v < D[s].get(t, 1 << 30): D[s][t] = v
    def ch(a, b):
        dx = abs(a[0] - b[0]); dy = abs(a[1] - b[1]); return max(min(dx, g.W - dx), min(dy, g.H - dy))
    tr = {'us': [], 'th': []}
    mindist = {'us': 99, 'th': 99}
    for kind, st, info in decode.walk(d):
        if kind != 'round_end': continue
        r = info
        occ = None
        for s in ('us', 'th'):
            q = queens[s]
            if q not in st.alive: continue
            qh = st.head(q)
            en = [st.head(j) for j in st.alive if side(st.team[j]) != s]
            nd = min((ch(qh, h) for h in en), default=99)
            mindist[s] = min(mindist[s], nd)
            if r % 10 == 0:
                if occ is None: occ = set(st.occupied())
                room = len(g.bfs(qh, blocked=occ - {qh}, limit=6)) if hasattr(g, 'bfs') else -1
                tr[s].append(dict(r=r, x=qh[0], y=qh[1], L=len(st.body[q]), dist=D[s].get(qh, -1),
                                  own3=sum(1 for j in st.alive if j != q and side(st.team[j]) == s and ch(st.head(j), qh) <= 3),
                                  en5=sum(1 for h in en if ch(qh, h) <= 5), nd=nd, room=room))
    return dict(tr=tr, mindist=mindist, W=g.W, H=g.H)


def main():
    G = {x['id']: x for x in json.load(open(sys.argv[1]))}
    ids = [int(a) for a in sys.argv[3:]] or list(G)
    o = open(sys.argv[2], 'a')
    for gid in ids:
        x = G[gid]
        p = os.path.join(os.path.dirname(sys.argv[2]) or '.', 'rep', f'{gid}.replay.gz')
        if not os.path.exists(p): continue
        us = x['meta']['our_seat']
        try:
            r = run(p, us)
        except Exception as e:
            print(gid, e, file=sys.stderr); continue
        r.update(id=gid, map=x['map'], won=x['winner'] == us, reason=x['reason'], opp=x['meta']['opponent'])
        o.write(json.dumps(r) + '\n'); o.flush()


if __name__ == '__main__':
    main()
