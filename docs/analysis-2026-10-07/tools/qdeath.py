import sys,json,collections
sys.path.insert(0,'.')
import decode
def tdist(g,a,b):
    dx=abs(a[0]-b[0]);dy=abs(a[1]-b[1]); dx=min(dx,g.W-dx); dy=min(dy,g.H-dy); return dx+dy,max(dx,dy)
def analyse(path,us):
    d=decode.load(path); g=decode.GameMap(d['map'])
    th='B' if us=='A' else 'A'
    queens={t:i for i,(t,_) in enumerate(g.dragons[:2])}
    q=queens[us]
    hist=[]; pre=None; mover_pre=None
    for kind,st,info in decode.walk(d):
        if kind=='pre':
            if info==q:
                qh=st.head(q)
                pre=dict(round=st.round,head=qh,len=st.length(q),
                    enemies={i:(st.head(i),st.length(i)) for i in st.alive if st.team[i]==th},
                    allies4=sum(1 for i in st.alive if st.team[i]==us and i!=q and tdist(g,qh,st.head(i))[0]<=4))
            elif st.team[info]==th and q in st.alive:
                mover_pre=(info,st.head(info),st.length(info),st.head(q))
        if kind=='turn' and info['id']==q:
            pre['act']=info['action']; pre['after']=st.head(q) if q in st.alive else None; hist.append(pre)
        if kind=='death' and info[1]==q:
            rnd,i,team,reason,length,detail=info
            o=dict(round=rnd,reason=reason,detail=detail,length=length)
            last=hist[-1] if hist else None
            if 'enemy' in detail and mover_pre and last:
                k,kh,kl,qh_now=mover_pre
                o['killer_len']=kl
                o['killer_dist_now']=tdist(g,kh,qh_now)[0]
                o['queen_moved']=len(last['act'][1]) if last['act'] and last['act'][0]=='move' else 0
                if last['round']==rnd and k in last['enemies']:
                    kh0=last['enemies'][k][0]
                    o['killer_dist_at_qturn']=tdist(g,kh0,last['head'])
                    o['killer_visible_at_qturn']=tdist(g,kh0,last['head'])[1]<=3
                    # visible enemies at queen's turn start
                    vis=[(i,l,tdist(g,h,last['head'])) for i,(h,l) in last['enemies'].items() if tdist(g,h,last['head'])[1]<=3]
                    o['visible_enemies']=len(vis)
                o['allies4']=last['allies4']
            o['qlen_hist']=[h['len'] for h in hist[-10:]]
            o['qmoves_hist']=[ (h['act'][1] if h['act'] and h['act'][0]=='move' else h['act']) for h in hist[-6:]]
            return o
    return None
if __name__=='__main__':
    print(analyse(sys.argv[1],sys.argv[2]))
