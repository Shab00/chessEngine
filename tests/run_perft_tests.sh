set -euo pipefail

# All expected values verified against Stockfish/reference perft tables.
TESTS=(
  # --- Startpos ---
  "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1|1|20"
  "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1|2|400"
  "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1|3|8902"
  "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1|4|197281"

  # --- Kiwipete (Position 2) ---
  "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1|1|48"
  "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1|2|2039"
  "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1|3|97862"
  "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1|4|4085603"

  # --- Position 3 (CPW) ---
  "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1|1|14"
  "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1|2|191"
  "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1|3|2812"
  "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1|4|43238"

  # --- Position 4 (CPW) ---
  "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1|1|6"
  "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1|2|264"
  "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1|3|9467"

  # --- Position 5 (CPW) ---
  "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8|1|44"
  "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8|2|1486"
  "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8|3|62379"

  # --- Position 6 (CPW) ---
  "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10|1|46"
  "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10|2|2079"
  "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10|3|89890"

  # --- En passant specific ---
  "8/8/8/8/1k1PpN1R/8/8/4K3 b - d3 0 1|1|9"
  "8/8/8/8/1k1PpN1R/8/8/4K3 b - d3 0 1|2|193"

  # --- Castling rights ---
  "r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1|1|26"
  "r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1|2|568"
  "r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1|3|13744"

  # --- Promotion ---
  "n1n5/PPPk4/8/8/8/8/4Kppp/5N1N b - - 0 1|1|24"
  "n1n5/PPPk4/8/8/8/8/4Kppp/5N1N b - - 0 1|2|496"
  "n1n5/PPPk4/8/8/8/8/4Kppp/5N1N b - - 0 1|3|9483"
)

extract_nodes() {
  local depth="$1"
  grep -E "^Depth: ${depth} " | awk '{for(i=1;i<=NF;i++) if ($i=="Nodes:") print $(i+1)}' | head -n1
}

failures=0
passes=0

for test in "${TESTS[@]}"; do
  IFS='|' read -r FEN DEPTH EXPECTED <<< "$test"
  printf "depth=%-2s expect=%-10s FEN=%s ... " "$DEPTH" "$EXPECTED" "$FEN"

  OUTPUT=$(./build/perft "$FEN" "$DEPTH" 2>&1)
  if echo "$OUTPUT" | grep -q "position_from_fen failed"; then
    echo "FAILED (FEN parse error)"
    failures=$((failures+1))
    continue
  fi

  NODES=$(echo "$OUTPUT" | grep -E "^Depth: ${DEPTH} " | awk '{for(i=1;i<=NF;i++) if ($i=="Nodes:") print $(i+1)}' | head -n1)

  if [[ -z "$NODES" ]]; then
    echo "FAILED (nodes not found in output)"
    failures=$((failures+1))
    continue
  fi

  if [[ "$NODES" != "$EXPECTED" ]]; then
    echo "FAILED (got $NODES)"
    failures=$((failures+1))
  else
    echo "PASS"
    passes=$((passes+1))
  fi
done

echo ""
echo "Results: $passes passed, $failures failed."
if [[ $failures -gt 0 ]]; then
  exit 1
fi
