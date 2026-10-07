import json,os,sys,collections
sys.path.insert(0,'.')
import decode
from qdeath import tdist
rows={r['id']:r for r in json.load(open('../data/rows.json'))}
out=[]
for f in sorted(os.listdir('../data/rep')):
    i=int(f.split('.')[0]); r=rows[i]
    if r['rounds']<499: continue
    d=decode.load('../data/rep/'+f); g=decode.GameMap(d['map'])
    queens={t:k for k,(t,_) in enumerate(g.dragons[:2])}
    side={'us':r['us'],'th':r['th']}
    C={s:collections.Counter() for s in side}
    for kind,st,info in decode.walk(d):
        if kind=='death':
            rnd,k,team,reason,length,detail=info
            if rnd<360: continue
            s='us' if team==r['us'] else 'th'
            ph='360-429' if rnd<430 else '430-499'
            q=queens[team]
            if reason=='no_valid_action':
                C[s][ph+' resign n']+=1; C[s][ph+' resign len']+=length
                if q in st.alive and k!=q:
                    dd=tdist(g,st.head(k),st.head(q))[0]
                    if dd<=2: C[s][ph+' resign near queen len']+=length
                # near any longer ally head
                near=any(st.team[j]==team and j!=k and st.length(j)>length and tdist(g,st.head(k),st.head(j))[0]<=2 for j in st.alive)
                if near: C[s][ph+' resign near longer ally len']+=length
            else:
                C[s][ph+' other death n']+=1; C[s][ph+' other death len']+=length
        if kind=='turn' and info['round']>=430 and info['ate']:
            s='us' if info['team']==r['us'] else 'th'
            if info['id']==queens[info['team']]: C[s]['430+ queen ate']+=info['ate']
            C[s]['430+ all ate']+=info['ate']
    o=dict(id=i,res=r['res'],opp=r['opp'],reason=r['reason'],qus=r['queens'].get(r['us']),qth=r['queens'].get(r['th']),us=dict(C['us']),th=dict(C['th']))
    out.append(o); print(json.dumps(o),flush=True)
json.dump(out,open('../data/feed.json','w'))
