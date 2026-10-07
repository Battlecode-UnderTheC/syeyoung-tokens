import json,os,sys,collections,statistics as S
g={x['id']:x for x in json.load(open(sys.argv[1]))}
RS=[3,6,10,20,30,40,45,49]  # indexes into rounds list (every 10): r30,60,100,200,300,400,450,490
M=['dragons','total_length','longest','queen_length','pearls_eaten','board_control','kills','deaths','splits','tiles_seen']
rows=[]
for i,x in g.items():
    p=f'met/{i}.json'
    if not os.path.exists(p): continue
    m=json.load(open(p)); us=x['meta']['our_seat']; th='B' if us=='A' else 'A'
    ev=m['game']['eval']; ev=[e if us=='A' else 1-e for e in ev]; ev=ev+[ev[-1]]*(51-len(ev))
    won=x['winner']==us
    # turning point: first index after which ev stays on the final side of .5
    tp=None
    for k in range(len(ev)):
        if all((e<0.5) for e in ev[k:-1]) : tp=k*10; break
    row=dict(id=i,won=won,ev=ev,tp=tp,map=x['map'],opp=x['meta']['opponent'],reason=x['reason'].split(',')[0])
    for k in M:
        a=m['team'][k][us]; b=m['team'][k][th]
        row[k]=[(a[j] if j<len(a) else a[-1], b[j] if j<len(b) else b[-1]) for j in RS]
    rows.append(row)
json.dump(rows,open('metrows.json','w'))
print('games',len(rows))
for lab,sel in (('WINS',[r for r in rows if r['won']]),('LOSSES',[r for r in rows if not r['won']])):
    print(f'== {lab} n={len(sel)}  mean our P at r30/60/100/200/300/400/450/490:',
          [round(S.mean(r['ev'][j] for r in sel),2) for j in RS])
    for k in M:
        print(f'  {k:14s}', ' '.join(f"{S.mean(r[k][t][0] for r in sel):6.1f}/{S.mean(r[k][t][1] for r in sel):<6.1f}" for t in range(len(RS))))
L=[r for r in rows if not r['won']]
c=collections.Counter()
for r in L:
    t=r['tp']; b='<=50' if t is not None and t<=50 else '<=100' if t is not None and t<=100 else '<=200' if t is not None and t<=200 else '<=300' if t is not None and t<=300 else '<=400' if t is not None and t<=400 else '>400'
    c[b]+=1
print('loss turning points (eval stays <.5 from):',sorted(c.items()))
