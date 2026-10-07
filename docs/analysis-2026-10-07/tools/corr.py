"""Corridor use: tile is 'corridor' if >=2 of its 4 sides are kelp. Per game & side: share of head-samples on corridor
tiles (every 5 rounds, r0-100), pearls eaten on corridor tiles r0-100 vs total."""
import json,sys,os,collections
sys.path.insert(0,os.path.dirname(__file__)); import decode; decode.FAST=True
G={x['id']:x for x in json.load(open(sys.argv[1]))}
o=open(sys.argv[2],'w'); seen=collections.Counter()
for gid,x in G.items():
    if seen[x['map']]>=int(sys.argv[3]): continue
    p=f'rep/{gid}.replay.gz'
    if not os.path.exists(p): continue
    seen[x['map']]+=1
    us=x['meta']['our_seat']; d=decode.load(p); g=decode.GameMap(d['map'])
    corr={}
    for yy in range(g.H):
        for xx in range(g.W):
            k=sum(1 for dd in 'NESW' if g.step((xx,yy),dd) is None)
            corr[(xx,yy)]=k>=2
    side=lambda t:'us' if t==us else 'th'
    H={s:[0,0] for s in ('us','th')}; E={s:[0,0] for s in ('us','th')}
    for kind,st,info in decode.walk(d):
        if kind=='turn' and info['round']<100 and info['ate']:
            hd=info['path'][-1] if info['path'] else info['head_before']
            s=side(info['team']); E[s][1]+=info['ate']
            if corr.get(hd): E[s][0]+=info['ate']
        elif kind=='round_end':
            if info>=100: break
            if info%5==0:
                for i in st.alive:
                    s=side(st.team[i]); H[s][1]+=1
                    if corr.get(st.head(i)): H[s][0]+=1
    ctiles=sum(corr.values())/len(corr)
    o.write(json.dumps(dict(id=gid,map=x['map'],won=x['winner']==us,ctiles=ctiles,H=H,E=E))+'\n')
