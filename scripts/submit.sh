#!/bin/sh
# Submits a variant from a task worktree: checks it, commits it, tags it variant/<name>.
#   scripts/submit.sh <variant-name>          (run inside the task's worktree)
# Refuses if anything outside variants/<name>/ changed, if the tag exists, or if the
# variant fails tools/check on the official maps.
set -e
name=$1
[ -n "$name" ] || { echo "usage: $0 <variant-name>" >&2; exit 2; }
top=$(git rev-parse --show-toplevel)
cd "$top"
dir="variants/$name"
{ [ -f "$dir/variant.toml" ] && [ -f "$dir/NOTES.md" ]; } || { echo "$dir needs variant.toml and NOTES.md" >&2; exit 1; }
grep -q "LEAGUE_VARIANT(" "$dir"/*.cpp || { echo "$dir has no LEAGUE_VARIANT(...) line" >&2; exit 1; }
# no environment variables in bots, not even behind a debug macro: the judge's sandbox is not our
# shell, and a knob read at runtime makes local results unreproducible (user rule, 2026-09-30)
if grep -rn -E "getenv|secure_getenv|environ\b" "$dir" --include='*.cpp' --include='*.hpp' --include='*.h' --include='*.cc' >/dev/null; then
  grep -rn -E "getenv|secure_getenv|environ\b" "$dir" --include='*.cpp' --include='*.hpp' --include='*.h' --include='*.cc' | head -5 >&2
  echo "environment variables are forbidden in variants: use compile-time constants" >&2
  exit 1
fi
if git rev-parse -q --verify "refs/tags/variant/$name" >/dev/null; then
  echo "variant/$name is already tagged: a changed strategy needs a new name" >&2
  exit 1
fi
outside=$( { git diff --name-only main...HEAD; git status --porcelain | cut -c4-; } | grep -v "^$dir/" | sort -u || true)
if [ -n "$outside" ]; then
  echo "changes outside $dir are not allowed:" >&2
  echo "$outside" >&2
  exit 1
fi
cmake -S . -B build >/dev/null
cmake --build build -j 4 --target "$name" check >/dev/null
if ! report=$(build/tools/check "$name" maps/official); then
  echo "$report" | tail -15
  echo "check failed: not submitted" >&2
  exit 1
fi
git add "$dir"
git commit -q -m "variant $name" -m "$(cat "$dir/variant.toml")" || true
git tag -a "variant/$name" -m "variant $name" -m "$(cat "$dir/variant.toml")" \
  -m "check on the official maps:
$(echo "$report" | tail -12)"
echo "tagged variant/$name at $(git rev-parse --short HEAD)"
