#!/bin/sh
# Arena round-robin on the faithful sim: every entrant against every other, official maps x seeds x both seats.
#   scripts/arena.sh <round> <seeds, e.g. 1,2,3> <entrant>...        entrant = name=plugin.so  or  a variant name
# Writes ../syeyoung-tokens.league/arena/round-<round>/standings.md (and raw.txt).
set -e
round=$1; seeds=$2; shift 2
root=$(git -C "$(dirname "$0")" rev-parse --show-toplevel)
sim=${SIM:-$root/build/tools/sim}
out=$root/../syeyoung-tokens.league/arena/round-$round
mkdir -p "$out"
raw=$out/raw.txt; : > "$raw"
names=""; for e in "$@"; do names="$names ${e%%=*}"; done
spec() { for e in $ENTRANTS; do [ "${e%%=*}" = "$1" ] && { case $e in *=*) echo "${e#*=}";; *) echo "$e";; esac; return; }; done; }
ENTRANTS="$*"
set -- $names
while [ $# -gt 1 ]; do
  a=$1; shift
  for b in "$@"; do
    r=$("$sim" --a "$(spec "$a")" --b "$(spec "$b")" --maps "$root/maps/official" --threads "${THREADS:-3}" --seed "$seeds" 2>&1 | grep "W-D-L" | sed 's/.*: //')
    echo "$a|$b|$r" >> "$raw"
    echo "$a vs $b: $r"
  done
done
python3 - "$raw" "$out/standings.md" "$round" "$seeds" <<'EOF'
import sys, re, collections
raw, dest, rnd, seeds = sys.argv[1:]
pts = collections.Counter(); games = collections.Counter(); rows = []
for line in open(raw):
    a, b, r = line.rstrip('\n').split('|', 2)
    m = re.match(r'(\d+)-(\d+)-(\d+)', r)
    if not m: rows.append((a, b, 'ERROR ' + r)); continue
    w, d, l = map(int, m.groups())
    pts[a] += w + d / 2; pts[b] += l + d / 2; games[a] += w + d + l; games[b] += w + d + l
    rows.append((a, b, f'{w}-{d}-{l}'))
with open(dest, 'w') as f:
    f.write(f'# Arena round {rnd} (faithful sim, official maps, seeds {seeds}, both seats)\n\n| # | entrant | score | games | win % |\n|---|---|---|---|---|\n')
    for i, (n, p) in enumerate(sorted(pts.items(), key=lambda x: -x[1] / max(1, games[x[0]])), 1):
        f.write(f'| {i} | {n} | {p:g} | {games[n]} | {100 * p / max(1, games[n]):.1f} |\n')
    f.write('\n## Pairings (row entrant\'s W-D-L)\n\n| A | B | W-D-L |\n|---|---|---|\n')
    for a, b, r in rows: f.write(f'| {a} | {b} | {r} |\n')
print(open(dest).read())
EOF
