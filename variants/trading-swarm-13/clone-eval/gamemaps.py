# per-game map files: the official map with the server replay's DRAGON lines (the server
# shuffles which team gets which spawn, and its Prisoners Dilemma sometimes has 10 dragons)
import sys, os
sys.path.insert(0, "/Users/syeyoung/Documents/postech/unswbattlecode/syeyoung-tokens.league/replays/league-2/analysis")
import decode
R = "/Users/syeyoung/Documents/postech/unswbattlecode/syeyoung-tokens.league/replays"
B = os.path.dirname(os.path.abspath(__file__))
MF = {"Default": "default", "Queen Of Spades": "queen_of_spades", "Schooltime": "schooltime", "Trophy": "trophy",
      "Devil": "devil", "Trauma": "trauma", "Prisoners Dilemma": "dilemma", "Autarky": "autarky",
      "Portals": "portals", "Slithery Fight": "slithery_fight"}
os.makedirs(B + "/gamemaps", exist_ok=True)
for line in open(R + "/server-seeds.tsv"):
    rid, mp = line.split("\t")[:2]
    path = next(os.path.join(dp, rid + ".replay") for dp, _, fs in os.walk(R) if rid + ".replay" in fs and "league-" not in dp)
    rep = decode.load(path)["map"].splitlines()
    drag = [l for l in rep if l.startswith(("DRAGON", "SNAKE"))]
    f = open(B + "/../../maps/official/" + MF[mp] + ".map").read().splitlines()
    out = []; done = False
    for l in f:
        if l.startswith(("DRAGON", "SNAKE")):
            if not done: out += drag; done = True
            continue
        out.append(l)
    assert done
    open(f"{B}/gamemaps/{rid}.map", "w").write("\n".join(out) + "\n")
