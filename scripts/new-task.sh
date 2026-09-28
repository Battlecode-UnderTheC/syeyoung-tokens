#!/bin/sh
# Starts an agent task: branch task/<task-id> off main, in its own worktree.
#   scripts/new-task.sh <task-id> [<new-variant> [--from <parent-variant> [--from <second-parent>]]]
# With a new variant name, creates variants/<new-variant>/ (copied from the first parent if
# given) with variant.toml and a NOTES.md to fill in.
set -e
root=$(git -C "$(dirname "$0")" rev-parse --show-toplevel)
task=$1
name=$2
if [ $# -ge 2 ]; then shift 2; else shift $#; fi
parents=""
while [ $# -gt 0 ]; do
  case $1 in
    --from) parents="$parents $2"; shift 2 ;;
    *) echo "unknown argument $1" >&2; exit 2 ;;
  esac
done
[ -n "$task" ] || { echo "usage: $0 <task-id> [<new-variant> [--from <parent>]...]" >&2; exit 2; }
dir="$root/../$(basename "$root").worktrees/$task"
if [ -n "$name" ] && git -C "$root" rev-parse -q --verify "refs/tags/variant/$name" >/dev/null; then
  echo "variant $name already exists: pick a new name" >&2
  exit 1
fi
git -C "$root" worktree add -q -b "task/$task" "$dir" main
if [ -n "$name" ]; then
  first=$(echo $parents | awk '{print $1}')
  if [ -n "$first" ]; then cp -R "$dir/variants/$first" "$dir/variants/$name"; else mkdir -p "$dir/variants/$name"; fi
  list=$(for p in $parents; do printf '"%s", ' "$p"; done | sed 's/, $//')
  op=fresh
  [ -n "$parents" ] && op=mutate
  [ "$(echo $parents | wc -w)" -ge 2 ] && op=crossover
  cat > "$dir/variants/$name/variant.toml" <<TOML
name = "$name"
parents = [$list]
operator = "$op"
archetype = ""
author = ""
created = "$(date +%F)"
task = "$task"
TOML
  cat > "$dir/variants/$name/NOTES.md" <<MD
# $name

## Idea

## What changed from the parents, and why

## What should improve
MD
fi
echo "task $task: $dir (branch task/$task)"
