import json,sys,os,collections
sys.path.insert(0,os.path.dirname(__file__)); import decode; decode.FAST=True
G={x['id']:x for x in json.load(open('real_ring-sprint-1.json'))}
seen=set(); out={}
for gid,x in G.items():
    m=x['map']
    if m in seen: continue
    p=f'rep/{gid}.replay.gz'
    if not os.path.exists(p): continue
    seen.add(m)
    d=decode.load(p); g=decode.GameMap(d['map'])
    us=x['meta']['our_seat']
    starts={s:[b[0] for t,b in g.dragons if (t==us)==(s=='us')] for s in ('us','th')}
    D={}
    for s in starts:
        D[s]={}
        for h in starts[s]:
            for t,v in g.bfs(h).items():
                if v<D[s].get(t,1<<30): D[s][t]=v
    spawns=collections.Counter()
    for kind,st,info in decode.walk(d):
        if kind=='round_end' and info>=200: break
    beds=st.beds
    z=collections.Counter(); dist=collections.Counter()
    for t in beds:
        a,b=D['us'].get(t,999),D['th'].get(t,999)
        z['own' if a<b-2 else 'far' if b<a-2 else 'mid']+=1
    out[m]=dict(beds=len(beds),**z,W=g.W,H=g.H,starters=len(starts['us']),portals=g.portal_count)
    print(f"{m:18s} {g.W}x{g.H} st={len(starts['us'])} portals={g.portal_count} beds={len(beds)} own={z['own']} mid={z['mid']} far={z['far']}")
json.dump(out,open('beds.json','w'))
