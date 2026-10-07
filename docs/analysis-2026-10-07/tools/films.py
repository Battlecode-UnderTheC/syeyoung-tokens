"""Render filmstrips for a list of (game, round, x, y, radius, step, tag).  python3 films.py list.json"""
import json, os, sys
sys.path.insert(0, os.path.dirname(__file__))
import render, decode
decode.FAST = False
for g, rnd, x, y, rad, step, tag, us in json.load(open(sys.argv[1])):
    out = os.path.join(os.path.dirname(__file__), f'../img/{g}_{tag}.png')
    if os.path.exists(out):
        continue
    os.environ['OURS'] = us
    at = (x, y) if x is not None else None
    print(render.film(os.path.join(os.path.dirname(__file__), f'../data/rep/{g}.replay.gz'), rnd, out, at=at,
                      radius=rad if at else None, frames=9, step=step), flush=True)
