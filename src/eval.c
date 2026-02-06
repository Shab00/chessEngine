#include "eval.h"
#include <stdint.h>

/* Simple piece values (centipawns) */
static const int piece_value[7] = {
    0,    /* empty */
    100,  /* pawn */
    320,  /* knight */
    330,  /* bishop */
    500,  /* rook */
    900,  /* queen */
    20000 /* king */
};

/* Small piece-square tables (white perspective). Values are small bonuses in centipawns.
   Index 0..63, file major: a1=0 ... h1=7, a2=8 ... h8=63 (matching SQ_INDEX macro). */

static const int pst_pawn[64] = {
     0,  0,  0,  0,  0,  0,  0,  0,
    50, 50, 50, 50, 50, 50, 50, 50,
    10, 10, 20, 30, 30, 20, 10, 10,
     5,  5, 10, 25, 25, 10,  5,  5,
     0,  0,  0, 20, 20,  0,  0,  0,
     5, -5,-10,  0,  0,-10, -5,  5,
     5, 10, 10,-20,-20, 10, 10,  5,
     0,  0,  0,  0,  0,  0,  0,  0
};

static const int pst_knight[64] = {
   -50,-40,-30,-30,-30,-30,-40,-50,
   -40,-20,  0,  5,  5,  0,-20,-40,
   -30,  5, 10, 15, 15, 10,  5,-30,
   -30,  0, 15, 20, 20, 15,  0,-30,
   -30,  5, 15, 20, 20, 15,  5,-30,
   -30,  0, 10, 15, 15, 10,  0,-30,
   -40,-20,  0,  0,  0,  0,-20,-40,
   -50,-40,-30,-30,-30,-30,-40,-50
};

static const int pst_bishop[64] = {
   -20,-10,-10,-10,-10,-10,-10,-20,
   -10,  5,  0,  0,  0,  0,  5,-10,
   -10, 10, 10, 10, 10, 10, 10,-10,
   -10,  0, 10, 10, 10, 10,  0,-10,
   -10,  5,  5, 10, 10,  5,  5,-10,
   -10,  0,  5, 10, 10,  5,  0,-10,
   -10,  0,  0,  0,  0,  0,  0,-10,
   -20,-10,-10,-10,-10,-10,-10,-20
};

static const int pst_rook[64] = {
     0,  0,  5, 10, 10,  5,  0,  0,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
     5, 10, 10, 10, 10, 10, 10,  5,
     0,  0,  0,  0,  0,  0,  0,  0
};

static const int pst_queen[64] = {
   -20,-10,-10, -5, -5,-10,-10,-20,
   -10,  0,  0,  0,  0,  0,  0,-10,
   -10,  0,  5,  5,  5,  5,  0,-10,
    -5,  0,  5,  5,  5,  5,  0, -5,
     0,  0,  5,  5,  5,  5,  0, -5,
   -10,  5,  5,  5,  5,  5,  0,-10,
   -10,  0,  5,  0,  0,  0,  0,-10,
   -20,-10,-10, -5, -5,-10,-10,-20
};

static const int pst_king[64] = {
   -30,-40,-40,-50,-50,-40,-40,-30,
   -30,-40,-40,-50,-50,-40,-40,-30,
   -30,-40,-40,-50,-50,-40,-40,-30,
   -30,-40,-40,-50,-50,-40,-40,-30,
   -20,-30,-30,-40,-40,-30,-30,-20,
   -10,-20,-20,-20,-20,-20,-20,-10,
    20, 20,  0,  0,  0,  0, 20, 20,
    20, 30, 10,  0,  0, 10, 30, 20
};

static inline int mirror_sq(int sq) { return 63 - sq; }

int evaluate(const Position *pos)
{
    int score = 0;

    for (int sq = 0; sq < 64; ++sq) {
        int8_t v = pos->board[sq];
        if (v == PIECE_EMPTY) continue;
        int sign = (v > 0) ? 1 : -1;
        int a = piece_abs(v);
        int base = 0;
        if (a >= 0 && a <= 6) base = piece_value[a];
        int pst = 0;
        switch (a) {
        case PIECE_PAWN:   pst = (v > 0) ? pst_pawn[sq] : -pst_pawn[mirror_sq(sq)]; break;
        case PIECE_KNIGHT: pst = (v > 0) ? pst_knight[sq] : -pst_knight[mirror_sq(sq)]; break;
        case PIECE_BISHOP: pst = (v > 0) ? pst_bishop[sq] : -pst_bishop[mirror_sq(sq)]; break;
        case PIECE_ROOK:   pst = (v > 0) ? pst_rook[sq] : -pst_rook[mirror_sq(sq)]; break;
        case PIECE_QUEEN:  pst = (v > 0) ? pst_queen[sq] : -pst_queen[mirror_sq(sq)]; break;
        case PIECE_KING:   pst = (v > 0) ? pst_king[sq] : -pst_king[mirror_sq(sq)]; break;
        default: pst = 0; break;
        }
        score += sign * base;
        score += pst;
    }

    /* score is in centipawns (approx) from White's perspective */
    return score;
}
