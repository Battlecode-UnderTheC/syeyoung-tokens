# feat.py <dir> [<dir> ...]: behaviour features of per-turn records (realdump.py / dump.cpp format)
import sys, json, os, collections
DXY = [(0, -1), (1, 0), (0, 1), (-1, 0)]
real = json.load(open("/Users/syeyoung/Documents/postech/unswbattlecode/syeyoung-tokens.worktrees/<task>/build/scratch/real.json"))
def size(rid):
    for l in open(f"/Users/syeyoung/Documents/postech/unswbattlecode/syeyoung-tokens.worktrees/<task>/build/scratch/gamemaps/{rid}.map"):
        if l.startswith("MAP "): return tuple(map(int, l.split()[1:3]))
def md(a, b): return abs(a[0] - b[0]) + abs(a[1] - b[1])
def feats(dirs, games=None, rmin=0, rmax=200):
    c = collections.Counter()
    for fn in sorted(os.listdir(dirs)):
        rid = fn[:-6]
        if games and real[rid]["map"] not in games: continue
        W, H = size(rid)
        for line in open(os.path.join(dirs, fn)):
            t = json.loads(line)
            if not (rmin <= t["r"] < rmax): continue
            c["turns"] += 1; c["ate"] += t["ate"]; c["died"] += t["died"]
            a = t["act"]
            if a[0] == "s":
                c["split"] += 1; c[f"splitL{min(t['L'],6)}"] += 1
            if t["L"] >= 4: c[f"turnsL{min(t['L'],6)}"] += 1
            if a[0] == "x": c["resign"] += 1
            if a[0] != "m" or not a[1]: continue
            if len(a[1]) > 1: c["lunge"] += 1
            d0 = a[1][0]; step = DXY[d0]
            c["moves"] += 1
            if d0 == t["f"]: c["straight"] += 1
            if t["pearls"]:
                c["pv"] += 1
                near = min(t["pearls"], key=lambda p: md(p, (0, 0)))
                if md(near, step) < md(near, (0, 0)): c["pv_toward"] += 1
                c["pv_dist"] += md(near, (0, 0))
            else:
                ripe = [b for b in t["beds"] if b[2] <= 6]
                if ripe:
                    c["ripe"] += 1
                    nb = min(ripe, key=lambda b: md(b, (0, 0)))
                    if md(nb, step) < md(nb, (0, 0)): c["ripe_toward"] += 1
                    c["ripe_dist"] += md(nb, (0, 0))
                else:
                    c["empty"] += 1
                    # toward the map centre?
                    cx = ((W // 2 - t["hx"] + W // 2) % W) - W // 2; cy = ((H // 2 - t["hy"] + H // 2) % H) - H // 2
                    if abs(cx) + abs(cy) > 4:
                        c["empty_far"] += 1
                        if step[0] * cx > 0 or step[1] * cy > 0: c["empty_centre"] += 1
                    if d0 == t["f"]: c["empty_straight"] += 1
                    if t["ally"]:
                        c["empty_ally"] += 1
                        na = min(t["ally"], key=lambda p: md(p, (0, 0)))
                        if md(na, step) > md(na, (0, 0)): c["empty_away_ally"] += 1
            if t["enemy"]:
                c["ev"] += 1
    return c
def show(name, c):
    r = lambda a, b: c[a] / max(1, c[b])
    print(f"{name:10s} turns {c['turns']:6d} eat/turn {r('ate','turns'):.3f} die/turn {r('died','turns'):.3f} | pearl-in-view {r('pv','moves'):.2f} toward {r('pv_toward','pv'):.2f} dist {r('pv_dist','pv'):.2f}"
          f" | ripe-bed-only {r('ripe','moves'):.2f} toward {r('ripe_toward','ripe'):.2f} | empty {r('empty','moves'):.2f} centre {r('empty_centre','empty_far'):.2f} straight {r('empty_straight','empty'):.2f} away-ally {r('empty_away_ally','empty_ally'):.2f}"
          f" | split/turn L4 {r('splitL4','turnsL4'):.2f} L5 {r('splitL5','turnsL5'):.2f} L6+ {r('splitL6','turnsL6'):.2f} lunge {r('lunge','moves'):.3f}")
maps = None
args = sys.argv[1:]
if args and args[0].startswith("--map="): maps = [args[0][6:]]; args = args[1:]
for rng in [(0, 50), (50, 200)]:
    print(f"rounds {rng}")
    for d in args: show(os.path.basename(d.rstrip('/')), feats(d, maps, *rng))
