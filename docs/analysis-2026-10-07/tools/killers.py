import sys,collections; sys.path.insert(0,'.'); import decode
from qdeath import tdist
p,us=sys.argv[1],sys.argv[2]
d=decode.load(p); cur=None; curlen=None; out=[]
for k,st,info in decode.walk(d):
    if k=='pre': cur=info; curlen=st.length(info)
    if k=='death' and info[0]>=360 and info[3]=='head_to_head' and info[2]==us and info[4]>=8:
        out.append((info[0],info[1],info[4],info[5],'killer',cur,'len',curlen,'killer team', 'them' if st.team[cur]!=us else 'us'))
for o in out: print(*o)
