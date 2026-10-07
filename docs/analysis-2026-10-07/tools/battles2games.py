"""Battle folders (bcweb fetch) -> games.json + rep/<game>.replay.gz, from the point of view of team TEAM.
our_seat = TEAM's seat in each game (siteWinner slot v winner seat; draws are skipped).
python3 battles2games.py ROOT TEAM out_dir [battle ids...]"""
import json, os, sys, gzip, glob
root, team, out = sys.argv[1], sys.argv[2], sys.argv[3]
ids = sys.argv[4:]
os.makedirs(out + '/rep', exist_ok=True)
G = []
folders = [f'{root}/M{i}' for i in ids] if ids else sorted(glob.glob(root + '/M*'))
for d in folders:
    p = d + '/battle.json'
    if not os.path.exists(p): continue
    b = json.load(open(p))
    if team not in (b['teamA'], b['teamB']): continue
    opp = b['teamB'] if b['teamA'] == team else b['teamA']
    for g in b['games']:
        sw, w = (g.get('siteWinner') or '').upper(), g.get('winner')
        if sw not in ('A', 'B') or w not in ('A', 'B'): continue
        teamA_seat = 'A' if sw == w else 'B'
        seat = teamA_seat if b['teamA'] == team else ('B' if teamA_seat == 'A' else 'A')
        src = f"{d}/M{g['id']}.replay"
        dst = f"{out}/rep/{g['id']}.replay.gz"
        if not os.path.exists(src): continue
        if not os.path.exists(dst):
            with open(src, 'rb') as fi, gzip.open(dst, 'wb', compresslevel=6) as fo: fo.write(fi.read())
        G.append(dict(id=g['id'], a='A', b='B', winner=w, reason=g.get('endReason'), map=g['map'],
                      meta=dict(our_seat=seat, opponent=opp, ranked=b['match'].get('ranked'), battle=b['battle'])))
json.dump(G, open(out + '/games.json', 'w'))
print(team, len(G), 'games', sum(x['winner'] == x['meta']['our_seat'] for x in G), 'won')
