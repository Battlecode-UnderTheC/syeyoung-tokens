"""Queen-kill opportunities: at the start of each non-queen dragon's turn, can it reach the ENEMY queen's head this
turn? Reach = ceil(L/4) free + paid steps while length > 2  ->  R = ceil(L/4) + max(0, L-2).
Path = BFS over tiles not occupied by any body (the queen's head tile is the target), within R steps.
Counts per side: opportunities (dragon-turns), distinct rounds with any opportunity, and conversions (the enemy queen
died head-to-head on that dragon's turn). Also records the first opportunity round and whether the queen survived.
python3 qopp.py games.json out.jsonl [ids]
"""
import json, sys, os, collections
sys.path.insert(0, os.path.dirname(__file__))
import decode
decode.FAST = True


def reach(L):
    return (L + 3) // 4 + max(0, L - 2)


def run(p, us):
    d = decode.load(p); g = decode.GameMap(d['map'])
    side = lambda t: 'us' if t == us else 'th'
    queens = {}
    for k, (t, _) in enumerate(g.dragons[:2]):
        queens[side(t)] = k
    def ch(a, b):
        dx = abs(a[0] - b[0]); dy = abs(a[1] - b[1]); return max(min(dx, g.W - dx), min(dy, g.H - dy))
    S = {s: dict(opp=0, rounds=set(), conv=0, first=None, qlen_at_opp=[], adj=0) for s in ('us', 'th')}
    cur = None; cur_opp = None
    for kind, st, info in decode.walk(d):
        if kind == 'pre':
            i = info; cur = i; cur_opp = None
            s = side(st.team[i])
            o = 'th' if s == 'us' else 'us'
            q = queens.get(o)
            if q is None or q not in st.alive or i in queens.values():
                continue
            h = st.head(i); qh = st.head(q); L = len(st.body[i])
            R = reach(L)
            if ch(h, qh) > R:
                continue
            occ = set(st.occupied()); occ.discard(qh)
            dist = g.bfs(h, blocked=occ, limit=R)
            if qh in dist:
                S[s]['opp'] += 1; S[s]['rounds'].add(st.round)
                if dist[qh] == 1: S[s]['adj'] += 1
                if S[s]['first'] is None: S[s]['first'] = st.round
                S[s]['qlen_at_opp'].append(len(st.body[q]))
                cur_opp = (s, q)
        elif kind == 'death':
            r, i, t, why, ln, det = info
            if cur_opp and i == cur_opp[1] and why == 'head_to_head':
                S[cur_opp[0]]['conv'] += 1
    out = {}
    for s in S:
        x = S[s]
        out[s] = dict(opp=x['opp'], rounds=len(x['rounds']), conv=x['conv'], first=x['first'], adj=x['adj'],
                      qlen_med=(sorted(x['qlen_at_opp'])[len(x['qlen_at_opp']) // 2] if x['qlen_at_opp'] else None))
    return out


def main():
    G = {x['id']: x for x in json.load(open(sys.argv[1]))}
    ids = [int(a) for a in sys.argv[3:]] or list(G)
    o = open(sys.argv[2], 'a')
    for gid in ids:
        x = G[gid]
        p = os.path.join(os.path.dirname(sys.argv[2]) or '.', 'rep', f'{gid}.replay.gz')
        if not os.path.exists(p): continue
        try:
            r = run(p, x['meta']['our_seat'])
        except Exception as e:
            print(gid, e, file=sys.stderr); continue
        r.update(id=gid, map=x['map'], won=x['winner'] == x['meta']['our_seat'], opp=x['meta']['opponent'])
        o.write(json.dumps(r) + '\n'); o.flush()


if __name__ == '__main__':
    main()
