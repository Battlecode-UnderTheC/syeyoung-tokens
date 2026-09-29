import sys, glob, collections
sys.path.insert(0, "/Users/syeyoung/Documents/postech/unswbattlecode/syeyoung-tokens.league/replays/league-1/analysis")
import decode

def analyze(path):
    d = decode.load(path)
    oppteam = "A" if d["botA"] == "10729" else "B"
    ourteam = "B" if oppteam == "A" else "A"
    splits = []  # (round, parent_len_before, team)
    counts = collections.defaultdict(dict)  # round -> team -> count
    eaten = collections.defaultdict(int)  # team -> total eaten
    eaten_at = {}
    deadend_enters = 0
    deadend_splits = 0
    for kind, st, rec in decode.walk(d):
        if kind == "split":
            rnd, p, c, t, before, clen = rec
            splits.append((rnd, before, t))
        if kind == "round_end":
            rnd = rec
            counts[rnd]["A"] = st.standing("A")
            counts[rnd]["B"] = st.standing("B")
        if kind == "turn":
            t = rec["team"]
            eaten[t] += rec["ate"]
            if rnd:=rec["round"]:
                pass
        if kind == "death":
            pass
    return d, oppteam, ourteam, splits, counts, eaten

for f in sorted(glob.glob("/Users/syeyoung/Documents/postech/unswbattlecode/syeyoung-tokens.league/replays/M547322/*.replay") +
                 glob.glob("/Users/syeyoung/Documents/postech/unswbattlecode/syeyoung-tokens.league/replays/M561834/*.replay") +
                 glob.glob("/Users/syeyoung/Documents/postech/unswbattlecode/syeyoung-tokens.league/replays/M572141/*.replay")):
    try:
        d, oppteam, ourteam, splits, counts, eaten = analyze(f)
    except Exception as e:
        print(f, "ERR", e)
        continue
    opp_splits = [s for s in splits if s[2] == oppteam]
    lens = collections.Counter(s[1] for s in opp_splits)
    r50 = counts.get(50, {})
    r100 = counts.get(100, {})
    mapname = [l for l in d["map"].splitlines() if l.startswith("MAP_NAME")]
    mapname = mapname[0] if mapname else "?"
    print(f.split("/")[-1], mapname, "winner", d["result"]["winner"], "oppteam", oppteam,
          "opp_split_lens", dict(lens), "n_opp_splits", len(opp_splits),
          "eaten opp/us", eaten.get(oppteam,0), eaten.get(ourteam,0),
          "r50 opp/us count", r50.get(oppteam,{}).get("count"), r50.get(ourteam,{}).get("count"),
          "r100 opp/us count", r100.get(oppteam,{}).get("count"), r100.get(ourteam,{}).get("count"))
