"""per-round series for one replay: python3 series.py <replay> <us A|B> [step]"""
import sys, collections, decode
d = decode.load(sys.argv[1]); us = sys.argv[2]; step = int(sys.argv[3]) if len(sys.argv) > 3 else 5
g = decode.GameMap(d['map'])
C = collections.Counter()
print('rnd  us:n tot ate dead h2h split | th:n tot ate dead h2h split')
for kind, st, info in decode.walk(d):
    if kind == 'turn':
        s = 'u' if info['team'] == us else 't'
        C[s + 'ate'] += info['ate']
        if info['action'] and info['action'][0] == 'split': C[s + 'split'] += 1
    elif kind == 'death':
        s = 'u' if info[2] == us else 't'
        C[s + 'dead'] += 1
        if info[3] == 'head_to_head': C[s + 'h2h'] += 1
    elif kind == 'round_end' and info % step == step - 1:
        row = []
        for t, s in ((us, 'u'), ('B' if us == 'A' else 'A', 't')):
            ls = [st.length(j) for j in st.alive if st.team[j] == t]
            row.append(f"{len(ls):3d} {sum(ls):4d} {C[s+'ate']:3d} {C[s+'dead']:3d} {C[s+'h2h']:3d} {C[s+'split']:3d}")
        print(f"{info:3d}  " + ' | '.join(row))
        C = collections.Counter()
