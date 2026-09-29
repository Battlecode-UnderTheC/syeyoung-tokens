# realdump.py <outdir>: per server game, the real opponent's turns (r < 200) in dump.cpp's format
import sys, os, json, bisect, collections
sys.path.insert(0, "/Users/syeyoung/Documents/postech/unswbattlecode/syeyoung-tokens.league/replays/league-2/analysis")
import decode
R = "/Users/syeyoung/Documents/postech/unswbattlecode/syeyoung-tokens.league/replays"
out = sys.argv[1]; os.makedirs(out, exist_ok=True)
DI = {"N": 0, "E": 1, "S": 2, "W": 3}
only = sys.argv[2:]
for line in open(R + "/server-seeds.tsv"):
    rid = line.split("\t")[0]
    if only and rid not in only: continue
    path = next(os.path.join(dp, rid + ".replay") for dp, _, fs in os.walk(R) if rid + ".replay" in fs and "league-" not in dp)
    d = decode.load(path); ours = decode.our_team(d); opp = "B" if ours == "A" else "A"
    g = decode.GameMap(d["map"]); W, H = g.W, g.H
    spawns = collections.defaultdict(list); rnd = -1; in_spawn = False
    for e in d["events"]:
        if e[0] == "roundStart": rnd = e[1]; in_spawn = True
        elif e[0] == "turnStart": in_spawn = False
        elif e[0] == "tileChange" and e[2] and in_spawn: spawns[e[1]].append(rnd)
    f = open(f"{out}/{rid}.jsonl", "w")
    cur = None
    for kind, st, info in decode.walk(d):
        if kind == "round_end" and info >= 199: break
        if kind == "pre" and st.team.get(info) == opp:
            i = info; b = st.body[i]; hx, hy = b[0]
            fc = -1
            if len(b) > 1:
                nx, ny = b[1]; ddx = (hx - nx) % W; ddy = (hy - ny) % H
                fc = 1 if ddx == 1 else 3 if ddx == W - 1 else 2 if ddy == 1 else 0 if ddy == H - 1 else -1
            owner = {}
            for j in st.alive:
                for k, t in enumerate(st.body[j]): owner[t] = (j, k == 0)
            pe, be, al, en, oc = [], [], [], [], []
            for dy in range(-3, 4):
                for dx in range(-3, 4):
                    t = ((hx + dx) % W, (hy + dy) % H)
                    if t in st.pearls: pe.append([dx, dy])
                    elif t in spawns:
                        s = spawns[t]; k = bisect.bisect_right(s, st.round)
                        if k < len(s): be.append([dx, dy, s[k] - st.round])
                    if t in owner:
                        j, head = owner[t]; oc.append([dx, dy])
                        if head and j != i: (al if st.team[j] == opp else en).append([dx, dy])
            occset = set(owner)
            DXY = [(0, -1), (1, 0), (0, 1), (-1, 0)]
            def room(d0):
                ex = g.edges.get(g.edge_id(hx, hy, "NESW"[d0]), (0, -1))[0]
                if ex == 1: return -1
                if ex == 2: return 20
                sx, sy = DXY[d0]
                if (sx, sy) in [(x[0], x[1]) for x in oc]: return -1
                seen = {(0, 0), (sx, sy)}; stack = [(sx, sy)]; n = 0; bonus = 0
                while stack:
                    a, b2 = stack.pop(); n += 1
                    for e in range(4):
                        typ = g.edges.get(g.edge_id((hx + a) % W, (hy + b2) % H, "NESW"[e]), (0, -1))[0]
                        if typ == 2: bonus = 20
                        if typ != 0: continue
                        m = (a + DXY[e][0], b2 + DXY[e][1])
                        if abs(m[0]) > 3 or abs(m[1]) > 3 or m in seen: continue
                        if ((hx + m[0]) % W, (hy + m[1]) % H) in occset: continue
                        seen.add(m); stack.append(m)
                return min(20, n + bonus)
            cur = dict(room=[room(k) for k in range(4)], r=st.round, id=i, L=len(b), f=fc, hx=hx, hy=hy, pearls=pe, beds=be, ally=al, enemy=en, occ=oc)
        elif kind == "turn" and cur is not None and info["id"] == cur["id"]:
            a = info["action"]
            cur["act"] = ["m", [DI[c] for c in a[1]]] if a and a[0] == "move" else ["s", a[1]] if a and a[0] == "split" else ["x"]
            cur["ate"] = info["ate"]; cur["died"] = 1 if info["died"] else 0
            f.write(json.dumps(cur) + "\n"); cur = None
    f.close()
    print(rid, file=sys.stderr)
