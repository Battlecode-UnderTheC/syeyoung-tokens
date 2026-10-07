"""Late-game concentration: for r>=360, splits/resigns/deaths by our and their team relative to the team's
longest, and the longest dragon's growth.  python3 late.py out.jsonl shard ids-file"""
import json, sys, os, collections
sys.path.insert(0, os.path.dirname(__file__))
import decode
decode.FAST = True

rows = {r['id']: r for r in json.load(open(os.path.join(os.path.dirname(__file__), '../data/rows.json')))}
outp, shard = sys.argv[1], int(sys.argv[2])
want = set(json.load(open(sys.argv[3])))
repd = os.path.join(os.path.dirname(__file__), '../data/rep')
done = set()
if os.path.exists(outp):
    done = {json.loads(l)['id'] for l in open(outp) if l.strip()}
fo = open(outp, 'a')
for f in sorted(os.listdir(repd)):
    i = int(f.split('.')[0])
    if i % 2 != shard or i in done or i not in want:
        continue
    r = rows[i]; us = r['us']
    d = decode.load(os.path.join(repd, f)); g = decode.GameMap(d['map'])
    queens = {t: k for k, (t, _) in enumerate(g.dragons[:2])}
    C = {'us': collections.Counter(), 'th': collections.Counter()}
    top_hist = {'us': [], 'th': []}
    seen = {'us': [], 'th': []}
    def top(st, t):
        ls = sorted(((st.length(j), j) for j in st.alive if st.team[j] == t), reverse=True)
        return ls
    for kind, st, info in decode.walk(d):
        if kind == 'split':
            rnd, p, c, t, before, clen = info
            if rnd < 360: continue
            s = 'us' if t == us else 'th'
            ls = top(st, t)
            rank = 1 + sum(1 for L, j in ls if j not in (p, c) and L > before)
            ph = '360' if rnd < 430 else '430'
            C[s][ph + ' splits'] += 1
            if rank <= 3: C[s][ph + ' splits by top3'] += 1; C[s][ph + ' split-off len by top3'] += clen
            if p == queens[t]: C[s][ph + ' queen splits'] += 1
        elif kind == 'death':
            rnd, k, team, reason, length, detail = info
            if rnd < 360: continue
            s = 'us' if team == us else 'th'
            ph = '360' if rnd < 430 else '430'
            ls = top(st, team)
            rank = 1 + sum(1 for L, j in ls if j != k and L > length)
            if rank <= 3 and length >= 8:
                C[s][ph + ' top3 deaths'] += 1; C[s][ph + ' top3 death len'] += length
                C[s][ph + ' top3 death ' + reason + '/' + detail] += 1
            if reason == 'no_valid_action':
                C[s][ph + ' resign len'] += length
        elif kind == 'turn':
            if info['round'] >= 360 and info['ate']:
                s = 'us' if info['team'] == us else 'th'
                ls = top(st, info['team'])
                ph = '360' if info['round'] < 430 else '430'
                C[s][ph + ' ate'] += info['ate']
                if ls and ls[0][1] == info['id']: C[s][ph + ' ate by #1'] += info['ate']
                a = info['action']
                if a and a[0] == 'move' and len(a[1]) > 1: C[s][ph + ' ate on sprint'] += info['ate']
        elif kind == 'round_end' and info >= 300:
            for t in ('A', 'B'):
                s = 'us' if t == us else 'th'
                eq = queens['B' if t == 'A' else 'A']
                if eq in st.alive:
                    segs = list(st.body[eq])
                    if any(g.in_window(st.head(j), x) for j in st.alive if st.team[j] == t for x in segs):
                        seen[s].append(info)
            if info in (360, 400, 430, 460, 480, 499):
              for t in ('A', 'B'):
                s = 'us' if t == us else 'th'
                ls = top(st, t)
                top_hist[s].append((info, ls[0][0] if ls else 0, ls[0][1] if ls else -1))
    # champion stability: is the r499 #1 the same dragon as r430's #1?
    out = dict(id=i, C={s: dict(c) for s, c in C.items()}, top=top_hist, seen=seen)
    fo.write(json.dumps(out) + '\n'); fo.flush()
