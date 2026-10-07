"""sprint-1 v sprint-21 (and sprint-1's later games) side by side, from replay-derived files."""
import json, statistics as S, collections, re
SETS = {
 's1':  dict(games='real_ring-sprint-1.json', feat='feat_s1.jsonl', qd='qd_s1.jsonl', bed='cmp_bedprox_s1.jsonl', ps='cmp_portsprint_s1.jsonl', su='cmp_sprintuse_s1.jsonl', th='cmp_qthreat_s1.jsonl', ql='cmp_qlife_s1.jsonl', bc='cmp_bc_s1.jsonl', sp='cmp_spacing_s1.jsonl', last='qlast.jsonl', esc='qesc_us.jsonl'),
 's21': dict(games='s21/real588.json', feat='s21/feat588.jsonl', qd='s21/qd588.jsonl', bed='s21/bedprox588.jsonl', ps='s21/portsprint588.jsonl', su='s21/sprintuse588.jsonl', th='s21/qthreat588.jsonl', ql='s21/qlife588.jsonl', bc='s21/bc588.jsonl', sp='s21/spacing588.jsonl', last='s21/qlast588.jsonl', esc='s21/qesc588.jsonl'),
 's1new': dict(games='s1new/real_new.json', feat='s1new/feat.jsonl', qd='s1new/qd.jsonl', bed='s1new/bedprox.jsonl', ps='s1new/portsprint.jsonl', su='s1new/sprintuse.jsonl', th='s1new/qthreat.jsonl', ql='s1new/qlife.jsonl', bc='s1new/bc.jsonl', sp='s1new/spacing.jsonl', last='s1new/qlast.jsonl', esc='s1new/qesc.jsonl'),
}
def L(f):
    try: return [json.loads(l) for l in open(f)]
    except FileNotFoundError: return []
def M(xs): return S.mean(xs) if xs else float('nan')
for name, F in SETS.items():
    G = {x['id']: x for x in json.load(open(F['games']))}
    won = {i: x['winner'] == x['meta']['our_seat'] for i, x in G.items()}
    us = {i: x['meta']['our_seat'] for i, x in G.items()}
    print(f'\n===== {name}: {len(G)} games, {sum(won.values())}-{len(G)-sum(won.values())}')
    # loss reasons
    print('loss reasons', dict(collections.Counter(x['reason'].split(',')[0] for i, x in G.items() if not won[i])))
    # queen outcomes
    Q = {r['id']: r for r in L(F['qd'])}
    c = collections.Counter()
    for i, r in Q.items():
        if i not in G: continue
        d = {q['side']: q['r'] for q in r['q']}
        k = ('ours dead' if 'us' in d else 'ours alive') + '/' + ('theirs dead' if 'th' in d else 'theirs alive')
        if 'us' in d and 'th' in d: k += ' (theirs first)' if d['th'] < d['us'] else ' (ours first)'
        c[(k, won[i])] += 1
    print('queen outcomes (n=%d):' % len(Q))
    for k in sorted({k for k, _ in c}): print(f'   {k:38s} {c[(k,True)]:3d}-{c[(k,False)]:<3d}')
    qrace = [tuple(map(int, re.findall(r'\d+', x['reason']))) for i, x in G.items() if x['reason'].startswith('longer queen') and not x['reason'].endswith(' 0')]
    print('both-alive queen races: W %d L %d' % (sum(1 for i, x in G.items() if x['reason'].startswith('longer queen') and not x['reason'].endswith(' 0') and won[i]), sum(1 for i, x in G.items() if x['reason'].startswith('longer queen') and not x['reason'].endswith(' 0') and not won[i])))
    # feat timeline
    FT = [g for g in L(F['feat']) if g['id'] in G]
    def at(g, r, s, k):
        for t in g['tl']:
            if t['r'] == r: return t[s].get(k)
    for r in (30, 60, 100, 200, 300, 400, 490):
        v = [g for g in FT if at(g, r, 'us', 'n') is not None]
        if not v: continue
        f = lambda s, k: M([at(g, r, s, k) or 0 for g in v])
        print(f'  r{r:3d} n {f("us","n"):5.1f}/{f("th","n"):5.1f} tot {f("us","tot"):6.1f}/{f("th","tot"):6.1f} lon {f("us","lon"):5.1f}/{f("th","lon"):5.1f} q {f("us","q"):5.1f}/{f("th","q"):5.1f} spread {f("us","spread"):4.1f}/{f("th","spread"):4.1f} eat {f("us","eat"):5.1f}/{f("th","eat"):5.1f} mid {f("us","eat_mid"):4.1f}/{f("th","eat_mid"):4.1f}')
    # board control
    B = [r for r in L(F['bc']) if r['id'] in G]
    if B:
        ub = lambda r, k: r['bcA'][k] if us[r['id']] == 'A' else 1 - r['bcA'][k]
        print('  board control r20/30/60/100: ' + ' '.join('%.3f' % M([ub(r, k) for r in B if k in r['bcA']]) for k in ('20', '30', '60', '100')) + ' (n%d)' % len(B))
    # bedprox
    BP = [r for r in L(F['bed']) if r['id'] in G and len(r['out']) == 8]
    th = lambda r: 'B' if r['us'] == 'A' else 'A'
    if BP: print('  beds near heads r20/r40 us/them %.2f/%.2f  %.2f/%.2f (n%d)' % (M([r['out'][r['us'] + '20'] for r in BP]), M([r['out'][th(r) + '20'] for r in BP]), M([r['out'][r['us'] + '40'] for r in BP]), M([r['out'][th(r) + '40'] for r in BP]), len(BP)))
    # portals / sprints
    PS = [r for r in L(F['ps']) if r['id'] in G]
    if PS:
        C = collections.Counter(); n = len(PS)
        for r in PS:
            for s, ph, m, v in r['c']: C[('us' if s == r['us'] else 'th', ph, m)] += v
        print('  r0-60 portal us/them %.1f/%.1f multi %.1f/%.1f | r60-100 portal %.1f/%.1f multi %.1f/%.1f' % tuple(C[(s, ph, m)] / n for ph, m in (('60', 'portal'), ('60', 'multi'), ('100', 'portal'), ('100', 'multi')) for s in ('us', 'th')))
    SU = [r for r in L(F['su']) if r['id'] in G]
    if SU:
        C = collections.Counter(); n = len(SU)
        for r in SU:
            for s, lb, m, v in r['c']: C[('us' if s == us[r['id']] else 'th', lb, m)] += v
        for s in ('us', 'th'):
            mv = C[(s, 'L5-8', 'moves')] + C[(s, 'L9+', 'moves')]; mu = C[(s, 'L5-8', 'multi')] + C[(s, 'L9+', 'multi')]
            print(f'  {s} r0-150: moves at L>=5 {mv/n:5.1f}/game, double-step use {mu/max(1,mv):.2f}; L3-4 paid sprints {C[(s,"L3-4","multi")]/n:.1f} (kill share {C[(s,"L3-4","multi_kill")]/max(1,C[(s,"L3-4","multi")]):.2f})')
    # queen threat state
    TH = [x for g in L(F['th']) if g['id'] in G for x in g['rows']]
    if TH:
        for side in (True, False):
            for a, b in ((60, 250), (250, 450), (450, 500)):
                s = [x for x in TH if x['us'] == side and a <= x['r'] < b]
                if s: print(f"  {'us' if side else 'th'} queen r{a}-{b}: L {M([x['L'] for x in s]):5.1f} threat-free {sum(x['t']>=20 for x in s)/len(s):.2f} enclosed {sum(x['sealed'] for x in s)/len(s):.2f} hazard30 safe {sum(x['d30'] for x in s if x['t']>=20)/max(1,sum(x['t']>=20 for x in s)):.3f} threatened {sum(x['d30'] for x in s if x['t']<20)/max(1,sum(x['t']<20 for x in s)):.3f}")
    QL = [g for g in L(F['ql']) if g['id'] in G]
    if QL:
        for side in ('us', 'th'):
            out = []
            for k, n in enumerate(('0-60', '60-150', '150-250', '250-350', '350-450', '450-500')):
                rows = [g['q'][side][k] for g in QL if g['q'][side][k]['alive']]; T = sum(r['alive'] for r in rows)
                out.append(f"{n}: {100*sum(r['bed']+r['drop'] for r in rows)/max(1,T):4.1f} eats/{100*sum(r['gave'] for r in rows)/max(1,T):3.1f} given")
            print(f'  {side} queen per 100 turns:', ' | '.join(out))
    LA = [r for r in L(F['last']) if r['id'] in G]
    if LA: print('  our queen head-on deaths %d: closer %d same %d away %d; multi-step at L3-4 %d of %d' % (len(LA), sum(r['after'] < r['before'] for r in LA), sum(r['after'] == r['before'] for r in LA), sum(r['after'] > r['before'] for r in LA), sum(r['steps'] > 1 for r in LA if r['L'] in (3, 4)), sum(r['L'] in (3, 4) for r in LA)))
    ES = [r for r in L(F['esc']) if r['id'] in G]
    if ES: print('  escape tile existed %.2f, 1-step %.2f, L2 share %.2f (n%d)' % (M([r['safe'] > 0 for r in ES]), M([r['safe1'] > 0 for r in ES]), M([r['L'] == 2 for r in ES]), len(ES)))
