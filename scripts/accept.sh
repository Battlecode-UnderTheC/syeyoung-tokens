#!/bin/sh
# Merges a finished task into main (the task branch is kept for its history).
#   scripts/accept.sh <task-id>              (run in the main checkout)
set -e
task=$1
[ -n "$task" ] || { echo "usage: $0 <task-id>" >&2; exit 2; }
root=$(git -C "$(dirname "$0")" rev-parse --show-toplevel)
[ "$(git -C "$root" branch --show-current)" = main ] || { echo "run in the main checkout" >&2; exit 1; }
git -C "$root" merge --no-ff -q "task/$task" -m "accept task $task"
echo "merged task/$task into main"
