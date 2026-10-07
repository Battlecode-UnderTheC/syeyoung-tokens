"""Opening movement efficiency, r0-60. For each dragon alive at both r and r+10 (r=0,10,..,50): net torus-Manhattan
head displacement, and number of distinct head tiles visited in the window (out of 10 moves). Per side mean.
python3 straight.py games.json out.jsonl ids"""
import json,sys,os,statistics as S
sys.path.insert(0,os.path.dirname(__file__)); import decode; decode.FAST=True
G={x['id']:x for x in json.load(open(sys.argv[1]))}
o=open(sys.argv[2],'w')
for gid in map(int,sys.argv[3:]):
    p=os.path.join(os.path.dirname(sys.argv[2]) or '.','rep',f'{gid}.replay.gz')
    if not os.path.exists(p): continue
    x=G[gid]; us=x['meta']['our_seat']; d=decode.load(p); g=decode.GameMap(d['map'])
    def md(a,b):
        dx=abs(a[0]-b[0]); dy=abs(a[1]-b[1]); return min(dx,g.W-dx)+min(dy,g.H-dy)
    hist={}; team={}
    for kind,st,info in decode.walk(d):
        if kind!='round_end': continue
        for i in st.alive: hist.setdefault(i,{})[info]=st.head(i); team[i]=st.team[i]
        if info>=60: break
    out={}
    for s in 'AB':
        disp=[];uniq=[]
        for i,h in hist.items():
            if team[i]!=s: continue
            for r in range(0,60,10):
                if r in h and r+10 in h and all(k in h for k in range(r,r+11)):
                    disp.append(md(h[r],h[r+10])); uniq.append(len({h[k] for k in range(r,r+11)}))
        out[s]=dict(disp=S.mean(disp) if disp else 0, uniq=S.mean(uniq) if uniq else 0, n=len(disp))
    o.write(json.dumps(dict(id=gid,map=x['map'],us=us,won=x['winner']==us,out=out))+'\n')
