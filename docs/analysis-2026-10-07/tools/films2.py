"""python3 films2.py '[[game, src(rep|rep16|r16), round, step, frames, tag, us, x, y, radius], ...]'"""
import json, os, sys
sys.path.insert(0, os.path.dirname(__file__))
import render
D = {'rep': '../data/rep', 'rep16': '../data/rep16', 'r16': '../../arena-r16/data/rep'}
for g, src, rnd, step, fr, tag, us, *xyr in json.loads(sys.argv[1]):
    out = f'../img/{g}_{tag}.png'
    if os.path.exists(out): continue
    p = f"{D[src]}/{g}.replay.gz"
    if src == 'rep16' and not os.path.exists(p): p = f"{D['r16']}/{g}.replay.gz"
    os.environ['OURS'] = us
    at = (xyr[0], xyr[1]) if xyr else None
    print(render.film(p, rnd, out, at=at, radius=xyr[2] if xyr else None, frames=fr, step=step), flush=True)
