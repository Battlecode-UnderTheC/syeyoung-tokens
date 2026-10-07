import json,sys,statistics as S,collections
R=[json.loads(l) for l in open(sys.argv[1])]
def at(r,rr,s,k):
    v=0
    for x in r['tl']:
        if x['r']<=rr: v=x[s].get(k,0)
    return v
by=collections.defaultdict(list)
for r in R: by[r['opp']].append(r)
cols=[('n',30),('n',100),('n',300),('spread',30),('spread',100),('eat',60),('eat_mid',60),('eat_far',60),('portal',100),('lon',200),('lon',300),('lon',400),('q',300),('q',400),('q',499)]
print('opp'.ljust(20),'N  W%  '+'  '.join(f'{k}@{rr}' for k,rr in cols))
def row(name,sel,s):
    return name.ljust(20)+f'{len(sel):3d} {sum(r["won"] for r in sel)/len(sel):.2f} '+'  '.join(f'{S.mean(at(r,rr,s,k) for r in sel):6.1f}' for k,rr in cols)
for o,v in sorted(by.items(),key=lambda kv:-len(kv[1])):
    if len(v)<9: continue
    print(row(o[:20],v,'th'))
print(row('US (all)',R,'us'))
