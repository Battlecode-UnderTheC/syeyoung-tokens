#!/bin/sh
# clonedump.sh <clone> <outdir>: dump the clone's turns in all 45 server games (per-game maps)
cd /Users/syeyoung/Documents/postech/unswbattlecode/syeyoung-tokens.worktrees/<task>
mkdir -p $2
python3 - "$1" "$2" <<'PY'
import json, subprocess, sys
from concurrent.futures import ThreadPoolExecutor
clone, out = sys.argv[1], sys.argv[2]
real = json.load(open("build/scratch/real.json"))
BOT = {"10729": "gather-split-trade-90", "11400": "gather-split-trade-96"}
def run(item):
    rid, g = item
    ours = BOT[g["sub"]]
    a, b, t = (ours, clone, "1") if g["ours"] == "A" else (clone, ours, "0")
    with open(f"{out}/{rid}.jsonl", "w") as f:
        subprocess.run(["build/tools/dump", a, b, f"build/scratch/gamemaps/{rid}.map", g["seed"], t], stdout=f)
with ThreadPoolExecutor(2) as ex: list(ex.map(run, real.items()))
PY
