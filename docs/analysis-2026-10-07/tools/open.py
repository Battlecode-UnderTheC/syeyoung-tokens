import json,sys,statistics as S,collections
R=[json.loads(l) for l in open(sys.argv[1])]
def at(r,rr,s,k):
    for x in r['tl']:
        if x['r']==rr: return x[s].get(k,0)
    return r['tl'][-1][s].get(k,0)
def m(v): return round(S.mean(v),1) if v else 0
K=['n','tot','eat','eat_own','eat_mid','eat_far','z_mid','z_far','spread','far','split','multi','portal','dlen']
for lab,sel in (('W',[r for r in R if r['won']]),('L',[r for r in R if not r['won']])):
    print(f'=== {lab} n={len(sel)}')
    for rr in (10,20,30,40,60):
        print(f' r{rr}: '+'  '.join(f"{k}={m([at(r,rr,'us',k) for r in sel])}/{m([at(r,rr,'th',k) for r in sel])}" for k in K))
