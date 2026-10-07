"""r<60 territory: our eats by zone (own/mid/far by BFS from both sides' starting heads) per 10 rounds,
and our worker heads' zone shares at r20/30/40.  python3 spread.py rows rep out ids"""
import json,sys,os,collections
sys.path.insert(0,os.path.dirname(__file__))
import decode
decode.SKIP={2,5,6,7,8,12}
rows={r['id']:r for r in json.load(open(sys.argv[1]))}; repd=sys.argv[2]; out=open(sys.argv[3],'a'); ids=json.load(open(sys.argv[4]))
for i in ids:
    p=f'{repd}/{i}.replay.gz'
    if not os.path.exists(p): continue
    us=rows[i]['us']; d=decode.load(p); g=decode.GameMap(d['map'])
    du={}; dt={}
    for t,b in g.dragons:
        D=du if t==us else dt
        for k,v in g.bfs(b[0]).items():
            if v<D.get(k,1<<30): D[k]=v
    def zone(t):
        a,b=du.get(t,999),dt.get(t,999)
        return 'own' if a<b-2 else 'far' if b<a-2 else 'mid'
    C=collections.Counter()
    for kind,st,info in decode.walk(d):
        if kind=='turn' and info['round']<60 and info['ate']:
            s='us' if info['team']==us else 'th'
            C[f"{s}_ate_{zone(info['path'][-1])}"]+=info['ate']
        if kind=='round_end':
            if info in (20,30,40):
                for j in st.alive:
                    if j>1:
                        s='us' if st.team[j]==us else 'th'
                        C[f"{s}_heads{info}_{zone(st.head(j))}"]+=1
            if info>=60: break
    out.write(json.dumps(dict(id=i,C=C))+'\n'); out.flush()
