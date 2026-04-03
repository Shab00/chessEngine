#include "eval.h"
#include <stdint.h>

static const int piece_value[7] = {
    0,    /* empty */
    100,  /* pawn */
    320,  /* knight */
    330,  /* bishop */
    500,  /* rook */
    900,  /* queen */
    20000 /* king */
};


static const int pst_pawn[64] = {
    /* rank 1 (a1..h1) — pawns never here */
     0,  0,  0,  0,  0,  0,  0,  0,
    /* rank 2 */
     5, 10, 10,-20,-20, 10, 10,  5,
    /* rank 3 */
     5, -5,-10,  0,  0,-10, -5,  5,
    /* rank 4 */
     0,  0,  0, 20, 20,  0,  0,  0,
    /* rank 5 */
     5,  5, 10, 25, 25, 10,  5,  5,
    /* rank 6 */
    10, 10, 20, 30, 30, 20, 10, 10,
    /* rank 7 */
    50, 50, 50, 50, 50, 50, 50, 50,
    /* rank 8 — pawns never here */
     0,  0,  0,  0,  0,  0,  0,  0
};

static const int pst_knight[64] = {
    /* rank 1 */
   -50,-40,-30,-30,-30,-30,-40,-50,
    /* rank 2 */
   -40,-20,  0,  0,  0,  0,-20,-40,
    /* rank 3 */
   -30,  0, 10, 15, 15, 10,  0,-30,
    /* rank 4 */
   -30,  5, 15, 20, 20, 15,  5,-30,
    /* rank 5 */
   -30,  0, 15, 20, 20, 15,  0,-30,
    /* rank 6 */
   -30,  5, 10, 15, 15, 10,  5,-30,
    /* rank 7 */
   -40,-20,  0,  5,  5,  0,-20,-40,
    /* rank 8 */
   -50,-40,-30,-30,-30,-30,-40,-50
};

static const int pst_bishop[64] = {
    /* rank 1 */
   -20,-10,-10,-10,-10,-10,-10,-20,
    /* rank 2 */
   -10,  0,  0,  0,  0,  0,  0,-10,
    /* rank 3 */
   -10,  0,  5, 10, 10,  5,  0,-10,
    /* rank 4 */
   -10,  5,  5, 10, 10,  5,  5,-10,
    /* rank 5 */
   -10,  0, 10, 10, 10, 10,  0,-10,
    /* rank 6 */
   -10, 10, 10, 10, 10, 10, 10,-10,
    /* rank 7 */
   -10,  5,  0,  0,  0,  0,  5,-10,
    /* rank 8 */
   -20,-10,-10,-10,-10,-10,-10,-20
};

static const int pst_rook[64] = {
    /* rank 1 */
     0,  0,  0,  0,  0,  0,  0,  0,
    /* rank 2 */
     5, 10, 10, 10, 10, 10, 10,  5,
    /* rank 3 */
    -5,  0,  0,  0,  0,  0,  0, -5,
    /* rank 4 */
    -5,  0,  0,  0,  0,  0,  0, -5,
    /* rank 5 */
    -5,  0,  0,  0,  0,  0,  0, -5,
    /* rank 6 */
    -5,  0,  0,  0,  0,  0,  0, -5,
    /* rank 7 */
    -5,  0,  0,  0,  0,  0,  0, -5,
    /* rank 8 */
     0,  0,  5, 10, 10,  5,  0,  0
};

static const int pst_queen[64] = {
    /* rank 1 */
   -20,-10,-10, -5, -5,-10,-10,-20,
    /* rank 2 */
   -10,  0,  5,  0,  0,  0,  0,-10,
    /* rank 3 */
   -10,  5,  5,  5,  5,  5,  0,-10,
    /* rank 4 */
     0,  0,  5,  5,  5,  5,  0, -5,
    /* rank 5 */
    -5,  0,  5,  5,  5,  5,  0, -5,
    /* rank 6 */
   -10,  0,  5,  5,  5,  5,  0,-10,
    /* rank 7 */
   -10,  0,  0,  0,  0,  0,  0,-10,
    /* rank 8 */
   -20,-10,-10, -5, -5,-10,-10,-20
};

static const int pst_king[64] = {
    /* rank 1 — king wants to castle and stay on back rank */
    20, 30, 10,  0,  0, 10, 30, 20,
    /* rank 2 */
    20, 20,  0,  0,  0,  0, 20, 20,
    /* rank 3 */
   -10,-20,-20,-20,-20,-20,-20,-10,
    /* rank 4 */
   -20,-30,-30,-40,-40,-30,-30,-20,
    /* rank 5 */
   -30,-40,-40,-50,-50,-40,-40,-30,
    /* rank 6 */
   -30,-40,-40,-50,-50,-40,-40,-30,
    /* rank 7 */
   -30,-40,-40,-50,-50,-40,-40,-30,
    /* rank 8 */
   -30,-40,-40,-50,-50,-40,-40,-30
};

static inline int mirror_sq(int sq) { return sq ^ 56; }

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

    return score;
}
