import json,sys,os,collections
sys.path.insert(0,os.path.dirname(__file__)); import decode; decode.FAST=True
p=sys.argv[1]; us=sys.argv[2]; R0,R1=int(sys.argv[3]),int(sys.argv[4])
d=decode.load(p); g=decode.GameMap(d['map'])
born={}; lastate={}; lastmove={}
for kind,st,info in decode.walk(d):
    if kind=='split' and st.team[info[2]]==us and R0<=info[0]<R1:
        born[info[2]]=info[0]
        print(f"r{info[0]} split parent {info[1]}(len {info[4]}) -> child {info[2]}:{info[5]} parent now {len(st.body[info[1]])}")
    elif kind=='turn' and info['team']==us:
        if info['ate']: lastate[info['id']]=info['round']
    elif kind=='death' and info[2]==us and R0<=info[0]<R1:
        r,i,t,why,ln,det=info
        h=st.body[i][0]
        # free neighbours of head
        occ=st.occupied(); free=0
        for dd in 'NESW':
            n=g.step(h,dd)
            if n is not None and n not in occ: free+=1
        nen=sum(1 for j in st.alive if st.team[j]!=us and abs(st.head(j)[0]-h[0])+abs(st.head(j)[1]-h[1])<=3)
        print(f"r{r} DEATH {i} len{ln} {why}:{det} age={r-born.get(i,-1)} lastate={lastate.get(i)} head={h} freeNbrs={free} enemyHeads<=3:{nen} alive_us={sum(1 for j in st.alive if st.team[j]==us)}")
