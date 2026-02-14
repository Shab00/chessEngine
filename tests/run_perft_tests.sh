#!/usr/bin/env bash
set -eu

FENS=(
  "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
  "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1P/PPPBBPP1/R3K2R w KQkq - 0 1"
  # Add any EP/promo edge cases you have here
)

DEPTH=3

extract_nodes() {
  awk -F'Nodes: ' -v D="$DEPTH" '/Depth: '"$DEPTH"'/ {split($2,a," "); print a[1]}'
}

for fen in "${FENS[@]}"; do
  printf "\nFEN: %s\n" "$fen"
  N1=$(./build/perft "$fen" "$DEPTH" | extract_nodes || true)
  echo "perft before: $N1"
  ./build/engine_search "$fen" "$DEPTH"
  N2=$(./build/perft "$fen" "$DEPTH" | extract_nodes || true)
  echo "perft after:  $N2"
  if [ "$N1" = "$N2" ]; then
    echo "Result: OK"
  else
    echo "Result: MISMATCH"
  fi
done
