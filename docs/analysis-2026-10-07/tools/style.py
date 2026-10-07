import json,sys,statistics as S,collections
R=[json.loads(l) for l in open(sys.argv[1])]
def at(r,rr,s,k):
    v=0
    for x in r['tl']:
        if x['r']<=rr: v=x[s].get(k,0)
    return v
print(sys.argv[1],'n',len(R),'W',sum(r['won'] for r in R))
for rr in (20,30,60,100,200,300,400,450,499):
    print(f' r{rr}: '+'  '.join(f"{k}={S.mean(at(r,rr,'us',k) for r in R):.1f}/{S.mean(at(r,rr,'th',k) for r in R):.1f}" for k in ('n','tot','lon','q','eat','eat_mid','eat_far','spread','portal','multi','split','dlen')))
def cat(w):
    if 'resign' in w: return 'resign'
    if w.startswith('head_to_head'): return 'h2h'
    if w.startswith('other_body'): return 'body'
    if 'split_fail' in w: return 'splitfail'
    return w.split(':')[0]
C={s:collections.Counter() for s in ('us','th')}
for r in R:
    for d in r['deaths']: C[d['s']][cat(d['why'])]+=1
n=len(R)
print(' deaths/game us',{k:round(v/n,1) for k,v in C['us'].most_common()},' th',{k:round(v/n,1) for k,v in C['th'].most_common()})
