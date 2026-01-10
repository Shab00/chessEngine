#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "position.h"
#include "movegen.h"

static int coord_to_sq(const char *s) {
    if (!s || strlen(s) < 2) return POS_NO_SQUARE;
    int file = s[0] - 'a';
    int rank = s[1] - '1';
    if (file < 0 || file > 7 || rank < 0 || rank > 7) return POS_NO_SQUARE;
    return SQ_INDEX(file, rank);
}

static int promo_from_char(char c) {
    if (!c) return 0;
    if (c == 'q' || c == 'Q') return PIECE_QUEEN;
    if (c == 'r' || c == 'R') return PIECE_ROOK;
    if (c == 'b' || c == 'B') return PIECE_BISHOP;
    if (c == 'n' || c == 'N') return PIECE_KNIGHT;
    return 0;
}

int main(int argc, char **argv) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <FEN> <move>\nMove example: e1g1 or e7e8q\n", argv[0]);
        return 2;
    }
    const char *fen = argv[1];
    const char *mv = argv[2];
    Position pos;
    char err[256];
    if (position_from_fen(&pos, fen, err, sizeof err) != POS_OK) {
        fprintf(stderr, "FEN parse failed: %s\n", err);
        return 3;
    }
    int from = coord_to_sq(mv);
    int to = coord_to_sq(mv+2);
    int promo = 0;
    if (strlen(mv) >= 5) promo = promo_from_char(mv[4]);
    MoveUndo undo;
    make_move(&pos, from, to, promo, &undo);
    char out[512];
    position_to_fen(&pos, out, sizeof out);
    printf("%s\n", out);
    return 0;
}
