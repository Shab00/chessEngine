set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BIN="$ROOT/build/perft"
TESTS="$ROOT/tests/perft_tests.txt"

if [ ! -x "$BIN" ]; then
  echo "Building perft binary..."
  mkdir -p "$ROOT/build"
  gcc -Iinclude -std=c11 -Wall -Wextra -O2 src/position.c src/position_fen.c src/movegen.c tests/perft.c -o "$BIN" || exit 1
fi

failures=0
while IFS= read -r line || [ -n "$line" ]; do
  line="${line%%#*}"
  line="${line#"${line%%[![:space:]]*}"}"
  line="${line%"${line##*[![:space:]]}"}"
  [ -z "$line" ] && continue

  IFS=$'\t' read -r fen depth expected <<< "$line"
  echo -n "Perft: depth=$depth ... "
  if "$BIN" "$fen" "$depth" "$expected"; then
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

echo "All perft tests passed"
exit 0
