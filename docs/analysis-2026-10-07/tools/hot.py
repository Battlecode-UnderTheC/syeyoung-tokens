"""Per game: the densest bed regions (bed count within Chebyshev radius 3, top tiles covering 40% of beds by r60).
First round each side has a head in the hot region, heads inside at r10..r60, eats inside by r60."""
import json,sys,os,collections
sys.path.insert(0,os.path.dirname(__file__)); import decode; decode.FAST=True
G={x['id']:x for x in json.load(open(sys.argv[1]))}
out=open(sys.argv[2],'a')
done={json.loads(l)['id'] for l in open(sys.argv[2])} if os.path.getsize(sys.argv[2]) else set()
for gid,x in G.items():
    if gid in done: continue
    p=f'rep/{gid}.replay.gz'
    if not os.path.exists(p): continue
    try:
        d=decode.load(p)
    except Exception as e:
        continue
    g=decode.GameMap(d['map']); us=x['meta']['our_seat']
    # pass 1: beds by r60
    for kind,st,info in decode.walk(d):
        if kind=='round_end' and info>=60: break
    beds=set(st.beds)
    def ch(a,b):
        dx=abs(a[0]-b[0]); dy=abs(a[1]-b[1]); return max(min(dx,g.W-dx),min(dy,g.H-dy))
    dens={t:sum(1 for b in beds if ch(t,b)<=3) for t in beds}
    # hot region: tiles within 2 of beds whose density >= 60% of max
    mx=max(dens.values()) if dens else 0
    core=[t for t,v in dens.items() if v>=0.6*mx]
    hot=set()
    for c in core:
        for dx in range(-2,3):
            for dy in range(-2,3):
                hot.add(((c[0]+dx)%g.W,(c[1]+dy)%g.H))
    first={'us':None,'th':None}; inside={}; eat={'us':0,'th':0}
    side=lambda t:'us' if t==us else 'th'
    for kind,st,info in decode.walk(d):
        if kind=='turn' and info['round']<60 and info['ate']:
            hd=info['path'][-1] if info['path'] else info['head_before']
            if hd in hot: eat[side(info['team'])]+=info['ate']
        if kind=='round_end':
            r=info
            for i in st.alive:
                s=side(st.team[i])
                if first[s] is None and st.head(i) in hot: first[s]=r
            if r in (10,20,30,40,60):
                c=collections.Counter(side(st.team[i]) for i in st.alive if st.head(i) in hot)
                inside[r]=[c['us'],c['th']]
            if r>=60: break
    rec=dict(id=gid,map=x['map'],won=x['winner']==us,beds=len(beds),hotbeds=sum(1 for b in beds if b in hot),
             first=first,inside=inside,eat=eat)
    out.write(json.dumps(rec)+'\n'); out.flush()
