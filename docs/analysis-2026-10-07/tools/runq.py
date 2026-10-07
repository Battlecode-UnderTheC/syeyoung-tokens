import json,os,sys
sys.path.insert(0,'.')
from qdeath import analyse
rows={r['id']:r for r in json.load(open('../data/rows.json'))}
out=[]
for f in sorted(os.listdir('../data/rep')):
    i=int(f.split('.')[0]); r=rows[i]
    q=r['queens'].get(r['us'])
    if not q or q['round']<200: continue
    th=r['queens'].get(r['th'])
    o=analyse('../data/rep/'+f,r['us'])
    if o is None: continue
    o.update(id=i,res=r['res'],opp=r['opp'],map=r['map'],their_q_dead_before= bool(th and th['round']<q['round']))
    out.append(o); print(json.dumps(o),flush=True)
json.dump(out,open('../data/qdeaths.json','w'))
