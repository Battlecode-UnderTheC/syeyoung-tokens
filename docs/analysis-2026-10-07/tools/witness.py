"""When the enemy queen dies: how many of OUR surviving dragons have her head tile in their 7x7 window (witnesses),
and was the killer ours. python3 witness.py games.json out ids"""
import json, sys, os
sys.path.insert(0, os.path.dirname(__file__)); import decode; decode.FAST = True
G = {x['id']: x for x in json.load(open(sys.argv[1]))}
o = open(sys.argv[2], 'w')
for gid in map(int, sys.argv[3:]):
    p = os.path.join(os.path.dirname(sys.argv[2]) or '.', 'rep', f'{gid}.replay.gz')
    if not os.path.exists(p): continue
    x = G[gid]; us = x['meta']['our_seat']; d = decode.load(p); g = decode.GameMap(d['map'])
    res = None; mover = None
    for kind, st, info in decode.walk(d):
        if kind == 'pre': mover = info
        if kind == 'death' and info[1] in (0, 1) and info[2] != us:
            qh = st.head(info[1])
            wit = [j for j in st.alive if st.team[j] == us and j != mover and j != info[1] and g.in_window(st.head(j), qh, 3)]
            res = dict(r=info[0], why=info[3] + ':' + info[5], witnesses=len(wit), mover_team=('us' if mover is not None and st.team.get(mover) == us else 'th'))
            break
    o.write(json.dumps(dict(id=gid, res=res)) + '\n')
