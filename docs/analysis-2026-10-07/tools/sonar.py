"""Sonar usage per side: pings per dragon-turn, share of pings that hit an ally / enemy / nothing, by phase."""
import json,sys,os,collections
sys.path.insert(0,os.path.dirname(__file__)); import decode
decode.FAST=False; decode.SKIP={2}
G={x['id']:x for x in json.load(open(sys.argv[1]))}
per=int(sys.argv[3]); seen=collections.Counter()
out=open(sys.argv[2],'w')
for gid,x in G.items():
    o=x['meta']['opponent']
    if seen[o]>=per: continue
    p=f'rep/{gid}.replay.gz'
    if not os.path.exists(p): continue
    seen[o]+=1
    us=x['meta']['our_seat']; d=decode.load(p); g=decode.GameMap(d['map'])
    team={i:t for i,(t,_) in enumerate(g.dragons)}
    turns=collections.Counter(); pings=collections.Counter(); hit=collections.Counter(); words=collections.defaultdict(set)
    for ev in d['events']:
        k=ev[0]
        if k=='dragonSplit': team[ev[2]]=ev[3]
        elif k=='turnStart':
            s='us' if team.get(ev[1])==us else 'th'; turns[s]+=1
        elif k=='sonarPing':
            s='us' if team.get(ev[1])==us else 'th'; pings[s]+=1
            h=ev[6]
            if h is None: hit[(s,'none')]+=1
            else: hit[(s,'ally' if team.get(h)==team.get(ev[1]) else 'enemy')]+=1
            words[s].add(ev[3]>>56)
    out.write(json.dumps(dict(id=gid,opp=o,turns=dict(turns),pings=dict(pings),hit={f'{a}_{b}':v for (a,b),v in hit.items()},tops={s:len(v) for s,v in words.items()}))+'\n')
