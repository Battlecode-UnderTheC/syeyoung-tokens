#!/usr/bin/env python3
"""Watch a replay as text: the map once (with kelp walls), then every round's events,
and an ASCII board every --every rounds.

  python3 watch.py <replay[.gz]> [--from R] [--to R] [--every K]

Board legend: our heads A-Z/0-9 by id (key printed), our bodies 'o'; opponent heads shown
the same way in the key with team 'opp', opponent bodies 'x'; pearls '*'; empty '.'.
Events: splits, eats (count per dragon), deaths (reason), multi-step moves.
"""
import argparse, os, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import decode

SYM = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnpqrsuvwyz0123456789"

def walls(g):
    out = []
    for y in range(g.H):
        row, under = [], []
        for x in range(g.W):
            e = g.step((x, y), "E"); s = g.step((x, y), "S")
            row.append(" " + ("|" if e is None else ("@" if e != ((x + 1) % g.W, y) else " ")))
            under.append(("-" if s is None else ("@" if s != (x, (y + 1) % g.H) else " ")) + " ")
        out.append("".join(row)); out.append("".join(under))
    return "\n".join(out)

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("replay"); ap.add_argument("--from", dest="lo", type=int, default=0)
    ap.add_argument("--to", type=int, default=500); ap.add_argument("--every", type=int, default=5); ap.add_argument("--us", default=None); ap.add_argument("--nowalls", action="store_true"); ap.add_argument("--ids", default=None, help="only events of these ids (+ queens, + deaths of length>=5)")
    a = ap.parse_args()
    d = decode.load(a.replay)
    ours = a.us or decode.our_team(d)
    g = decode.GameMap(d["map"])
    print(f"map {g.name} {g.W}x{g.H}  us=team {ours}  (walls: | east, - south, @ portal)")
    if not a.nowalls: print(walls(g))
    keep = None if a.ids is None else {0, 1} | {int(x) for x in a.ids.split(",") if x}
    sym = {}
    ev = []
    for kind, st, info in decode.walk(d):
        r = st.round
        if kind == "turn":
            if a.lo <= info["round"] <= a.to:
                s = []
                if len(info["path"]) > 1: s.append(f"moved {len(info['path'])} steps")
                if info["ate"]: s.append(f"ate {info['ate']}")
                if info["killed"]: s.append(f"killed {info['killed']}")
                if s and (keep is None or info['id'] in keep):
                    t = "us" if info["team"] == ours else "opp"
                    ev.append(f"  {t} {info['id']}:{info['len_after']} " + ", ".join(s))
        elif kind == "split" and a.lo <= info[0] <= a.to:
            rr, p, c, t, pl, cl = info
            if keep is None or p in keep: ev.append(f"  {'us' if t == ours else 'opp'} {p} split (len {pl}) -> child {c}:{cl}")
        elif kind == "death" and a.lo <= info[0] <= a.to:
            rr, i, t, why, ln, det = info
            if keep is None or i in keep or ln >= 5: ev.append(f"  {'us' if t == ours else 'opp'} {i}:{ln} DIED {why}{(':' + det) if det else ''}")
        elif kind == "round_end":
            if r < a.lo or r > a.to:
                ev = []; continue
            n = {t: sum(1 for i in st.alive if st.team[i] == t) for t in "AB"}
            L = {t: sum(len(st.body[i]) for i in st.alive if st.team[i] == t) for t in "AB"}
            o = "B" if ours == "A" else "A"
            qs = {st.team[q]: (f"Q{q}@{st.body[q][0]}:{len(st.body[q])}" if q in st.alive else "Qdead") for q in (0, 1)}
            print(f"r{r}: us n{n[ours]} T{L[ours]} {qs.get(ours,'')} | opp n{n[o]} T{L[o]} {qs.get(o,'')}")
            for e in ev: print(e)
            ev = []
            if (r - a.lo) % a.every == 0:
                grid = [["." for _ in range(g.W)] for _ in range(g.H)]
                for (x, y) in st.pearls: grid[y][x] = "*"
                key = []
                for i in sorted(st.alive):
                    b = list(st.body[i]); us = st.team[i] == ours
                    for (x, y) in b[1:]: grid[y][x] = "o" if us else "x"
                    c = sym.setdefault(i, SYM[len(sym) % len(SYM)])
                    x, y = b[0]; grid[y][x] = c
                    key.append(f"{c}={'us' if us else 'opp'}{i}{'(QUEEN)' if i < 2 else ''}:{len(b)}")
                print("\n".join("".join(row) for row in grid))
                print("  key: " + " ".join(key))

if __name__ == "__main__":
    main()
