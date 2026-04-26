#include "search.h"
#include "eval.h"
#include "movegen.h"
#include "search_context.h"
#include "position.h"
#include "hash.h"
#include "tt.h"
#include "search_order.h"
#include <limits.h>
#include <stdlib.h>
#include <string.h>

#define DEBUG_ROOT_MOVES 0
#define is_square_attacked(pos, sq, by) position_is_square_attacked((pos), (sq), (by))
#define MATE_SCORE  100000
#define INF        1000000000

/* ------------------------------------------------------------------
 * evaluate_for_stm
 * ------------------------------------------------------------------ */
static inline int evaluate_for_stm(const Position *pos)
{
    int score = evaluate(pos);
    return (pos->side_to_move == COLOR_WHITE) ? score : -score;
}

/* ------------------------------------------------------------------
 * find_king
 * ------------------------------------------------------------------ */
static int find_king(const Position *pos, int color)
{
    for (int i = 0; i < 64; ++i) {
        int8_t v = pos->board[i];
        if (color == COLOR_WHITE && v ==  PIECE_KING) return i;
        if (color == COLOR_BLACK && v == -PIECE_KING) return i;
    }
    return POS_NO_SQUARE;
}

/* ------------------------------------------------------------------
 * Move ordering helpers
 * ------------------------------------------------------------------ */
typedef struct { int from, to, promo, score; } MoveScore;

static int cmp_moves_desc(const void *a, const void *b)
{
    return ((const MoveScore *)b)->score - ((const MoveScore *)a)->score;
}

static void reorder_moves(int *froms, int *tos, int *promos, int n,
                           const Position *pos, int ply)
{
    if (!froms || n <= 1 || !pos) return;
    MoveScore *arr    = malloc(sizeof(MoveScore) * n);
    int       *scores = malloc(sizeof(int)       * n);
    if (!arr || !scores) { free(arr); free(scores); return; }

    score_moves_from(pos, froms, tos, promos, n, ply, scores);
    for (int i = 0; i < n; ++i) {
        arr[i].from  = froms[i];
        arr[i].to    = tos[i];
        arr[i].promo = promos ? promos[i] : 0;
        arr[i].score = scores[i];
    }
    qsort(arr, n, sizeof(MoveScore), cmp_moves_desc);
    for (int i = 0; i < n; ++i) {
        froms[i]  = arr[i].from;
        tos[i]    = arr[i].to;
        if (promos) promos[i] = arr[i].promo;
    }
    free(arr);
    free(scores);
}

/* ------------------------------------------------------------------
 * promote_tt_move_to_front
 *
 * Only promotes the TT move if it actually exists in the legal move
 * list. This prevents hash collisions from causing illegal moves to
 * be played — a stale TT entry from a different position with the
 * same Zobrist key would otherwise bypass legality filtering.
 * ------------------------------------------------------------------ */
static void promote_tt_move_to_front(int *froms, int *tos, int *promos,
                                      int n, int tt_from, int tt_to,
                                      int tt_promo)
{
    /* FIX: verify the TT move is actually in the generated legal move
       list before promoting it. Without this check a hash collision
       can produce a move from an empty square or into an illegal
       square — exactly the "piece disappeared" symptom. */
    for (int i = 1; i < n; ++i) {
        if (froms[i] == tt_from && tos[i] == tt_to && promos[i] == tt_promo) {
            int tf = froms[0], tt2 = tos[0], tp = promos[0];
            froms[0] = froms[i]; tos[0] = tos[i]; promos[0] = promos[i];
            froms[i] = tf;       tos[i] = tt2;    promos[i] = tp;
            break;
            /* Note: if the TT move is NOT found in the list we simply
               do nothing — the list stays in its reordered state and
               the stale TT move is silently ignored. */
        }
    }
}

/* ------------------------------------------------------------------
 * quiescence
 * ------------------------------------------------------------------ */
static int quiescence(Position *pos, int alpha, int beta, SearchContext *ctx)
{
    if (search_context_should_stop(ctx)) return 0;

    ctx->nodes++;

    int capacity = 128;
    int *froms  = malloc(sizeof(int) * capacity);
    int *tos    = malloc(sizeof(int) * capacity);
    int *promos = malloc(sizeof(int) * capacity);
    if (!froms || !tos || !promos) {
        free(froms); free(tos); free(promos);
        return evaluate_for_stm(pos);
    }

    int n = generate_legal_moves(pos, froms, tos, promos, capacity);

    /* Detect checkmate / stalemate */
    if (n == 0) {
        free(froms); free(tos); free(promos);
        int king_sq = find_king(pos, pos->side_to_move);
        if (king_sq == POS_NO_SQUARE) {
            return -MATE_SCORE;
        }
        int opp = (pos->side_to_move == COLOR_WHITE) ? COLOR_BLACK : COLOR_WHITE;
        if (position_is_square_attacked(pos, king_sq, opp)) {
            return -MATE_SCORE;
        }
        return 0;
    }

    int stand_pat = evaluate_for_stm(pos);
    if (stand_pat >= beta) {
        free(froms); free(tos); free(promos);
        return beta;
    }
    if (stand_pat > alpha) alpha = stand_pat;

    for (int i = 0; i < n; ++i) {
        int is_capture = (pos->board[tos[i]] != PIECE_EMPTY);
        int is_ep      = (piece_abs(pos->board[froms[i]]) == PIECE_PAWN)
                       && (tos[i] == pos->en_passant)
                       && (pos->en_passant != POS_NO_SQUARE);
        int is_promo   = (promos[i] != 0);
        if (!is_capture && !is_ep && !is_promo) continue;

        MoveUndo undo;
        make_move(pos, froms[i], tos[i], promos[i], &undo);
        int score = -quiescence(pos, -beta, -alpha, ctx);
        unmake_move(pos, &undo);

        if (score >= beta) {
            free(froms); free(tos); free(promos);
            return beta;
        }
        if (score > alpha) alpha = score;

        if (search_context_should_stop(ctx)) break;
    }

    free(froms); free(tos); free(promos);
    return alpha;
}

/* ------------------------------------------------------------------
 * search_ab
 * ------------------------------------------------------------------ */
static int search_ab(Position *pos, int depth, int ply, int alpha, int beta,
                     SearchContext *ctx)
{
    if (search_context_should_stop(ctx)) return 0;

    if (depth <= 0)
        return quiescence(pos, alpha, beta, ctx);

    ctx->nodes++;

    /* TT probe */
    uint64_t key = pos->hash;
    int tt_from = -1, tt_to = -1, tt_promo = 0, tt_val = 0;
    if (tt_probe(key, depth, alpha, beta, &tt_val, &tt_from, &tt_to, &tt_promo))
        return tt_val;

    /* In-check detection */
    int king_sq  = find_king(pos, pos->side_to_move);
    int in_check = 0;
    if (king_sq != POS_NO_SQUARE) {
        int opp = (pos->side_to_move == COLOR_WHITE) ? COLOR_BLACK : COLOR_WHITE;
        in_check = is_square_attacked(pos, king_sq, opp);
    }

    /* Null-move pruning.
       FIX: use NullMoveUndo — the correct type for make/unmake_null_move.
       Previously MoveUndo was used here which is a different struct and
       caused undefined behaviour when unmake_null_move read back fields
       at the wrong offsets. */
    if (!in_check && depth >= 3 && position_material(pos) > 16) {
        NullMoveUndo null_undo;                  /* FIX: was MoveUndo */
        make_null_move(pos, &null_undo);
        int R = 2;
        int null_val = -search_ab(pos, depth - 1 - R, ply + 1,
                                  -beta, -beta + 1, ctx);
        unmake_null_move(pos, &null_undo);
        if (!search_context_should_stop(ctx) && null_val >= beta)
            return beta;
    }

    /* Generate and order moves */
    int capacity = 1024;
    int *froms  = malloc(sizeof(int) * capacity);
    int *tos    = malloc(sizeof(int) * capacity);
    int *promos = malloc(sizeof(int) * capacity);
    if (!froms || !tos || !promos) {
        free(froms); free(tos); free(promos);
        return evaluate_for_stm(pos);
    }

    int n = generate_legal_moves(pos, froms, tos, promos, capacity);
    reorder_moves(froms, tos, promos, n, pos, ply);

    /* FIX: promote_tt_move_to_front now only promotes if the TT move
       exists in the legal list, so the tt_from >= 0 guard here is the
       only extra safety needed — no separate loop required. */
    if (tt_from >= 0 && n > 0)
        promote_tt_move_to_front(froms, tos, promos, n, tt_from, tt_to, tt_promo);

    /* Terminal node */
    if (n == 0) {
        free(froms); free(tos); free(promos);
        if (king_sq == POS_NO_SQUARE) return -MATE_SCORE + ply;
        return in_check ? (-MATE_SCORE + ply) : 0;
    }

    int alpha_orig = alpha;
    int best_value = -INF;
    int best_from  = -1, best_to = -1, best_promo = 0;

    for (int i = 0; i < n; ++i) {
        if (search_context_should_stop(ctx)) break;

        MoveUndo undo;
        make_move(pos, froms[i], tos[i], promos[i], &undo);

        /* Check extension */
        int gives_check = 0;
        {
            int opp_king_sq = find_king(pos, pos->side_to_move);
            if (opp_king_sq != POS_NO_SQUARE) {
                int attacker = (pos->side_to_move == COLOR_WHITE)
                                   ? COLOR_BLACK : COLOR_WHITE;
                gives_check = is_square_attacked(pos, opp_king_sq, attacker);
            }
        }
        int extension = (gives_check && ply < 64) ? 1 : 0;

        int is_capture = (undo.captured_piece != PIECE_EMPTY);
        int is_promo   = (promos[i] != 0);
        int reduction  = 0;
        if (depth >= 3 && i >= 4 && !is_capture && !is_promo
            && !in_check && !gives_check)
            reduction = 1;

        int new_depth = depth - 1 + extension;

        int val;
        if (reduction) {
            val = -search_ab(pos, new_depth - reduction, ply + 1,
                             -alpha - 1, -alpha, ctx);
            if (!search_context_should_stop(ctx) && val > alpha)
                val = -search_ab(pos, new_depth, ply + 1, -beta, -alpha, ctx);
        } else {
            val = -search_ab(pos, new_depth, ply + 1, -beta, -alpha, ctx);
        }

        unmake_move(pos, &undo);

        if (search_context_should_stop(ctx)) break;

        if (val > best_value) {
            best_value = val;
            best_from  = froms[i];
            best_to    = tos[i];
            best_promo = promos[i];
        }
        if (val > alpha) {
            alpha = val;
            if (undo.captured_piece == PIECE_EMPTY)
                update_history_from(pos, froms[i], tos[i], promos[i], depth);
        }
        if (alpha >= beta) {
            if (undo.captured_piece == PIECE_EMPTY)
                update_killers_from(ply, froms[i], tos[i], promos[i]);
            break;
        }
    }

    if (!search_context_should_stop(ctx)) {
        tt_flag_t flag = TT_FLAG_EXACT;
        if      (best_value <= alpha_orig) flag = TT_FLAG_UPPER;
        else if (best_value >= beta)       flag = TT_FLAG_LOWER;
        tt_store(key, best_value, depth, flag, best_from, best_to, best_promo);
    }

    free(froms); free(tos); free(promos);
    return best_value;
}

/* ------------------------------------------------------------------
 * search_root_once
 * ------------------------------------------------------------------ */
static int search_root_once(Position *pos, int depth, int alpha, int beta,
                             int *out_from, int *out_to, int *out_promotion,
                             SearchContext *ctx)
{
    if (depth <= 0) return 0;

    int capacity = 4096;
    int *froms  = malloc(sizeof(int) * capacity);
    int *tos    = malloc(sizeof(int) * capacity);
    int *promos = malloc(sizeof(int) * capacity);
    if (!froms || !tos || !promos) {
        free(froms); free(tos); free(promos);
        return -INF;
    }

    int n = generate_legal_moves(pos, froms, tos, promos, capacity);
    reorder_moves(froms, tos, promos, n, pos, 0);

    int tt_from = -1, tt_to = -1, tt_promo = 0, tt_val = 0;
    tt_probe(pos->hash, depth, alpha, beta, &tt_val, &tt_from, &tt_to, &tt_promo);

    if (tt_from >= 0 && n > 0)
        promote_tt_move_to_front(froms, tos, promos, n, tt_from, tt_to, tt_promo);

    if (n == 0) {
        int king_sq  = find_king(pos, pos->side_to_move);
        int in_check = 0;
        if (king_sq != POS_NO_SQUARE) {
            int opp = (pos->side_to_move == COLOR_WHITE) ? COLOR_BLACK : COLOR_WHITE;
            in_check = is_square_attacked(pos, king_sq, opp);
        }
        free(froms); free(tos); free(promos);
        return in_check ? (-MATE_SCORE + 1) : 0;
    }

    int best_score = -INF;
    int best_from  = -1, best_to = -1, best_promo = 0;

    for (int i = 0; i < n; ++i) {
        if (search_context_should_stop(ctx)) break;

        MoveUndo undo;
        make_move(pos, froms[i], tos[i], promos[i], &undo);
        int val = -search_ab(pos, depth - 1, 1, -beta, -alpha, ctx);
        unmake_move(pos, &undo);

        if (search_context_should_stop(ctx)) break;

        if (val > best_score) {
            best_score = val;
            best_from  = froms[i];
            best_to    = tos[i];
            best_promo = promos[i];
            ctx->best_from  = best_from;
            ctx->best_to    = best_to;
            ctx->best_promo = best_promo;
            ctx->best_score = best_score;
        }
        if (val > alpha) {
            alpha = val;
            if (undo.captured_piece == PIECE_EMPTY)
                update_history_from(pos, froms[i], tos[i], promos[i], depth);
        }
        if (alpha >= beta) break;
    }

    if (!search_context_should_stop(ctx)) {
        tt_store(pos->hash, best_score, depth, TT_FLAG_EXACT,
                 best_from, best_to, best_promo);
    }

    if (out_from)      *out_from      = best_from;
    if (out_to)        *out_to        = best_to;
    if (out_promotion) *out_promotion = best_promo;

    free(froms); free(tos); free(promos);
    return best_score;
}

/* ------------------------------------------------------------------
 * search_root  — iterative deepening + aspiration windows
 * ------------------------------------------------------------------ */
int search_root(Position *pos, int depth, int *out_from, int *out_to,
                int *out_promotion, SearchContext *ctx)
{
    if (depth <= 0) return 0;

    SearchContext local_ctx;
    if (!ctx) {
        search_context_init(&local_ctx, depth, 0);
        ctx = &local_ctx;
    }

    order_init(depth + 8);

    int final_from = -1, final_to = -1, final_promo = 0;
    int prev_score = 0;
    const int ASP_WINDOW = 50;

    int print_root_debug = 0;
    if (DEBUG_ROOT_MOVES) print_root_debug = 1;

    for (int d = 1; d <= depth; ++d) {
        if (search_context_should_stop(ctx)) break;

        int tmp_from = -1, tmp_to = -1, tmp_promo = 0;

        int alpha = prev_score - ASP_WINDOW;
        int beta  = prev_score + ASP_WINDOW;

        int score = search_root_once(pos, d, alpha, beta,
                                     &tmp_from, &tmp_to, &tmp_promo, ctx);

        if (!search_context_should_stop(ctx) &&
            (score <= alpha || score >= beta)) {
            score = search_root_once(pos, d, -INF, INF,
                                     &tmp_from, &tmp_to, &tmp_promo, ctx);
        }

        prev_score = score;

        if (!search_context_should_stop(ctx) && tmp_from != -1) {
            final_from  = tmp_from;
            final_to    = tmp_to;
            final_promo = tmp_promo;
        }

        if (print_root_debug && d == depth) {
            printf("\n[Debug] Root moves and scores at depth %d:\n", depth);

            int cap = 1024, n;
            int *froms  = malloc(sizeof(int) * cap);
            int *tos    = malloc(sizeof(int) * cap);
            int *promos = malloc(sizeof(int) * cap);
            if (froms && tos && promos) {
                n = generate_legal_moves(pos, froms, tos, promos, cap);
                for (int j = 0; j < n; ++j) {
                    MoveUndo undo;
                    make_move(pos, froms[j], tos[j], promos[j], &undo);
                    int val = -search_ab(pos, depth - 1, 1, -INF, INF, ctx);
                    unmake_move(pos, &undo);

                    char src[3], dst[3];
                    src[0] = 'a' + (froms[j] % 8); src[1] = '1' + (froms[j] / 8); src[2] = 0;
                    dst[0] = 'a' + (tos[j]   % 8); dst[1] = '1' + (tos[j]   / 8); dst[2] = 0;

                    printf("    %s%s", src, dst);
                    if (promos[j]) printf("(prom=%d)", promos[j]);
                    printf("  score: %d", val);
                    if (froms[j] == final_from && tos[j] == final_to)
                        printf("  <== engine move");
                    printf("\n");
                }
                printf("[Debug] End root move list.\n\n");
            }
            free(froms); free(tos); free(promos);
        }
    }

    order_free();

    if (final_from < 0 && ctx->best_from >= 0) {
        final_from  = ctx->best_from;
        final_to    = ctx->best_to;
        final_promo = ctx->best_promo;
    }

    if (out_from)      *out_from      = final_from;
    if (out_to)        *out_to        = final_to;
    if (out_promotion) *out_promotion = final_promo;

    return (final_from >= 0) ? 1 : 0;
}
