#!/bin/sh
# A multi-seed duel in the judge's sandbox (per-match pearl seeds, unlike `sim`, which always uses the
# engine's one fixed seed and so overfits it).
#   scripts/sbx-duel.sh <variant-a> <variant-b> [seeds (default "11 12")] [maps dir (default maps/official)] [jobs (default 3)]
# Builds both as unswbc bots under build/sbx-bots/, plays every map x seed x both seats, prints each
# result and A's W-D-L.
set -e
a=$1; b=$2; seeds=${3:-"11 12"}; maps=${4:-maps/official}; jobs=${5:-3}
[ -n "$a" ] && [ -n "$b" ] || { echo "usage: $0 <variant-a> <variant-b> [seeds] [maps] [jobs]" >&2; exit 2; }
root=$(git -C "$(dirname "$0")" rev-parse --show-toplevel)
out=$root/build/sbx-bots
mkdir -p "$out"
for v in "$a" "$b"; do
  [ -d "$out/$v" ] || "$root/scripts/make-bot.sh" "$v" "$out/$v" > /dev/null
done
log=$(mktemp)
for f in "$maps"/*.map; do
  for s in $seeds; do
    echo "$f $s $a $b"
    echo "$f $s $b $a"
  done
done | xargs -P "$jobs" -n 4 sh -c '
  r=$(unswbc run "$0" "'"$out"'/$2" "'"$out"'/$3" --sandbox --no-replay --seed "$1" 2>&1 | grep -a -E "wins|draw" | tail -1 | sed "s/\x1b\[[0-9;]*m//g")
  echo "$(basename "$0" .map) seed $1 A=$2 B=$3: $r"' > "$log"
sort "$log"
awk -v a="$a" '
  { split($0, p, "A="); split(p[2], q, " "); A = q[1]; split($0, p2, "B="); split(p2[2], q2, ":"); B = q2[1] }
  /team A wins/ { if (A == a) w++; else l++ }
  /team B wins/ { if (B == a) w++; else l++ }
  /draw/ { d++ }
  END { printf "%s vs other: %d-%d-%d (W-D-L) over %d games\n", a, w, d, l, w + d + l }' "$log"
rm -f "$log"
