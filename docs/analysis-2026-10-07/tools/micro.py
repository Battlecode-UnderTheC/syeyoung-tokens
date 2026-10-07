import json,sys,os,collections
sys.path.insert(0,os.path.dirname(__file__)); import decode; decode.FAST=True
G={x['id']:x for x in json.load(open(sys.argv[1]))}
o=open(sys.argv[2],'w')
for gid,x in G.items():
    p=f'rep/{gid}.replay.gz'
    if not os.path.exists(p): continue
    us=x['meta']['our_seat']; d=decode.load(p)
    side=lambda t:'us' if t==us else 'th'
    first_eat={}; first_split={}; eat10=collections.Counter(); eat20=collections.Counter(); n={}
    for kind,st,info in decode.walk(d):
        if kind=='turn':
            s=side(info['team'])
            if info['ate']:
                first_eat.setdefault(s,info['round'])
                if info['round']<10: eat10[s]+=info['ate']
                if info['round']<20: eat20[s]+=info['ate']
        elif kind=='split':
            s=side(info[3])
            if info[0]>0: first_split.setdefault(s,info[0])
        elif kind=='round_end':
            if info in (10,20): n[info]={s:sum(1 for i in st.alive if side(st.team[i])==s) for s in ('us','th')}
            if info>=20: break
    o.write(json.dumps(dict(id=gid,map=x['map'],won=x['winner']==us,fe=first_eat,fs=first_split,e10=eat10,e20=eat20,n=n))+'\n')
