"""Starter heading r0->r10 and r0->r20: for each side, mean BFS-distance change toward (a) the enemy spawn heads,
(b) away from own spawn. Per map, us vs them."""
import json,sys,os,collections,statistics as S
sys.path.insert(0,os.path.dirname(__file__)); import decode; decode.FAST=True
G={x['id']:x for x in json.load(open(sys.argv[1]))}
out=collections.defaultdict(list)
seen=collections.Counter()
for gid,x in G.items():
    if seen[x['map']]>=12: continue
    p=f'rep/{gid}.replay.gz'
    if not os.path.exists(p): continue
    seen[x['map']]+=1
    us=x['meta']['our_seat']
    d=decode.load(p); g=decode.GameMap(d['map'])
    side=lambda t:'us' if t==us else 'th'
    starts={s:[b[0] for t,b in g.dragons if side(t)==s] for s in ('us','th')}
    D={}
    for s in starts:
        D[s]={}
        for h in starts[s]:
            for t,v in g.bfs(h).items():
                if v<D[s].get(t,1<<30): D[s][t]=v
    snap={}
    for kind,st,info in decode.walk(d):
        if kind=='round_end' and info in (10,20):
            for s in ('us','th'):
                o='th' if s=='us' else 'us'
                hs=[st.head(i) for i in st.alive if side(st.team[i])==s]
                snap[(s,info)]=(S.mean(D[s].get(h,0) for h in hs), S.mean(D[o].get(h,0) for h in hs))
        if kind=='round_end' and info>=20: break
    for s in ('us','th'):
        out[(x['map'],s)].append(snap.get((s,10),(0,0))+snap.get((s,20),(0,0)))
maps=sorted(set(m for m,_ in out))
print('map: for us / them -> r10 (dist from own spawn, dist to enemy spawn) r20 (same)')
for m in maps:
    def f(s):
        v=out[(m,s)]; return ' '.join(f'{S.mean(a[k] for a in v):5.1f}' for k in range(4))
    print(f'{m:18s} us: {f("us")}   th: {f("th")}')
