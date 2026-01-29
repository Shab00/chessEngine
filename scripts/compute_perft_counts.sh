#!/usr/bin/env bash
set -uo pipefail

mkdir -p tests
cp -n tests/perft_tests.txt tests/perft_tests.txt.bak || true

out="tests/perft_tests_added.txt"
errlog="tests/perft_tests_errors.txt"
: > "$out"
: > "$errlog"

while IFS= read -r fen; do
  [ -z "$fen" ] && continue
  for d in 1 2 3 4; do
    echo "Computing depth $d for: $fen"
    # capture both stdout and stderr and do not abort on non-zero exit
    outtxt=$(./build/perft_runner "$fen" "$d" 2>&1) || true
    nodes=$(printf '%s\n' "$outtxt" | awk -F'nodes=' '/nodes/ {print $2}' | tr -d '\r')
    if [ -n "$nodes" ]; then
      printf '%s %d %s\n' "$fen" "$d" "$nodes" >> "$out"
    else
      {
        printf 'FEN: %s depth: %d\n' "$fen" "$d"
        printf '%s\n\n' "$outtxt"
      } >> "$errlog"
      echo "  [ERROR] see $errlog for details"
    fi
  done
done < tests/new_perft_candidates.txt

echo "Done. Results in $out; failures (if any) in $errlog"
