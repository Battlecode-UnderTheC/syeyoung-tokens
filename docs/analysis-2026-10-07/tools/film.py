"""python3 film.py '[[game, round, step, frames, tag, x, y, radius], ...]'  (x,y,radius optional)"""
import json, os, sys
sys.path.insert(0, os.path.dirname(__file__))
import render
rows = {r['id']: r for r in json.load(open(os.path.join(os.path.dirname(__file__), '../data/rows.json')))}
for g, rnd, step, fr, tag, *xyr in json.loads(sys.argv[1]):
    out = os.path.join(os.path.dirname(__file__), f'../img/{g}_{tag}.png')
    if os.path.exists(out): continue
    os.environ['OURS'] = rows[g]['us']
    p = os.path.join(os.path.dirname(__file__), f'../data/rep/{g}.replay.gz')
    if not os.path.exists(p): p = os.path.join(os.path.dirname(__file__), f'../data/repw/{g}.replay.gz')
    at = (xyr[0], xyr[1]) if xyr else None
    print(render.film(p, rnd, out, at=at, radius=xyr[2] if xyr else None, frames=fr, step=step), flush=True)
