#!/bin/sh
# Builds a variant as a self-contained unswbc C++ project (one flat directory: the
# toolkit compiles without include paths).
#   scripts/make-bot.sh <variant> <out dir>
#   then: unswbc run <map> <out dir> <other bot> [--sandbox]  /  unswbc submit <out dir>
# <variant> is a name under variants/, or a path to a variant directory (another
# worktree's, say), built against this tree's shared layer.
set -e
name=$1
out=$2
{ [ -n "$name" ] && [ -n "$out" ]; } || { echo "usage: $0 <variant> <out dir>" >&2; exit 2; }
root=$(git -C "$(dirname "$0")" rev-parse --show-toplevel)
case $name in
  */*) src=$(cd "$name" && pwd); name=$(basename "$src") ;;
  *) src=$root/variants/$name ;;
esac
[ -d "$src" ] || { echo "no variant $name" >&2; exit 1; }
rm -rf "$out"
mkdir -p "$out"
cp "$root"/shared/engine/dragons/* "$root"/shared/protocol/* "$root"/shared/bot/* "$root"/shared/lib/* "$out/"
for f in "$src/"*.hpp "$src/"*.cpp; do [ -f "$f" ] && cp "$f" "$out/"; done
sed -i.bak 's#"dragons/\([a-z_]*\.hpp\)"#"\1"#' "$out"/*.hpp "$out"/*.cpp
rm -f "$out"/*.bak
cat > "$out/bot.toml" <<TOML
# only translation units belong in include; headers are pulled in by #include
[project]
language = "cpp"
include = ["*.cpp", "*.hpp"]
TOML
echo "$name as an unswbc bot in $out"
