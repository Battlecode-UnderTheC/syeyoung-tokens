"""arena-r17: the four g42 mechanisms (plus context) per replay -> one JSON line per game.
python3 mech.py <rows.json> <repdir> <out.jsonl> <shard 0|1> [ids-file]
  contest: early (r<60) pearl races won/lost (a pearl eaten while the other side's head was within 2
           and approaching), early head-on deaths, splits, dragons at r30/60
  call:    rounds with an active call in our ring words (bits 48-51), heads within 3 of the call tile,
           max heads in any 5x5 box r<60 (crowding)
  gate:    our queen's last ring-word sighting bits by r430, ground-truth last sighting of the enemy queen
           by our heads, queen lengths at r430/r499, queen multi-step moves r430+
  resign:  our/their resigns r360+, while own queen alive?, team size, champion growth, who eats the
           resign pearls
"""
import json, sys, os, collections
sys.path.insert(0, os.path.dirname(__file__))
import decode
decode.SKIP = {2, 5, 6, 7, 8}

rows = {r['id']: r for r in json.load(open(sys.argv[1]))}
repd, outp, shard = sys.argv[2], sys.argv[3], int(sys.argv[4])
want = set(json.load(open(sys.argv[5]))) if len(sys.argv) > 5 else None
done = set()
if os.path.exists(outp):
    done = {json.loads(l)['id'] for l in open(outp) if l.strip()}
fo = open(outp, 'a')


def tdist(g, a, b):
    dx = abs(a[0] - b[0]); dy = abs(a[1] - b[1]); dx = min(dx, g.W - dx); dy = min(dy, g.H - dy)
    return dx + dy, max(dx, dy)


def analyse(path, us):
    d = decode.load(path); g = decode.GameMap(d['map'])
    queens = {t: k for k, (t, _) in enumerate(g.dragons[:2])}
    side = lambda t: 'us' if t == us else 'th'
    O = dict(W=g.W, H=g.H, starters=sum(1 for t, _ in g.dragons if t == us))
    # ---- sonar pass: our ring words (0xBA, team bit 25) -> call bits and qgate sighting
    team_bit = None
    rnd = -1
    calls = collections.Counter(); call_tiles = {}
    qsight = {}  # round -> (valid, seen_round) from our queen's pings
    teamsight = {}
    # which team bit is ours? our ids = dragons of team `us`; collect sender ids by team via walk later.
    pings = []
    for ev in d['events']:
        if ev[0] == 'roundStart':
            rnd = ev[1]
        elif ev[0] == 'sonarPing':
            w = ev[3]
            if (w >> 56) == 0xBA:
                pings.append((rnd, ev[1], w))
    # ---- state pass
    team_of = {}
    E = collections.Counter()
    early_heads_prev = {}
    last_seen_eq = -1  # last round our heads saw the enemy queen
    eq_dead = None; uq_dead = None
    snap = {}
    resign_tiles = {}  # tile -> round dropped (our resigns r360+)
    max_box = 0; box_rounds5 = 0
    champ = {}
    ql = {}
    qmulti430 = 0; qsteps430 = 0
    pre_pearls = None; pre_heads = None
    for kind, st, info in decode.walk(d):
        if kind == 'pre':
            team_of.update(st.team)
            if st.round < 60:
                pre_pearls = set(st.pearls)
                pre_heads = {j: st.head(j) for j in st.alive}
        elif kind == 'turn':
            r = info['round']; s = side(info['team']); a = info['action']
            if r < 60 and info['ate'] and pre_pearls is not None:
                other = 'th' if s == 'us' else 'us'
                for t in info['path']:
                    if t not in pre_pearls:
                        continue
                    # did the other side race for it? an other-side head within 2 that approached last turn
                    raced = False
                    for j, h in pre_heads.items():
                        if side(st.team[j]) != other or j not in st.alive:
                            continue
                        dn = tdist(g, h, t)[0]
                        hp = early_heads_prev.get(j)
                        if dn <= 2 and hp is not None and tdist(g, hp, t)[0] > dn:
                            raced = True; break
                    if raced:
                        E[s + '_race_won'] += 1
                    E[s + '_ate_early'] += 1
            if r < 60 and a and a[0] == 'split':
                E[s + '_splits_early'] += 1
            if r < 60 and info['id'] in st.alive:
                early_heads_prev[info['id']] = info['head_before']
            if r >= 430 and info['id'] == queens[us] and a and a[0] == 'move':
                qsteps430 += 1
                if len(a[1]) > 1:
                    qmulti430 += 1
            if r >= 360 and info['ate']:
                for t in info['path']:
                    if t in resign_tiles:
                        E[s + '_ate_our_resign_pearls'] += 1
                        del resign_tiles[t]
                ls = max(((st.length(j), j) for j in st.alive if st.team[j] == info['team']), default=(0, -1))
                if ls[1] == info['id']:
                    E[s + '_champ_ate'] += info['ate']
        elif kind == 'death':
            rr, i, team, reason, length, detail = info
            s = side(team)
            if rr < 60 and reason == 'head_to_head':
                E[s + '_h2h_early'] += 1
            if rr < 60:
                E[s + '_dead_early'] += 1
            if i == queens[us] and uq_dead is None: uq_dead = rr
            if i == queens['B' if us == 'A' else 'A'] and eq_dead is None: eq_dead = rr
            if rr >= 360 and reason == 'no_valid_action' and detail == 'resign':
                qalive = queens[team] in st.alive and queens[team] != i
                ph = '360' if rr < 400 else '400' if rr < 430 else '430'
                E[f'{s}_resign_{ph}'] += 1
                E[f'{s}_resign_len'] += length
                if qalive:
                    E[f'{s}_resign_qalive'] += 1
                if s == 'us':
                    for k, t in enumerate(st.body[i]):
                        if k % 2 == 0:
                            resign_tiles[t] = rr
                    # beside the champion?
                    ls = max(((st.length(j), j) for j in st.alive if st.team[j] == team and j != i), default=(0, -1))
                    if ls[1] >= 0 and tdist(g, st.head(ls[1]), st.head(i))[0] <= 3:
                        E['us_resign_beside_champ'] += 1
        elif kind == 'round_end':
            r = info
            eq = queens['B' if us == 'A' else 'A']
            if eq in st.alive:
                segs = st.body[eq]
                for j in st.alive:
                    if st.team[j] == us and any(g.in_window(st.head(j), x) for x in segs):
                        last_seen_eq = r; break
            if r < 60:
                hs = [st.head(j) for j in st.alive if st.team[j] == us]
                best = 0
                for h in hs:
                    c = sum(1 for h2 in hs if tdist(g, h, h2)[1] <= 2)
                    best = max(best, c)
                max_box = max(max_box, best)
                if best >= 5: box_rounds5 += 1
            if r in (30, 60, 100, 300, 360, 400, 430, 460, 499):
                snap[r] = {}
                for t in ('A', 'B'):
                    ls = sorted((st.length(j) for j in st.alive if st.team[j] == t), reverse=True)
                    snap[r][side(t)] = [len(ls), sum(ls), ls[0] if ls else 0, st.length(queens[t]) if queens[t] in st.alive else 0]
            if r == 429:
                O['eq_last_seen_by430'] = last_seen_eq
    O['eq_last_seen'] = last_seen_eq
    # our pings: identify ours by sender team
    qs = None; ts = None
    for (r, sid, w) in pings:
        if team_of.get(sid) != us:
            continue
        surplus = (w >> 48) & 15
        if surplus and r < 60:
            calls[r] += 1
            call_tiles[r] = ((w >> 36) & 63, (w >> 42) & 63)
        if r <= 430:
            valid = (w >> 35) & 1; seen = (w >> 26) & 511
            if sid == queens[us]:
                qs = (r, valid, seen)
            ts = (r, valid, seen) if ts is None or r >= ts[0] else ts
    O['q_ring_word_430'] = qs; O['team_ring_word_430'] = ts
    O['call_rounds'] = len(calls); O['call_first'] = min(calls) if calls else None
    O.update(E=dict(E), snap=snap, max_box=max_box, box_rounds5=box_rounds5, eq_dead=eq_dead, uq_dead=uq_dead,
             qmulti430=qmulti430, qsteps430=qsteps430)
    return O


for f in sorted(os.listdir(repd)):
    if not f.endswith('.replay.gz'):
        continue
    i = int(f.split('.')[0])
    if i % 2 != shard or i in done or i not in rows or (want is not None and i not in want):
        continue
    r = rows[i]
    try:
        o = analyse(os.path.join(repd, f), r['us'])
    except Exception as e:
        print(i, 'ERR', repr(e), file=sys.stderr, flush=True); continue
    o.update(id=i, res=r['res'], opp=r['opp'], map=r['map'], suite=r['suite'], us=r['us'], seed=r.get('seed'))
    fo.write(json.dumps(o) + '\n'); fo.flush()
