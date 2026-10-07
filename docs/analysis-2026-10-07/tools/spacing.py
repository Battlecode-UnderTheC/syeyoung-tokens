import json,sys,os,collections,statistics as S
sys.path.insert(0,os.path.dirname(__file__)); import decode; decode.FAST=True
G={x['id']:x for x in json.load(open(sys.argv[1]))}
ids=[int(a) for a in sys.argv[3:]] or list(G)
o=open(sys.argv[2],'w')
for gid in ids:
    x=G[gid]; p=os.path.join(os.path.dirname(sys.argv[2]) or '.','rep',f'{gid}.replay.gz')
    if not os.path.exists(p): continue
    d=decode.load(p); g=decode.GameMap(d['map'])
    def ch(a,b):
        dx=abs(a[0]-b[0]); dy=abs(a[1]-b[1]); return max(min(dx,g.W-dx),min(dy,g.H-dy))
    out={}
    for kind,st,info in decode.walk(d):
        if kind=='round_end' and info in (30,60,100):
            for s in 'AB':
                hs=[st.head(i) for i in st.alive if st.team[i]==s]
                if len(hs)<2: continue
                nn=[min(ch(h,k) for k in hs if k is not h) for h in hs]
                out[f'{s}{info}']=(S.mean(nn), sum(1 for v in nn if v<=2)/len(nn))
        if kind=='round_end' and info>=100: break
    o.write(json.dumps(dict(id=gid,out=out))+'\n')
