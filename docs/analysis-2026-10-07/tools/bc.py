"""Board control from replays: at r20/30/60/100, multi-source BFS (kelp walls, no body blocking) from each side's heads;
share of reachable tiles strictly closer to side A (ties split). Output bc for seat A."""
import json,sys,os,collections
sys.path.insert(0,os.path.dirname(__file__)); import decode; decode.FAST=True
G={x['id']:x for x in json.load(open(sys.argv[1]))}
o=open(sys.argv[2],'w')
for gid,x in G.items():
    p=os.path.join(os.path.dirname(sys.argv[2]) or '.','rep',f'{gid}.replay.gz')
    if not os.path.exists(p): continue
    d=decode.load(p); g=decode.GameMap(d['map']); out={}
    for kind,st,info in decode.walk(d):
        if kind=='round_end' and info in (20,30,60,100):
            dist={}
            for s in 'AB':
                D={}; fr=[]
                for i in st.alive:
                    if st.team[i]==s: h=st.head(i); D[h]=0; fr.append(h)
                dd=0
                while fr:
                    dd+=1; nx=[]
                    for t in fr:
                        for k in 'NESW':
                            n=g._step[(t,k)]
                            if n is not None and n not in D: D[n]=dd; nx.append(n)
                    fr=nx
                dist[s]=D
            tiles=set(dist['A'])|set(dist['B']); a=0.0
            for t in tiles:
                da=dist['A'].get(t,1e9); db=dist['B'].get(t,1e9)
                a+=1 if da<db else 0.5 if da==db else 0
            out[info]=a/len(tiles) if tiles else 0.5
        if kind=='round_end' and info>=100: break
    o.write(json.dumps(dict(id=gid,map=x['map'],winner=x['winner'],bcA=out))+'\n')
