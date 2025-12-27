#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
BIN="$ROOT_DIR/build/fen_roundtrip"
TESTS="$ROOT_DIR/tests/fen_tests.txt"

if [ ! -x "$BIN" ]; then
  echo "Building fen_roundtrip..."
  mkdir -p "$ROOT_DIR/build"
  gcc -Iinclude -std=c11 -Wall -Wextra -c tests/fen_roundtrip.c -o build/fen_roundtrip.o || exit 1
  gcc -Iinclude -std=c11 -Wall -Wextra -c src/position.c      -o build/position.o || exit 1
  gcc -Iinclude -std=c11 -Wall -Wextra -c src/position_fen.c  -o build/position_fen.o || exit 1
  gcc build/fen_roundtrip.o build/position.o build/position_fen.o -o build/fen_roundtrip || exit 1
fi

failures=0
while IFS= read -r line || [ -n "$line" ]; do
  line="${line%%#*}"
  line="${line#"${line%%[![:space:]]*}"}"
  line="${line%"${line##*[![:space:]]}"}"
  [ -z "$line" ] && continue

  echo -n "Test: $line ... "
  if "$BIN" "$line" >/dev/null 2>&1; then
    echo "OK"
  else
    echo "FAIL"
    failures=$((failures+1))
  fi
done < "$TESTS"

if [ $failures -ne 0 ]; then
  echo "$failures tests failed"
  exit 1
fi

echo "All tests passed"
exit 0
