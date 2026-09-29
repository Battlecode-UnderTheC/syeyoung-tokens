import sys, json, os
sys.path.insert(0, "/Users/syeyoung/Documents/postech/unswbattlecode/syeyoung-tokens.league/replays/league-2/analysis")
import decode
R = "/Users/syeyoung/Documents/postech/unswbattlecode/syeyoung-tokens.league/replays"
out = {}
for line in open(R + "/server-seeds.tsv"):
    rid, mp, seed, side, res, sub = line.rstrip("\n").split("\t")
    path = next(os.path.join(dp, rid + ".replay") for dp, _, fs in os.walk(R) if rid + ".replay" in fs and "league-" not in dp)
    d = decode.load(path)
    ours = decode.our_team(d)
    assert ours == side, (rid, ours, side)
    opp = "B" if ours == "A" else "A"
    snaps = {}
    eat = {'A': [0]*5, 'B': [0]*5}; spl = {'A': [0]*5, 'B': [0]*5}; dth = {'A': [0]*5, 'B': [0]*5}; cz = {'A': {}, 'B': {}}
    for kind, st, info in decode.walk(d):
        if kind == "turn": eat[info["team"]][min(4, info["round"] // 100)] += info["ate"]
        if kind == "split": spl[info[3]][min(4, info[0] // 100)] += 1
        if kind == "death" and info[0] < 200: cz[info[2]][info[3] + ':' + info[5]] = cz[info[2]].get(info[3] + ':' + info[5], 0) + 1
        if kind == "death": dth[info[2]][min(4, info[0] // 100)] += 1
        if kind == "round_end" and info in (25, 50, 100, 200, 300, 400, 450):
            snaps[str(info)] = [st.standing(t) for t in "AB"]
    snaps["final"] = [d["result"]["A"], d["result"]["B"]]
    w = d["result"]["winner"]
    out[rid] = dict(map=mp, seed=seed, ours=ours, res=res, sub=sub, winner=w, snaps=snaps, eat=[eat['A'], eat['B']], spl=[spl['A'], spl['B']], dth=[dth['A'], dth['B']], cz=[cz['A'], cz['B']])
    print(rid, mp, ours, res, w, file=sys.stderr)
json.dump(out, open(sys.argv[1], "w"))
