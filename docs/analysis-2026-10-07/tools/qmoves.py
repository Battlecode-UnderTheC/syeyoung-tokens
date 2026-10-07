import json,os,sys,collections
sys.path.insert(0,'.')
import decode
from qdeath import tdist
rows={r['id']:r for r in json.load(open('../data/rows.json'))}
files=sorted(os.listdir('../data/rep'))
part=int(sys.argv[1]); files=files[part::2]
res=[]
for f in files:
    i=int(f.split('.')[0]); r=rows[i]; us=r['us']; th=r['th']
    d=decode.load('../data/rep/'+f); g=decode.GameMap(d['map'])
    q={t:k for k,(t,_) in enumerate(g.dragons[:2])}[us]
    turns=[]; pre=None
    for kind,st,info in decode.walk(d):
        if kind=='pre' and info==q and st.round>=300:
            qh=st.head(q)
            occ=st.occupied()
            own=en=0
            for t in g.__class__.__dict__ and []: pass
            for dx in range(-3,4):
                for dy in range(-3,4):
                    t=((qh[0]+dx)%g.W,(qh[1]+dy)%g.H)
                    o=occ.get(t)
                    if o is None or o==q: continue
                    if st.team[o]==us: own+=1
                    else: en+=1
            eh=min([tdist(g,qh,st.head(j))[0] for j in st.alive if st.team[j]==th] or [99])
            pre=dict(r=st.round,len=st.length(q),own=own,en=en,eh=eh)
        if kind=='turn' and info['id']==q and pre:
            a=info['action']; pre['steps']=len(a[1]) if a and a[0]=='move' else 0
            pre['died_turn']=False; turns.append(pre); pre=None
        if kind=='death' and info[1]==q:
            if turns: turns[-1]['died_next']=info[0]
            break
    res.append(dict(id=i,res=r['res'],turns=turns))
json.dump(res,open(f'../data/qmoves{part}.json','w'))
