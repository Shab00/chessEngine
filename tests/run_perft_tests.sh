#!/usr/bin/env bash
set -euo pipefail

TESTS=(
  "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1|3|8902"
  "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1|3|97862"
)

extract_nodes() {
  grep -E "^Depth: $DEPTH" | awk '{for(i=1;i<=NF;i++) if ($i=="Nodes:") print $(i+1)}' | head -n1
}

failures=0

for test in "${TESTS[@]}"; do
  IFS='|' read -r FEN DEPTH EXPECTED <<< "$test"

  printf "\nTesting FEN: %s (depth %s, expect %s)\n" "$FEN" "$DEPTH" "$EXPECTED"

  NODES=$(
    ./build/perft "$FEN" "$DEPTH" | extract_nodes || echo "ERR"
  )

  if [[ "$NODES" == "ERR" ]] || [[ -z "$NODES" ]]; then
    echo "FAILED: Perft execution error (Nodes not found)"
    failures=$((failures+1))
    continue
  fi

  if [[ "$NODES" != "$EXPECTED" ]]; then
    echo "FAILED: got $NODES, expected $EXPECTED"
    failures=$((failures+1))
  else
    echo "PASS"
  fi
done

if [[ $failures -gt 0 ]]; then
  echo -e "\nFAILED: $failures test(s) failed."
  exit 1
else
  echo -e "\nAll perft tests passed!"
fi
