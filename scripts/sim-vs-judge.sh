#!/bin/sh
# Checks that `sim` reproduces the judge: plays every map from both sides at one pearl seed in
# build/tools/sim and in the judge's sandbox (`unswbc run --sandbox --seed`), and compares each
# game's winner and length.
#   scripts/sim-vs-judge.sh <variant-a> <variant-b> [seed (default 1592614637)] [maps dir (default maps/official)] [jobs (default 2)]
# Needs a built tree (cmake --build build). Any mismatch means sim no longer is the judge: a new
# unswbc engine, a bot that reads the clock or random_get, or one over the sandbox's limits.
set -e
a=$1; b=$2; seed=${3:-1592614637}; maps=${4:-maps/official}; jobs=${5:-2}
[ -n "$a" ] && [ -n "$b" ] || { echo "usage: $0 <variant-a> <variant-b> [seed] [maps] [jobs]" >&2; exit 2; }
root=$(git -C "$(dirname "$0")" rev-parse --show-toplevel)
out=$root/build/sbx-bots
mkdir -p "$out"
for v in "$a" "$b"; do "$root/scripts/make-bot.sh" "$v" "$out/$v" > /dev/null; done
tmp=$(mktemp -d)
# sim: "  <map> <a> as A|B: WIN|loss|draw after R rounds ..." -> "<map> <A-side> <winner side> <rounds>"
"$root/build/tools/sim" --a "$a" --b "$b" --maps "$maps" --seed "$seed" --threads "$jobs" |
  awk -v a="$a" -v b="$b" '$3 == "as" {
      seat = substr($4, 1, 1); w = ($5 == "draw") ? "draw" : (($5 == "WIN") == (seat == "A") ? "A" : "B")
      print $1, (seat == "A" ? a : b), w, $7 }' | sort > "$tmp/sim"
for f in "$maps"/*.map; do echo "$f $a $b"; echo "$f $b $a"; done | xargs -P "$jobs" -n 3 sh -c '
  r=$(unswbc run "$0" "'"$out"'/$1" "'"$out"'/$2" --sandbox --no-replay --seed '"$seed"' 2>&1 | sed "s/\x1b\[[0-9;]*m//g" |
      grep -a -E "wins after|draw after" | tail -1)
  w=$(echo "$r" | awk "/team A wins/ {print \"A\"} /team B wins/ {print \"B\"} /^draw/ {print \"draw\"}")
  n=$(echo "$r" | sed -E "s/.*after ([0-9]+) rounds.*/\1/")
  echo "$(basename "$0" .map) $1 $w $((n - 1))"' | sort > "$tmp/judge"
awk 'NR == FNR { s[$1 "/" $2] = $3 " " $4; next }
     { k = $1 "/" $2; n++; ok = s[k] == $3 " " $4; same += ok; split(s[k], x, " "); win += !ok && x[1] == $3
       printf "%-40s sim %-4s %3s | judge %-4s %3s%s\n", k, x[1], x[2], $3, $4, ok ? "" : "   MISMATCH" }
     END { printf "same winner and length in %d of %d games (%d more with the same winner only), seed %s\n", same, n, win, "'"$seed"'" }' \
  "$tmp/sim" "$tmp/judge"
rm -rf "$tmp"
