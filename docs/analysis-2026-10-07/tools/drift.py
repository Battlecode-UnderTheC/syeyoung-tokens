import json,sys,os,collections
sys.path.insert(0,os.path.dirname(__file__)); import decode; decode.FAST=True
G={x['id']:x for x in json.load(open(sys.argv[1]))}
n=int(sys.argv[3]); seen=collections.Counter(); o=open(sys.argv[2],'w')
for gid,x in G.items():
    if seen[x['map']]>=n: continue
    p=f'rep/{gid}.replay.gz'
    if not os.path.exists(p): continue
    seen[x['map']]+=1
    us=x['meta']['our_seat']; d=decode.load(p); g=decode.GameMap(d['map'])
    side=lambda t:'us' if t==us else 'th'
    starts={s:[b[0] for t,b in g.dragons if side(t)==s] for s in ('us','th')}
    D={}
    for s in starts:
        D[s]={}
        for h in starts[s]:
            for t,v in g.bfs(h).items():
                if v<D[s].get(t,1<<30): D[s][t]=v
    C={s:collections.Counter() for s in ('us','th')}
    for kind,st,info in decode.walk(d):
        if kind=='turn' and info['round']<40:
            s=side(info['team']); a=info['action']
            if not a or a[0]!='move': C[s][a[0] if a else 'none']+=1; continue
            h0=info['head_before']; h1=info['path'][-1] if info['path'] else h0
            dd=D[s].get(h1,0)-D[s].get(h0,0)
            k='ate' if info['ate'] else ('out' if dd>0 else 'in' if dd<0 else 'level')
            C[s][k]+=1
        elif kind=='round_end' and info>=40: break
    o.write(json.dumps(dict(id=gid,map=x['map'],won=x['winner']==us,C=C))+'\n')
