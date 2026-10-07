"""Elimination losses: our queen death round, their queen death round, game end round, our count/total at our queen's
death, theirs too, and at r(end-50). python3 elim.py games.json out ids"""
import json, sys, os
sys.path.insert(0, os.path.dirname(__file__)); import decode; decode.FAST = True
G = {x['id']: x for x in json.load(open(sys.argv[1]))}
o = open(sys.argv[2], 'w')
for gid in map(int, sys.argv[3:]):
    p = os.path.join(os.path.dirname(sys.argv[2]) or '.', 'rep', f'{gid}.replay.gz')
    if not os.path.exists(p): continue
    x = G[gid]; us = x['meta']['our_seat']; d = decode.load(p)
    qd = {}; snap = {}; end = 0; tl = {}
    for kind, st, info in decode.walk(d):
        if kind == 'death' and info[1] in (0, 1):
            s = 'us' if info[2] == us else 'th'
            n = {t: sum(1 for j in st.alive if ('us' if st.team[j] == us else 'th') == t) for t in ('us', 'th')}
            qd[s] = dict(r=info[0], why=info[3] + ':' + info[5], n=n)
        if kind == 'round_end':
            end = info
            if info % 10 == 0:
                tl[info] = {t: sum(1 for j in st.alive if ('us' if st.team[j] == us else 'th') == t) for t in ('us', 'th')}
    o.write(json.dumps(dict(id=gid, map=x['map'], opp=x['meta']['opponent'], end=end, qd=qd, tl=tl)) + '\n')
