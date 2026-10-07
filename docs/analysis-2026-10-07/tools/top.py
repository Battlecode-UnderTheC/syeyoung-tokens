import sys,collections; sys.path.insert(0,'.'); import decode
p,us=sys.argv[1],sys.argv[2]; th='B' if us=='A' else 'A'
d=decode.load(p); g=decode.GameMap(d['map'])
want={300,350,380,400,420,440,460,480,499}
deaths=collections.Counter()
for k,st,info in decode.walk(d):
    if k=='death' and info[0]>=380:
        rnd,i,team,reason,length,detail=info
        s='us' if team==us else 'th'
        deaths[(s,reason+('/'+detail if reason=='no_valid_action' else ''), 'len>=8' if length>=8 else 'len<8')]+=length
    if k=='round_end' and st.round in want:
        L={t:sorted((st.length(i) for i in st.alive if st.team[i]==t),reverse=True) for t in (us,th)}
        print(st.round,'us top5',L[us][:5],'n',len(L[us]),'tot',sum(L[us]),'| them top5',L[th][:5],'n',len(L[th]),'tot',sum(L[th]))
for k,v in sorted(deaths.items()): print(k,v)
