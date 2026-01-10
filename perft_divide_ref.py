# perft_divide_ref.py
# Usage: python3 perft_divide_ref.py "<FEN>" <depth>
import sys
import chess
sys.setrecursionlimit(10000)

if len(sys.argv) < 3:
    print("Usage: python3 perft_divide_ref.py \"<FEN>\" <depth>")
    sys.exit(2)

fen = sys.argv[1]
depth = int(sys.argv[2])

board = chess.Board(fen)

def perft(b, d):
    if d == 0:
        return 1
    nodes = 0
    for mv in list(b.legal_moves):
        b.push(mv)
        nodes += perft(b, d-1)
        b.pop()
    return nodes

moves = list(board.legal_moves)
total = 0
print(f"Reference perft divide for depth={depth}, moves={len(moves)}")
for i, mv in enumerate(moves, start=1):
    board.push(mv)
    nodes = perft(board, depth - 1)
    board.pop()
    print(f"{i:2d}: {mv.uci()} -> {nodes}")
    total += nodes
print(f"Total nodes: {total}")
