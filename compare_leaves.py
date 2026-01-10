# compare_leaves.py
import chess, collections

GRANDCHILD = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q2/PPPBBPpP/R4K1R w kq - 0 2"

# read engine leaves (produced earlier)
with open("eng_leaves.txt", "r") as f:
    eng = [line.strip() for line in f if line.strip()]

# build reference leaves using python-chess
b = chess.Board(GRANDCHILD)
ref = []
for mv in b.legal_moves:
    b.push(mv)
    ref.append(b.fen())
    b.pop()

eng_set = set(eng)
ref_set = set(ref)

print("Engine leaves total:", len(eng))
print("Reference leaves total:", len(ref))
print("Unique engine leaf FENs:", len(eng_set))
print("Unique reference leaf FENs:", len(ref_set))
print()

only_engine = sorted(eng_set - ref_set)
only_ref    = sorted(ref_set - eng_set)
both        = sorted(eng_set & ref_set)

print("Leaves only in engine (count={}):".format(len(only_engine)))
for fen in only_engine:
    print("E: " + fen)
print()
print("Leaves only in reference (count={}):".format(len(only_ref)))
for fen in only_ref:
    print("R: " + fen)
print()
print("Leaves in both (count={}):".format(len(both)))
for fen in both:
    print("B: " + fen)
