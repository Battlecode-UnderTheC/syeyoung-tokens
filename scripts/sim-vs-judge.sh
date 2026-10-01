#!/bin/sh
# Checks that `sim` reproduces the judge (unswbc 1.2.3): plays every map from both sides at each
# pearl seed in build/tools/sim and in the judge's sandbox (`unswbc run --sandbox --seed`), and
# compares each game's winner, length and verdict (what decided it: elimination, or at round 500
# the longer queen, the longest dragon, the total length, with the deciding numbers).
#   scripts/sim-vs-judge.sh <variant-a> <variant-b> [seeds S[,S...] (default 1592614637)] [maps dir (default maps/official)] [jobs (default 2)]
# A variant is a name under variants/ or a path to a variant directory (another worktree's).
# Both sides are built from the same flat sources (scripts/make-bot.sh, this tree's shared layer):
# the sandbox bot in build/sbx-bots/<name>, and sim's plugin from those very files in
# build/sbx-plugins/lib<name>.so, so sim and judge run identical bot code.
# Needs the tools built (cmake --build build --target sim). Any mismatch means sim no longer is
# the judge: a new unswbc engine, a bot that reads the clock or random_get, or one over the
# sandbox's limits.
set -e
a=$1; b=$2; seeds=${3:-1592614637}; maps=${4:-maps/official}; jobs=${5:-2}
[ -n "$a" ] && [ -n "$b" ] || { echo "usage: $0 <variant-a> <variant-b> [seeds] [maps] [jobs]" >&2; exit 2; }
root=$(git -C "$(dirname "$0")" rev-parse --show-toplevel)
out=$root/build/sbx-bots
plugins=$root/build/sbx-plugins
mkdir -p "$out" "$plugins"
cxx=${CXX:-c++}
for v in "$a" "$b"; do
  n=$(basename "$v")
  "$root/scripts/make-bot.sh" "$v" "$out/$n" > /dev/null
  srcs=$(ls "$out/$n"/*.cpp | grep -v '/bot_main\.cpp$')
  # shellcheck disable=SC2086
  $cxx -std=c++20 -O2 -fPIC -shared -fvisibility=hidden -fvisibility-inlines-hidden -w -I"$out/$n" $srcs \
    -o "$plugins/lib$n.so"
done
a=$(basename "$a"); b=$(basename "$b")
tmp=$(mktemp -d)
# sim: "  <map> <a> as A|B: WIN|loss|draw after R rounds (by D; queen x-y, longest x-y, total x-y)..."
#   -> "<map>/<A-side>/<seed> <winner> <rounds> <verdict as the judge words it>"
for seed in $(echo "$seeds" | tr ',' ' '); do
  "$root/build/tools/sim" --a "$plugins/lib$a.so" --b "$plugins/lib$b.so" --maps "$maps" --seed "$seed" --threads "$jobs" |
    awk -v a="$a" -v b="$b" -v seed="$seed" '$3 == "as" {
        seat = substr($4, 1, 1); res = $5; dec = substr($10, 1, length($10) - 1)
        split($12, q, "-"); split($14, l, "-"); split($16, t, "-")
        sub(/,$/, "", q[2]); sub(/,$/, "", l[2]); sub(/\).*$/, "", t[2])
        # [1] = a, [2] = b; to team A/B
        if (seat == "A") { qa = q[1]; qb = q[2]; la = l[1]; lb = l[2]; ta = t[1]; tb = t[2] }
        else             { qa = q[2]; qb = q[1]; la = l[2]; lb = l[1]; ta = t[2]; tb = t[1] }
        w = (res == "draw") ? "draw" : ((res == "WIN") == (seat == "A") ? "A" : "B")
        if (dec == "elimination") v = "by elimination"
        else if (w == "draw" && $7 + 1 < 500) v = "both teams eliminated"
        else if (w == "draw") v = sprintf("equal length: queen %d, longest %d, total %d each", qa, la, ta)
        else {
          label = dec == "queen" ? "longer queen" : (dec == "longest" ? "longest dragon" : "total length")
          x = dec == "queen" ? qa : (dec == "longest" ? la : ta); y = dec == "queen" ? qb : (dec == "longest" ? lb : tb)
          v = (w == "A") ? sprintf("%s, %d to %d", label, x, y) : sprintf("%s, %d to %d", label, y, x)
        }
        print $1 "/" (seat == "A" ? a : b) "/" seed, w, $7, v }'
done | sort > "$tmp/sim"
for seed in $(echo "$seeds" | tr ',' ' '); do
  for f in "$maps"/*.map; do echo "$f $a $b $seed"; echo "$f $b $a $seed"; done
done | xargs -P "$jobs" -n 4 sh -c '
  r=$(unswbc run "$0" "'"$out"'/$1" "'"$out"'/$2" --sandbox --no-replay --seed "$3" 2>&1 | sed "s/\x1b\[[0-9;]*m//g" |
      grep -a -E "wins after|draw after" | tail -1)
  w=$(echo "$r" | awk "/team A wins/ {print \"A\"} /team B wins/ {print \"B\"} /^draw/ {print \"draw\"}")
  n=$(echo "$r" | sed -E "s/.*after ([0-9]+) rounds.*/\1/")
  v=$(echo "$r" | sed -E "s/.*rounds \(([^)]*)\).*/\1/")
  echo "$(basename "$0" .map)/$1/$3 $w $((n - 1)) $v"' | sort > "$tmp/judge"
awk 'NR == FNR { k = $1; $1 = ""; s[k] = substr($0, 2); next }
     { k = $1; $1 = ""; j = substr($0, 2); n++; ok = s[k] == j; same += ok
       split(s[k], x, " "); split(j, y, " "); win += !ok && x[1] == y[1]
       printf "%-48s sim %-4s %3s | judge %-4s %3s  %s%s\n", k, x[1], x[2], y[1], y[2], j, ok ? "" : "   MISMATCH (sim: " s[k] ")" }
     END { printf "same winner, length and verdict in %d of %d games (%d more with the same winner only), seeds '"$seeds"'\n", same, n, win }' \
  "$tmp/sim" "$tmp/judge"
rm -rf "$tmp"
