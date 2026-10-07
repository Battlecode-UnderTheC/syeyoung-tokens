import sys,decode
p,us,rnd=sys.argv[1],sys.argv[2],int(sys.argv[3])
d=decode.load(p); g=decode.GameMap(d['map'])
q=[k for k,(t,_) in enumerate(g.dragons[:2]) if t==us][0]
last=None
for k,st,info in decode.walk(d):
    if k=='death' and info[1]==q:
        print('death',info,'head',st.head(q)); break
    if k=='turn' and info['id']==q: last=(info['round'],info['action'],st.head(q) if q in st.alive else None)
print('last',last,'queen id',q)
