import json,sys,collections
R=[json.loads(l) for l in open(sys.argv[1])]
PH=[(0,60),(60,150),(150,300),(300,430),(430,500)]
def cat(w):
    if w.startswith('head_to_head:mover'): return 'H2H_mover'
    if w.startswith('head_to_head:hit_by_enemy'): return 'H2H_hit'
    if w.startswith('head_to_head:hit_by_ally'): return 'H2H_ally'
    if 'resign' in w: return 'resign'
    if 'split_fail' in w: return 'splitfail'
    if w.startswith('no_valid'): return 'novalid'
    if w.startswith('other_body:into_enemy'): return 'into_enemy'
    if w.startswith('other_body'): return 'into_ally'
    return w.split(':')[0]
for lab,sel in (('W',[r for r in R if r['won']]),('L',[r for r in R if not r['won']])):
    print(f'=== {lab} n={len(sel)}  per game: count (length lost)')
    for a,b in PH:
        C={s:collections.Counter() for s in ('us','th')}; Ln={s:collections.Counter() for s in ('us','th')}
        for r in sel:
            for d in r['deaths']:
                if a<=d['r']<b:
                    c=cat(d['why']); C[d['s']][c]+=1; Ln[d['s']][c]+=d['len']
        keys=sorted(set(C['us'])|set(C['th']),key=lambda k:-(C['us'][k]+C['th'][k]))
        n=len(sel)
        print(f' r{a}-{b}: '+'  '.join(f"{k}={C['us'][k]/n:.1f}({Ln['us'][k]/n:.0f})/{C['th'][k]/n:.1f}({Ln['th'][k]/n:.0f})" for k in keys))
