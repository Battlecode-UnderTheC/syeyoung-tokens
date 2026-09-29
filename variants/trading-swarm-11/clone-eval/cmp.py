import sys, json, subprocess, statistics as S
from concurrent.futures import ThreadPoolExecutor
B = "/Users/syeyoung/Documents/postech/unswbattlecode/syeyoung-tokens.worktrees/<task>/build"
clone = sys.argv[1]
verbose = "-v" in sys.argv
real = json.load(open(B + "/scratch/real.json"))
MF = {"Default": "default", "Queen Of Spades": "queen_of_spades", "Schooltime": "schooltime", "Trophy": "trophy",
      "Devil": "devil", "Trauma": "trauma", "Prisoners Dilemma": "dilemma", "Autarky": "autarky",
      "Portals": "portals", "Slithery Fight": "slithery_fight"}
BOT = {"10729": "gather-split-trade-90", "11400": "gather-split-trade-96"}
def run(item):
    rid, g = item
    ours = BOT[g["sub"]]
    a, b = (ours, clone) if g["ours"] == "A" else (clone, ours)
    out = subprocess.run([B + "/tools/snap", a, b, B + "/scratch/gamemaps/" + rid + ".map", g["seed"]],
                         capture_output=True, text=True).stdout
    return rid, json.loads(out)
with ThreadPoolExecutor(2) as ex:
    sims = dict(ex.map(run, real.items()))
KEYS = ["25", "50", "100", "200", "300", "400", "450", "final"]
agree = 0; cw = rw = 0
err = {k: {"count": [], "longest": [], "total": []} for k in KEYS}
means = {k: [[], []] for k in KEYS}
for rid, g in real.items():
    s = sims[rid]
    oi = 1 if g["ours"] == "A" else 0
    swin = "AB"[s["winner"]] if s["winner"] >= 0 else None
    sres = "W" if swin == g["ours"] else ("D" if swin is None else "L")
    agree += sres == g["res"]
    cw += sres == "L"; rw += g["res"] == "L"
    row = []
    for k in KEYS:
        if k not in g["snaps"] or k not in s["snaps"]: continue
        rv = g["snaps"][k][oi]; sv = s["snaps"][k][oi]
        rc, rl, rt = rv["count"], rv["longest"], rv["total"]
        sc, sl, st = sv
        err[k]["count"].append(abs(sc - rc)); err[k]["longest"].append(abs(sl - rl)); err[k]["total"].append(abs(st - rt))
        means[k][0].append((rc, rl, rt)); means[k][1].append((sc, sl, st))
        row.append(f"r{k} {rc}/{rl}/{rt}~{sc}/{sl}/{st}")
    if verbose:
        print(f"{rid} {g['map'][:12]:12s} {g['ours']} {BOT[g['sub']][-2:]} real {g['res']} sim {sres} {'ok' if sres==g['res'] else '--'} | " + "  ".join(row))
print(f"{clone}: result agreement {agree}/45; opponent record real {rw}-{45-rw}, clone {cw}-{45-cw} (opp wins-losses)")
for k in KEYS:
    r = means[k][0]; c = means[k][1]
    f = lambda v, i: S.mean(x[i] for x in v)
    print(f"  r{k:5s} mean count/longest/total real {f(r,0):5.1f}/{f(r,1):5.1f}/{f(r,2):6.1f}  clone {f(c,0):5.1f}/{f(c,1):5.1f}/{f(c,2):6.1f}"
          f"  MAE {S.mean(err[k]['count']):5.1f}/{S.mean(err[k]['longest']):5.1f}/{S.mean(err[k]['total']):6.1f}")
if "eat" in next(iter(real.values())):
    n = len(real)
    re_ = [0]*5; ce = [0]*5; rs = [0]*5; cs = [0]*5; rd = {}; cd = [0]*5
    ue_r = [0]*5; ue_c = [0]*5
    for rid, g in real.items():
        s = sims[rid]; oi = 1 if g["ours"] == "A" else 0
        for c in range(5):
            re_[c] += g["eat"][oi][c]; ce[c] += s["eat"][oi][c]; rs[c] += g["spl"][oi][c]; cs[c] += s["spl"][oi][c]
            ue_r[c] += g["eat"][1-oi][c]; ue_c[c] += s["eat"][1-oi][c]
            cd[c] += s["dth"][oi][c]
        for c in range(5): rd[c] = rd.get(c, 0) + g['dth'][oi][c]
    f = lambda v: " ".join(f"{x/n:6.1f}" for x in v)
    print("  opp eaten/century  real", f(re_), " clone", f(ce))
    print("  ours eaten/century real", f(ue_r), " sim  ", f(ue_c))
    print("  opp splits/century real", f(rs), " clone", f(cs))
    print("  opp deaths/game real", {k: round(v/n,1) for k, v in rd.items()}, " clone wall/self/body/h2h/noact", f(cd))

    import collections
    rz = collections.Counter(); czs = [0]*5
    for rid, g in real.items():
        s = sims[rid]; oi = 1 if g["ours"] == "A" else 0
        rz.update(g["cz"][oi]); czs = [a + b for a, b in zip(czs, s["cz"][oi])]
    print("  r0-199 opp deaths real", [(k, round(v/n,1)) for k, v in rz.most_common()])
    print("  r0-199 clone deaths wall/self/body/h2h/noact", [round(x/n,1) for x in czs])
