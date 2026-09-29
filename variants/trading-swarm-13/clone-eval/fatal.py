import sys, json, subprocess, collections
from concurrent.futures import ThreadPoolExecutor
sys.argv += [""]
exec(open("build/scratch/cmp.py").read().split("def run")[0].replace("clone = sys.argv[1]", "clone = sys.argv[1]"))
def run(item):
    rid, g = item
    ours = BOT[g["sub"]]
    a, b = (ours, clone) if g["ours"] == "A" else (clone, ours)
    j = json.loads(subprocess.run([B + "/tools/match", a, b, B + "/scratch/gamemaps/" + rid + ".map", g["seed"]], capture_output=True, text=True).stdout)
    oi = 1 if g["ours"] == "A" else 0
    return j["fatal"][oi], j["reasons"][oi], j["deaths"][oi]
F = collections.Counter(); Rr = collections.Counter(); D = collections.Counter()
with ThreadPoolExecutor(2) as ex:
    for f, r, d in ex.map(run, real.items()):
        F.update(f); Rr.update(r); D.update(d)
print("fatal", [(k, round(v/45,1)) for k, v in F.most_common()])
print("deaths", [(k, round(v/45,1)) for k, v in D.most_common()])
print("reasons", [(k, round(v/45)) for k, v in Rr.most_common()])
