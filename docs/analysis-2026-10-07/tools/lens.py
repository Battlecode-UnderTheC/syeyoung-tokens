"""r<60: our dragon-length histogram at r20/30/40/50 and 'held' worker-turns (non-queen, len>=4, moved, no split)
python3 lens.py rows rep out ids"""
import json,sys,os,collections
sys.path.insert(0,os.path.dirname(__file__))
import decode
decode.SKIP={2,5,6,7,8,12}
rows={r['id']:r for r in json.load(open(sys.argv[1]))}; repd=sys.argv[2]; out=open(sys.argv[3],'a'); ids=json.load(open(sys.argv[4]))
for i in ids:
    p=f'{repd}/{i}.replay.gz'
    if not os.path.exists(p): continue
    us=rows[i]['us']; d=decode.load(p)
    H={}; C=collections.Counter()
    for kind,st,info in decode.walk(d):
        if kind=='turn' and info['round']<60 and info['team']==us and info['id']>1:
            a=info['action']; L=info['len_before']
            if a and a[0]=='move' and L>=4: C[f'held{min(L,6)}']+=1
            if a and a[0]=='split': C[f'split{min(L,6)}']+=1
        if kind=='round_end':
            if info in (20,30,40,50):
                H[info]=collections.Counter(min(st.length(j),6) for j in st.alive if st.team[j]==us and j>1)
            if info>=60: break
    out.write(json.dumps(dict(id=i,H={k:dict(v) for k,v in H.items()},C=C))+'\n'); out.flush()
