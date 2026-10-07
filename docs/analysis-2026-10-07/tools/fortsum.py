import json,sys,collections
R=[json.loads(l) for l in open(sys.argv[1])]
# hazard: P(queen dies within next 30 rounds | sample)
def hz(rows):
    n=len(rows); d=sum(1 for x in rows if x['died'] is not None and x['r']<x['died']<=x['r']+30)
    return f"{d}/{n}={d/max(n,1):.2f}"
rows=[x for g in R for x in g['rows']]
print('samples',len(rows),'queens >=8 at some point', len({(g['id'],x['q']) for g in R for x in g['rows']}))
print('overall',hz(rows))
for lab,f in [('eb<=3',lambda x:x['eb']<=3),('eb 4-8',lambda x:4<=x['eb']<=8),('eb 9-40',lambda x:9<=x['eb']<=40),('sealed(eb99)',lambda x:x['eb']==99)]:
    print(lab,hz([x for x in rows if f(x)]))
for lab,f in [('own5<.15',lambda x:x['own5']<.15),('own5 .15-.3',lambda x:.15<=x['own5']<.3),('own5>=.3',lambda x:x['own5']>=.3)]:
    print(lab,hz([x for x in rows if f(x)]))
for lab,f in [('en5=0',lambda x:x['en5']==0),('en5>0',lambda x:x['en5']>0)]:
    print(lab,hz([x for x in rows if f(x)]))
for lab,f in [('L8-15',lambda x:x['L']<=15),('L16-30',lambda x:16<=x['L']<=30),('L>30',lambda x:x['L']>30)]:
    print(lab,hz([x for x in rows if f(x)]))
print('combined own5>=.3 & eb>=9', hz([x for x in rows if x['own5']>=.3 and x['eb']>=9]))
print('combined own5<.15 & eb<=8', hz([x for x in rows if x['own5']<.15 and x['eb']<=8]))
