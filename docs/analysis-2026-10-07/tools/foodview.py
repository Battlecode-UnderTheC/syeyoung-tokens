"""r0-60: for each dragon-turn with >=1 pearl within its 7x7 window: did it eat, or step closer (BFS, bodies as blocks)
to the nearest visible pearl? Per side."""
import json,sys,os,collections
sys.path.insert(0,os.path.dirname(__file__)); import decode; decode.FAST=True
G={x['id']:x for x in json.load(open(sys.argv[1]))}
ids=[int(a) for a in sys.argv[3:]]
o=open(sys.argv[2],'w')
for gid in ids:
    x=G[gid]; p=f'rep/{gid}.replay.gz'
    if not os.path.exists(p): continue
    us=x['meta']['our_seat']; d=decode.load(p); g=decode.GameMap(d['map'])
    side=lambda t:'us' if t==us else 'th'
    def ch(a,b):
        dx=abs(a[0]-b[0]); dy=abs(a[1]-b[1]); return max(min(dx,g.W-dx),min(dy,g.H-dy))
    C={s:collections.Counter() for s in ('us','th')}
    pre=None
    for kind,st,info in decode.walk(d):
        if kind=='pre':
            if st.round>=60: pre=None; continue
            i=info; h=st.head(i); vis=[pp for pp in st.pearls if ch(pp,h)<=3]
            C[side(st.team[i])]['_turns']+=1
            if not vis: pre=None; continue
            occ=set(st.occupied())
            dist=g.bfs(h,blocked=occ,limit=8)
            reach=[(dist[pp],pp) for pp in vis if pp in dist]
            if not reach: pre=(i,side(st.team[i]),None,None); continue
            dm,tgt=min(reach)
            pre=(i,side(st.team[i]),tgt,dm)
        elif kind=='turn' and pre and info['id']==pre[0]:
            s=pre[1]
            if pre[2] is None: C[s]['visible_unreachable']+=1
            elif info['ate']: C[s]['ate']+=1
            else:
                h1=info['path'][-1] if info['path'] else info['head_before']
                occ=set(st.occupied()); dist=g.bfs(h1,blocked=occ-{h1},limit=10)
                nd=dist.get(pre[2],99)
                C[s]['closer' if nd<pre[3] else 'not_closer']+=1
            pre=None
        elif kind=='round_end' and info>=60: break
    o.write(json.dumps(dict(id=gid,map=x['map'],won=x['winner']==us,C=C))+'\n')
