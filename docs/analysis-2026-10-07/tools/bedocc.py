"""Bed occupancy: every 5 rounds, for every known bed tile, which side's body covers it (blocking spawns).
Split by bed zone (own/mid/far relative to the occupier). Output per game: occupied bed-samples per side & zone,
plus total bed-samples."""
import json,sys,os,collections
sys.path.insert(0,os.path.dirname(__file__)); import decode; decode.FAST=True
G={x['id']:x for x in json.load(open(sys.argv[1]))}
ids=[int(a) for a in sys.argv[3:]] or list(G)
o=open(sys.argv[2],'a')
for gid in ids:
    x=G[gid]; p=f'rep/{gid}.replay.gz'
    if not os.path.exists(p): continue
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
    def zone(s,t):
        o2='th' if s=='us' else 'us'; a,b=D[s].get(t,999),D[o2].get(t,999)
        return 'own' if a<b-2 else 'far' if b<a-2 else 'mid'
    # learn bed set over whole game first
    beds=None
    C=collections.Counter(); samples=0; nb=0
    for kind,st,info in decode.walk(d):
        if kind=='round_end' and info%5==0 and info>=20:
            bset=st.beds
            if not bset: continue
            occ=st.occupied()
            nb+=len(bset); samples+=1
            for b in bset:
                j=occ.get(b)
                if j is not None:
                    s=side(st.team[j]); C[(s,zone(s,b))]+=1
    rec=dict(id=gid,won=x['winner']==us,map=x['map'],samples=samples,bedsamples=nb,occ={f'{a}_{b}':v for (a,b),v in C.items()})
    o.write(json.dumps(rec)+'\n'); o.flush()
