#!/bin/sh
# multi.sh <label>: build trading-swarm-13 with rng salts 0-3, run cmp.py on each, print the mean
cd /Users/syeyoung/Documents/postech/unswbattlecode/syeyoung-tokens.worktrees/<task>
V=variants/trading-swarm-13/swarm5.cpp
mkdir -p build/scratch/salted build/scratch/multi
for s in 0 1 2 3; do
  sleep 1;   sed -i '' "s/inline constexpr uint32_t kSalt = [0-9]*;/inline constexpr uint32_t kSalt = $s;/" $V
  cmake --build build -j 2 --target trading-swarm-13 2>&1 | grep " error"
  cp build/variants/libtrading-swarm-13.so build/scratch/salted/libts13-$1-$s.so
done
sed -i '' "s/inline constexpr uint32_t kSalt = [0-9]*;/inline constexpr uint32_t kSalt = 0;/" $V
for s in 0 1 2 3; do python3 build/scratch/cmp.py $PWD/build/scratch/salted/libts13-$1-$s.so -v > build/scratch/multi/$1-$s.txt; done
python3 - $1 <<'PY'
import sys, re, statistics as S
lab = sys.argv[1]; rows = []
for s in range(4):
    t = open(f"build/scratch/multi/{lab}-{s}.txt").read()
    ag = int(re.search(r"agreement (\d+)/45", t).group(1)); w = int(re.search(r"clone (\d+)-\d+ \(opp", t).group(1))
    cnt = [float(re.search(rf"r{k} +mean.*?clone +([0-9.]+)/", t).group(1)) for k in (100, 200, 300)]
    lg = float(re.search(r"rfinal +mean.*?clone +[0-9.]+/ *([0-9.]+)/", t).group(1))
    rows.append([ag, w] + cnt + [lg])
m = [S.mean(c) for c in zip(*rows)]; sd = [S.pstdev(c) for c in zip(*rows)]
print(f"{lab}: agreement {m[0]:.1f}±{sd[0]:.1f}/45  clone wins {m[1]:.1f}  count r100/200/300 {m[2]:.1f}/{m[3]:.1f}/{m[4]:.1f}  final longest {m[5]:.1f}   per salt {[r[0] for r in rows]}")
PY
