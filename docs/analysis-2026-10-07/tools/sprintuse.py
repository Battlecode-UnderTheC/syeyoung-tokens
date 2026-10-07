"""Multi-step moves r0-150: by mover length; free vs paid; outcome: ate (pearls), killed (head-on), died; and whether a
pearl was on the path tiles beyond the first step. python3 sprintuse.py games.json out ids"""
import json, sys, os, collections
sys.path.insert(0, os.path.dirname(__file__)); import decode; decode.FAST = True
G = {x['id']: x for x in json.load(open(sys.argv[1]))}
o = open(sys.argv[2], 'w')
for gid in map(int, sys.argv[3:]):
    p = os.path.join(os.path.dirname(sys.argv[2]) or '.', 'rep', f'{gid}.replay.gz')
    if not os.path.exists(p): continue
    x = G[gid]; d = decode.load(p); C = collections.Counter()
    for kind, st, info in decode.walk(d):
        if kind != 'turn' or info['round'] >= 150: continue
        a = info['action']
        if not a or a[0] != 'move': continue
        s = info['team']; L = info['len_before']; n = len(a[1]); free = (L + 3) // 4
        lb = 'L2' if L <= 2 else 'L3-4' if L <= 4 else 'L5-8' if L <= 8 else 'L9+'
        C[(s, lb, 'moves')] += 1
        if n > 1:
            C[(s, lb, 'multi')] += 1; C[(s, lb, 'paid')] += max(0, n - free)
            C[(s, lb, 'multi_ate')] += info['ate'] > 0; C[(s, lb, 'multi_kill')] += len(info['killed']) > 0
            C[(s, lb, 'multi_died')] += info['died'] is not None
        else:
            C[(s, lb, 'single_ate')] += info['ate'] > 0
    o.write(json.dumps(dict(id=gid, c=[[*k, v] for k, v in C.items()])) + '\n')
