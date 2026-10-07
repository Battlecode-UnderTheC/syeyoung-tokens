"""For every queen death (both sides): context at the moment of death.
python3 qd2.py games.json out.jsonl
"""
import json, sys, os, collections
sys.path.insert(0, os.path.dirname(__file__))
import decode
decode.FAST = True


def run(p, us):
    d = decode.load(p); g = decode.GameMap(d['map'])
    side = lambda t: 'us' if t == us else 'th'
    queens = {k: side(t) for k, (t, _) in enumerate(g.dragons[:2])}
    starts = {s: [b[0] for t, b in g.dragons if side(t) == s] for s in ('us', 'th')}
    D = {}
    for s in starts:
        D[s] = {}
        for h in starts[s]:
            for t, v in g.bfs(h).items():
                if v < D[s].get(t, 1 << 30): D[s][t] = v
    def ch(a, b):
        dx = abs(a[0] - b[0]); dy = abs(a[1] - b[1]); return max(min(dx, g.W - dx), min(dy, g.H - dy))
    cur = None; prelen = {}; prehead = {}
    qacts = collections.defaultdict(lambda: collections.deque(maxlen=6))
    qmax = collections.Counter()
    born = {}
    out = []
    for kind, st, info in decode.walk(d):
        if kind == 'pre':
            cur = info; prelen[cur] = len(st.body[cur]); prehead[cur] = st.head(cur)
        elif kind == 'split':
            born[info[2]] = info[0]
        elif kind == 'turn':
            if info['id'] in queens:
                a = info['action']
                qacts[info['id']].append(('m%d' % len(info['path'])) if a and a[0] == 'move' else (a[0] if a else 'none'))
                qmax[info['id']] = max(qmax[info['id']], info.get('len_after', 0) or 0)
        elif kind == 'death':
            r, i, t, why, ln, det = info
            if i not in queens: continue
            s = queens[i]
            qh = st.body[i][0]
            killer = cur if (why == 'head_to_head' and cur != i) else None
            rec = dict(r=r, side=s, qlen=ln, qmax=qmax[i], why=why + (':' + det if det else ''),
                       qdist=D[s].get(qh, -1),
                       allies3=sum(1 for j in st.alive if j != i and st.team[j] == st.team[i] and ch(st.head(j), qh) <= 3),
                       enemies5=sum(1 for j in st.alive if st.team[j] != st.team[i] and ch(st.head(j), qh) <= 5),
                       qacts=list(qacts[i]), qhead=qh)
            if killer is not None:
                rec.update(killer_len=prelen.get(killer), killer_age=r - born.get(killer, -1),
                           killer_from=ch(prehead.get(killer, qh), qh))
            out.append(rec)
    return out


def main():
    G = {x['id']: x for x in json.load(open(sys.argv[1]))}
    o = open(sys.argv[2], 'w')
    for gid, x in G.items():
        p = os.path.join(os.path.dirname(sys.argv[2]) or '.', 'rep', f'{gid}.replay.gz')
        if not os.path.exists(p): continue
        us = x['meta']['our_seat']
        try:
            recs = run(p, us)
        except Exception as e:
            print(gid, e, file=sys.stderr); continue
        o.write(json.dumps(dict(id=gid, map=x['map'], won=x['winner'] == us, opp=x['meta']['opponent'], q=recs)) + '\n')
        o.flush()


if __name__ == '__main__':
    main()
