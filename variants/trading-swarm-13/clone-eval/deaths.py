import sys, json, os, collections
DXY = [(0, -1), (1, 0), (0, 1), (-1, 0)]
for d in sys.argv[1:]:
    c = collections.Counter(); n = collections.Counter()
    for fn in os.listdir(d):
        for line in open(os.path.join(d, fn)):
            t = json.loads(line)
            if not (50 <= t["r"] < 200): continue
            occ = set(map(tuple, t["occ"]))
            nb = sum(1 for s in DXY if s in occ)
            ctx = "pv" if t["pearls"] else "ripe" if any(b[2] <= 6 for b in t["beds"]) else "empty"
            a = t["act"][0]
            dens = len(t["ally"])
            for key in [f"L{min(t['L'],6)}", f"nbocc{nb}", ctx, f"act_{a}", f"allies{min(dens,4)}", f"enemies{min(len(t['enemy']),2)}"]:
                n[key] += 1; c[key] += t["died"]
    print(d, " ".join(f"{k}:{c[k]/n[k]:.3f}({n[k]//1000}k)" for k in sorted(n)))
