#include "search.h"
#include "eval.h"
#include "movegen.h"
#include "position.h"
#include "hash.h"
#include "tt.h"
#include "search_order.h"
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

typedef struct {
    int from;
    int to;
    int promo;
    int score;
} MoveScore;

static int cmp_moves_desc(const void *a, const void *b)
{
    const MoveScore *A = a;
    const MoveScore *B = b;
    return (B->score - A->score);
}

static void reorder_moves(int *froms, int *tos, int *promos, int n, const Position *pos, int ply)
{
    if (!froms || n <= 1 || !pos) return;

    MoveScore *arr = malloc(sizeof(MoveScore) * n);
    if (!arr) return;

    int *scores = malloc(sizeof(int) * n);
    if (!scores) { free(arr); return; }

    score_moves_from(pos, froms, tos, promos, n, ply, scores);

    for (int i = 0; i < n; ++i) {
        arr[i].from = froms[i];
        arr[i].to = tos[i];
        arr[i].promo = promos ? promos[i] : 0;
        arr[i].score = scores[i];
    }

    qsort(arr, n, sizeof(MoveScore), cmp_moves_desc);

    for (int i = 0; i < n; ++i) {
        froms[i] = arr[i].from;
        tos[i]   = arr[i].to;
        if (promos) promos[i] = arr[i].promo;
    }

    free(arr);
    free(scores);
}

static int search_ab(Position *pos, int depth, int alpha, int beta)
{
    if (depth <= 0) {
        return evaluate(pos);
    }

    uint64_t key = pos->hash;
    int tt_from = 0, tt_to = 0, tt_promo = 0, tt_val = 0;
    if (tt_probe(key, depth, alpha, beta, &tt_val, &tt_from, &tt_to, &tt_promo)) {
        return tt_val;
    }

    int capacity = 1024;
    int *froms = malloc(sizeof(int) * capacity);
    int *tos = malloc(sizeof(int) * capacity);
    int *promos = malloc(sizeof(int) * capacity);
    if (!froms || !tos || !promos) {
        free(froms); free(tos); free(promos);
        return evaluate(pos);
    }

    int n = generate_legal_moves(pos, froms, tos, promos, capacity);
    reorder_moves(froms, tos, promos, n, pos, depth);

    if (tt_from >= 0 && n > 0) {
        for (int i = 0; i < n; ++i) {
            if (froms[i] == tt_from && tos[i] == tt_to && promos[i] == tt_promo) {
                if (i != 0) {
                    int tf = froms[0], tt = tos[0], tp = promos[0];
                    froms[0] = froms[i]; tos[0] = tos[i]; promos[0] = promos[i];
                    froms[i] = tf; tos[i] = tt; promos[i] = tp;
                }
                break;
            }
        }
    }

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
            return 0;
        }
    }

    int best_value = (pos->side_to_move == COLOR_WHITE) ? -INF : INF;
    int best_from = -1, best_to = -1, best_promo = 0;
    int alpha_orig = alpha;
    int beta_orig = beta;

    for (int i = 0; i < n; ++i) {
        MoveUndo undo;
        make_move(pos, froms[i], tos[i], promos[i], &undo);
        int val = search_ab(pos, depth - 1, alpha, beta);
        unmake_move(pos, &undo);

        int mover = (undo.moved_piece > 0) ? COLOR_WHITE : COLOR_BLACK;

        if (mover == COLOR_WHITE) {
            if (val > best_value) {
                best_value = val;
                best_from = froms[i]; best_to = tos[i]; best_promo = promos[i];
                if (undo.captured_piece == PIECE_EMPTY) {
                    update_history_from(pos, froms[i], tos[i], promos[i], depth);
                }
            }
            if (val > alpha) alpha = val;
        } else {
            if (val < best_value) {
                best_value = val;
                best_from = froms[i]; best_to = tos[i]; best_promo = promos[i];
                if (undo.captured_piece == PIECE_EMPTY) {
                    update_history_from(pos, froms[i], tos[i], promos[i], depth);
                }
            }
            if (val < beta) beta = val;
        }

        if (alpha >= beta) {
            if (undo.captured_piece == PIECE_EMPTY) {
                update_killers_from(depth, froms[i], tos[i], promos[i]);
            }
            break;
        }
    }

    tt_flag_t flag = TT_FLAG_EXACT;
    if (best_value <= alpha_orig) flag = TT_FLAG_UPPER;
    else if (best_value >= beta_orig) flag = TT_FLAG_LOWER;

    tt_store(key, best_value, depth, flag, best_from, best_to, best_promo);

    free(froms); free(tos); free(promos);
    return best_value;
}

static int search_root_once(Position *pos, int depth, int *out_from, int *out_to, int *out_promotion)
{
    if (depth <= 0) return 0;
    uint64_t key = pos->hash;
    int tt_from = 0, tt_to = 0, tt_promo = 0, tt_val = 0;
    (void)tt_probe(key, depth, -INF, INF, &tt_val, &tt_from, &tt_to, &tt_promo);
    int capacity = 4096;
    int *froms = malloc(sizeof(int) * capacity);
    int *tos = malloc(sizeof(int) * capacity);
    int *promos = malloc(sizeof(int) * capacity);
    if (!froms || !tos || !promos) { free(froms); free(tos); free(promos); return 0; }
    int n = generate_legal_moves(pos, froms, tos, promos, capacity);
    reorder_moves(froms, tos, promos, n, pos, depth);
    if (tt_from >= 0 && n > 0) {
        for (int i = 0; i < n; ++i) {
            if (froms[i] == tt_from && tos[i] == tt_to && promos[i] == tt_promo) {
                if (i != 0) {
                    int tf = froms[0], tt = tos[0], tp = promos[0];
                    froms[0] = froms[i]; tos[0] = tos[i]; promos[0] = promos[i];
                    froms[i] = tf; tos[i] = tt; promos[i] = tp;
                }
                break;
            }
        }
    }
    if (n == 0) { free(froms); free(tos); free(promos); return 0; }
    int best_from = -1, best_to = -1, best_promo = 0;
    int best_score = (pos->side_to_move == COLOR_WHITE) ? -INF : INF;
    int alpha = -INF, beta = INF;
    for (int i = 0; i < n; ++i) {
        MoveUndo undo;
        make_move(pos, froms[i], tos[i], promos[i], &undo);
        int val = search_ab(pos, depth - 1, alpha, beta);
        unmake_move(pos, &undo);
        int mover = (undo.moved_piece > 0) ? COLOR_WHITE : COLOR_BLACK;
        if (mover == COLOR_WHITE) {
            if (val > best_score) {
                best_score = val;
                best_from = froms[i];
                best_to = tos[i];
                best_promo = promos[i];
                if (undo.captured_piece == PIECE_EMPTY) {
                    update_history_from(pos, froms[i], tos[i], promos[i], depth);
                }
            }
            if (val > alpha) alpha = val;
        } else {
            if (val < best_score) {
                best_score = val;
                best_from = froms[i];
                best_to = tos[i];
                best_promo = promos[i];
                if (undo.captured_piece == PIECE_EMPTY) {
                    update_history_from(pos, froms[i], tos[i], promos[i], depth);
                }
            }
            if (val < beta) beta = val;
        }
        if (alpha >= beta) break;
    }
    tt_store(key, best_score, depth, TT_FLAG_EXACT, best_from, best_to, best_promo);
    free(froms); free(tos); free(promos);
    if (best_from >= 0) {
        if (out_from) *out_from = best_from;
        if (out_to) *out_to = best_to;
        if (out_promotion) *out_promotion = best_promo;
        return 1;
    }
    return 0;
}

int search_root(Position *pos, int depth, int *out_from, int *out_to, int *out_promotion)
{
    if (depth <= 0) return 0;
    order_init(depth + 8);
    int final_from = -1, final_to = -1, final_promo = 0;
    for (int d = 1; d <= depth; ++d) {
        int tmp_from = -1, tmp_to = -1, tmp_promo = 0;
        int ok = search_root_once(pos, d, &tmp_from, &tmp_to, &tmp_promo);
        if (ok) {
            final_from = tmp_from;
            final_to = tmp_to;
            final_promo = tmp_promo;
        }
    }
    order_free();
    if (out_from) *out_from = final_from;
    if (out_to) *out_to = final_to;
    if (out_promotion) *out_promotion = final_promo;
    return (final_from >= 0) ? 1 : 0;
}
