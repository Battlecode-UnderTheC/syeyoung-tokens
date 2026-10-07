"""Late feeding proximity: r380-499 deaths of each side; share of dead length whose head lies within Chebyshev 3 of
any segment of that side's current longest living dragon (or queen if longer). Also champion growth r380->r499."""
import json,sys,os,collections
sys.path.insert(0,os.path.dirname(__file__)); import decode; decode.FAST=True
G={x['id']:x for x in json.load(open(sys.argv[1]))}
o=open(sys.argv[2],'w'); n=int(sys.argv[3]); seen=collections.Counter()
for gid,x in G.items():
    if seen[x['map']]>=n: continue
    p=f'rep/{gid}.replay.gz'
    if not os.path.exists(p): continue
    seen[x['map']]+=1
    us=x['meta']['our_seat']; d=decode.load(p); g=decode.GameMap(d['map'])
    side=lambda t:'us' if t==us else 'th'
    def ch(a,b):
        dx=abs(a[0]-b[0]); dy=abs(a[1]-b[1]); return max(min(dx,g.W-dx),min(dy,g.H-dy))
    near={'us':0,'th':0}; tot={'us':0,'th':0}; lon380={}; lonend={}
    for kind,st,info in decode.walk(d):
        if kind=='round_end':
            if info==380:
                for s in ('us','th'): lon380[s]=max((len(st.body[i]) for i in st.alive if side(st.team[i])==s),default=0)
            lonend={s:max((len(st.body[i]) for i in st.alive if side(st.team[i])==s),default=0) for s in ('us','th')}
        elif kind=='death' and info[0]>=380:
            r,i,t,why,ln,det=info; s=side(t)
            if not st.body[i]: continue
            h=st.body[i][0]
            champs=[j for j in st.alive if j!=i and side(st.team[j])==s]
            if not champs: continue
            c=max(champs,key=lambda j:len(st.body[j]))
            tot[s]+=ln
            if any(ch(h,b)<=3 for b in st.body[c]): near[s]+=ln
    o.write(json.dumps(dict(id=gid,map=x['map'],won=x['winner']==us,near=near,tot=tot,lon380=lon380,lonend=lonend))+'\n')
