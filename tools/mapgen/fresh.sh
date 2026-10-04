#!/bin/sh
# A fresh generated map set for one evaluation: N maps spread evenly over every family.
#   tools/mapgen/fresh.sh N SEED OUTDIR
# Use a NEW seed each time (e.g. `date +%s`), so there is no fixed generated set to tune to.
# Writes OUTDIR/<family>/*.map (one folder per family, for per-family sim runs),
# OUTDIR/all/*.map (a copy of every map, for one `sim --maps OUTDIR/all` run) and
# OUTDIR/features.txt (mapgen.py --describe of every map).
set -e
[ $# -eq 3 ] || { echo "usage: $0 N SEED OUTDIR" >&2; exit 2; }
n=$1 seed=$2 out=$3
here=$(cd "$(dirname "$0")" && pwd)
gen="python3 $here/mapgen.py"
mkdir -p "$out"
tmp="$out/.gen"
rm -rf "$tmp"
$gen --family all --seed "$seed" --count "$n" --out "$tmp" > /dev/null
for f in $($gen --list); do
  set -- "$tmp/$f"-s*.map
  [ -e "$1" ] || continue
  mkdir -p "$out/$f"
  mv "$@" "$out/$f/"
done
rm -rf "$tmp"
rm -rf "$out/all"
mkdir -p "$out/all"
find "$out" -mindepth 2 -maxdepth 2 -name '*.map' ! -path "$out/all/*" -exec cp {} "$out/all/" \;
$gen --describe "$out/all" > "$out/features.txt"
echo "$(ls "$out/all" | wc -l | tr -d ' ') maps (seed $seed) in $out; features in $out/features.txt"
