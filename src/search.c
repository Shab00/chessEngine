#include "search.h"
#include "eval.h"
#include "movegen.h"
#include "position.h"
#include <limits.h>
#include <stdlib.h>
#include <string.h>

int is_square_attacked(const Position *pos, int sq, int by);

static int find_king(const Position *pos, int color)
{
    for (int i = 0; i < 64; ++i) {
        int8_t v = pos->board[i];
        if (color == COLOR_WHITE && v == PIECE_KING) return i;
        if (color == COLOR_BLACK && v == -PIECE_KING) return i;
    }
    return POS_NO_SQUARE;
}

#define MATE_SCORE 100000
#define INF 1000000000

static int search_ab(Position *pos, int depth, int alpha, int beta)
{
    if (depth <= 0) {
        return evaluate(pos);
    }

    /* generate moves */
    int capacity = 1024;
    int *froms = malloc(sizeof(int) * capacity);
    int *tos = malloc(sizeof(int) * capacity);
    int *promos = malloc(sizeof(int) * capacity);
    if (!froms || !tos || !promos) {
        free(froms); free(tos); free(promos);
        return evaluate(pos);
    }

    int n = generate_legal_moves(pos, froms, tos, promos, capacity);
    if (n == 0) {
        int king_sq = find_king(pos, pos->side_to_move);
        int in_check = 0;
        if (king_sq != POS_NO_SQUARE) {
            in_check = is_square_attacked(pos, king_sq,
                                          (pos->side_to_move == COLOR_WHITE) ? COLOR_BLACK : COLOR_WHITE);
        }
        free(froms); free(tos); free(promos);
        if (in_check) {
            return (pos->side_to_move == COLOR_WHITE) ? -MATE_SCORE : MATE_SCORE;
        } else {
            /* stalemate */
            return 0;
        }
    }

    int best = (pos->side_to_move == COLOR_WHITE) ? -INF : INF;

    for (int i = 0; i < n; ++i) {
        MoveUndo undo;
        make_move(pos, froms[i], tos[i], promos[i], &undo);
        int val = search_ab(pos, depth - 1, alpha, beta);
        unmake_move(pos, &undo);

        if (pos->side_to_move == COLOR_WHITE) {
            /* After unmake, side_to_move is restored to original side; but previous call
               made a move and toggled it, we evaluate from current position's perspective.
               We'll treat the search value as White-perspective always. */
        }

        /* value is White-perspective; maximize for White, minimize for Black */
        if (pos->side_to_move == COLOR_WHITE) {
            /* when we generated moves, pos->side_to_move was the side to move BEFORE making the move.
               The variable pos->side_to_move after unmake is same as before; we need to determine
               which side made the move to know whether to maximize or minimize. Simpler: use the
               sign of value relative to White and perform conventional alpha-beta: at root we
               maximize for side that was to move at call site. For internal nodes, we can flip logic
               by tracking side_to_move before making moves, but simpler approach:
               We'll implement search to always treat the calling side as side to move by reading pos->side_to_move
               BEFORE making moves. To avoid confusion, restructure by checking the side variable at the top.
            */
        }

        /* For correctness we should have known side_to_move at the node where moves were generated.
           To do this simply: compute side variable as the side that was to move when moves were generated.
        */
        /* Actually we can compute that by looking at undo.moved_piece sign: moved_piece >0 means white moved */
        int mover = (undo.moved_piece > 0) ? COLOR_WHITE : COLOR_BLACK;

        /* If mover was WHITE then after the move, the search evaluated the position where side_to_move is BLACK.
           The returned val is White-perspective; updating best for root node: since the side that moved was mover,
           we want to maximize if mover==WHITE (White made the move), minimize if mover==BLACK. */
        if (mover == COLOR_WHITE) {
            if (val > best) best = val;
            if (val > alpha) alpha = val;
        } else {
            if (val < best) best = val;
            if (val < beta) beta = val;
        }

        if (alpha >= beta) {
            break;
        }
    }

    free(froms); free(tos); free(promos);
    return best;
}

/* Root search: iterate moves at root to pick best move */
int search_root(Position *pos, int depth, int *out_from, int *out_to, int *out_promotion)
{
    if (depth <= 0) return 0;

    int capacity = 4096;
    int *froms = malloc(sizeof(int) * capacity);
    int *tos = malloc(sizeof(int) * capacity);
    int *promos = malloc(sizeof(int) * capacity);
    if (!froms || !tos || !promos) { free(froms); free(tos); free(promos); return 0; }

    int n = generate_legal_moves(pos, froms, tos, promos, capacity);
    if (n == 0) {
        free(froms); free(tos); free(promos);
        return 0;
    }

    int best_from = -1, best_to = -1, best_promo = 0;
    int best_score = (pos->side_to_move == COLOR_WHITE) ? -INF : INF;
    int alpha = -INF, beta = INF;

    for (int i = 0; i < n; ++i) {
        MoveUndo undo;
        make_move(pos, froms[i], tos[i], promos[i], &undo);
        int val = search_ab(pos, depth - 1, alpha, beta);
        unmake_move(pos, &undo);

        if (pos->side_to_move == COLOR_WHITE) {
            /* same note as in search_ab: determine mover from undo.moved_piece sign */
        }
        int mover = (undo.moved_piece > 0) ? COLOR_WHITE : COLOR_BLACK;
        if (mover == COLOR_WHITE) {
            if (val > best_score) {
                best_score = val;
                best_from = froms[i];
                best_to = tos[i];
                best_promo = promos[i];
            }
            if (val > alpha) alpha = val;
        } else {
            if (val < best_score) {
                best_score = val;
                best_from = froms[i];
                best_to = tos[i];
                best_promo = promos[i];
            }
            if (val < beta) beta = val;
        }
        if (alpha >= beta) break;
    }

    free(froms); free(tos); free(promos);

    if (best_from >= 0) {
        if (out_from) *out_from = best_from;
        if (out_to) *out_to = best_to;
        if (out_promotion) *out_promotion = best_promo;
        return 1;
    }
    return 0;
}
