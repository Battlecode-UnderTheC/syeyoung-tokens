#!/bin/sh
# Every tagged variant: name, commit, date, operator, parents.
root=$(git -C "$(dirname "$0")" rev-parse --show-toplevel)
git -C "$root" for-each-ref --sort=creatordate refs/tags/variant/ \
  --format='%(refname:strip=3) %(*objectname:short) %(creatordate:short)' |
while read -r name sha date; do
  toml=$(git -C "$root" show "$sha:variants/$name/variant.toml" 2>/dev/null || true)
  op=$(echo "$toml" | sed -n 's/^operator = "\(.*\)"/\1/p')
  parents=$(echo "$toml" | sed -n 's/^parents = \[\(.*\)\]/\1/p')
  printf '%-28s %s %s %-9s parents: %s\n' "$name" "$sha" "$date" "$op" "${parents:-none}"
done
