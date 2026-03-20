#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "position.h"
#include "movegen.h"
#include "tt.h"

/* ------------------------------------------------------------------ */
/*  perft_debug internals                                               */
/* ------------------------------------------------------------------ */

typedef struct {
    uint64_t nodes;
    uint64_t captures;
    uint64_t ep;
    uint64_t castles;
    uint64_t promotions;
    uint64_t checks;
} PerftResult;

static void result_add(PerftResult *dst, const PerftResult *src)
{
    dst->nodes      += src->nodes;
    dst->captures   += src->captures;
    dst->ep         += src->ep;
    dst->castles    += src->castles;
    dst->promotions += src->promotions;
    dst->checks     += src->checks;
}

static void perft_debug_r(Position *pos, int depth, PerftResult *result)
{
    int max_moves = 256;
    int *from = malloc(sizeof(int) * max_moves);
    int *to   = malloc(sizeof(int) * max_moves);
    int *prom = malloc(sizeof(int) * max_moves);

    int n = generate_legal_moves(pos, from, to, prom, max_moves);

    if (depth == 1) {
        for (int i = 0; i < n; ++i) {
            result->nodes++;

            int is_ep = (piece_abs(pos->board[from[i]]) == PIECE_PAWN)
                     && (to[i] == pos->en_passant)
                     && (pos->en_passant != POS_NO_SQUARE);

            if (pos->board[to[i]] != PIECE_EMPTY || is_ep) result->captures++;
            if (is_ep)                                      result->ep++;

            if (piece_abs(pos->board[from[i]]) == PIECE_KING &&
                abs((to[i] % 8) - (from[i] % 8)) == 2)
                result->castles++;

            if (prom[i] != 0) result->promotions++;

            MoveUndo undo;
            make_move(pos, from[i], to[i], prom[i], &undo);
            if (position_king_in_check(pos, pos->side_to_move))
                result->checks++;
            unmake_move(pos, &undo);
        }
    } else {
        for (int i = 0; i < n; ++i) {
            MoveUndo undo;
            make_move(pos, from[i], to[i], prom[i], &undo);
            PerftResult child = {0};
            perft_debug_r(pos, depth - 1, &child);
            result_add(result, &child);
            unmake_move(pos, &undo);
        }
    }

    free(from);
    free(to);
    free(prom);
}

/* ------------------------------------------------------------------ */
/*  Public perft_debug entry point                                      */
/* ------------------------------------------------------------------ */

uint64_t perft_debug(Position *pos, int depth)
{
    printf("\nRunning instrumented perft at depth %d...\n\n", depth);

    PerftResult total = {0};
    int max_moves = 256;
    int *from = malloc(sizeof(int) * max_moves);
    int *to   = malloc(sizeof(int) * max_moves);
    int *prom = malloc(sizeof(int) * max_moves);

    int n = generate_legal_moves(pos, from, to, prom, max_moves);

    printf("%-8s  %12s  %9s  %7s  %9s  %10s  %8s\n",
           "Move", "Nodes", "Captures", "EP", "Castles", "Promotions", "Checks");
    printf("%-8s  %12s  %9s  %7s  %9s  %10s  %8s\n",
           "--------", "------------", "---------", "-------",
           "---------", "----------", "--------");

    for (int i = 0; i < n; ++i) {
        MoveUndo undo;
        make_move(pos, from[i], to[i], prom[i], &undo);

        PerftResult child = {0};
        if (depth == 1) {
            child.nodes = 1;
            int moved_type = piece_abs(pos->board[to[i]]);
            int is_ep = (moved_type == PIECE_PAWN)
                     && (from[i] % 8 != to[i] % 8)
                     && (undo.captured_piece == PIECE_EMPTY);
            if (undo.captured_piece != PIECE_EMPTY || is_ep) child.captures++;
            if (is_ep)                                        child.ep++;
            if (moved_type == PIECE_KING && abs((to[i]%8)-(from[i]%8)) == 2)
                child.castles++;
            if (prom[i] != 0) child.promotions++;
            if (position_king_in_check(pos, pos->side_to_move)) child.checks++;
        } else {
            perft_debug_r(pos, depth - 1, &child);
        }

        char mv[8];
        char fsq[4], tsq[4];
        fsq[0] = 'a' + (from[i] % 8); fsq[1] = '1' + (from[i] / 8); fsq[2] = 0;
        tsq[0] = 'a' + (to[i]   % 8); tsq[1] = '1' + (to[i]   / 8); tsq[2] = 0;
        snprintf(mv, sizeof(mv), "%s%s", fsq, tsq);
        if (prom[i]) { const char *pp = "?pnbrqk"; mv[4] = pp[prom[i]]; mv[5] = 0; }

        printf("%-8s  %12llu  %9llu  %7llu  %9llu  %10llu  %8llu\n",
               mv,
               (unsigned long long)child.nodes,
               (unsigned long long)child.captures,
               (unsigned long long)child.ep,
               (unsigned long long)child.castles,
               (unsigned long long)child.promotions,
               (unsigned long long)child.checks);

        result_add(&total, &child);
        unmake_move(pos, &undo);
    }

    printf("%-8s  %12s  %9s  %7s  %9s  %10s  %8s\n",
           "--------", "------------", "---------", "-------",
           "---------", "----------", "--------");
    printf("%-8s  %12llu  %9llu  %7llu  %9llu  %10llu  %8llu\n",
           "TOTAL",
           (unsigned long long)total.nodes,
           (unsigned long long)total.captures,
           (unsigned long long)total.ep,
           (unsigned long long)total.castles,
           (unsigned long long)total.promotions,
           (unsigned long long)total.checks);

    printf("\nReference (Kiwipete depth %d):\n", depth);
    switch (depth) {
        case 1: printf("  Nodes:48        Caps:8      EP:0    Castles:2       Promos:0      Checks:0\n");     break;
        case 2: printf("  Nodes:2039      Caps:351    EP:1    Castles:91      Promos:0      Checks:3\n");     break;
        case 3: printf("  Nodes:97862     Caps:17102  EP:45   Castles:3162    Promos:0      Checks:993\n");   break;
        case 4: printf("  Nodes:4085603   Caps:757163 EP:1929 Castles:128013  Promos:15172  Checks:25523\n"); break;
        case 5: printf("  Nodes:193690690 Caps:35043416 EP:73365 Castles:4993637 Promos:8392 Checks:3309887\n"); break;
        default: printf("  (no reference stored for depth %d)\n", depth); break;
    }

    printf("\nDelta from reference (+ = over, - = under):\n");
    if (depth == 1) {
        printf("  Nodes:      %+lld\n", (long long)total.nodes      - 48LL);
        printf("  Captures:   %+lld\n", (long long)total.captures   - 8LL);
        printf("  EP:         %+lld\n", (long long)total.ep         - 0LL);
        printf("  Castles:    %+lld\n", (long long)total.castles    - 2LL);
        printf("  Promotions: %+lld\n", (long long)total.promotions - 0LL);
        printf("  Checks:     %+lld\n", (long long)total.checks     - 0LL);
    }
    if (depth == 2) {
        printf("  Nodes:      %+lld\n", (long long)total.nodes      - 2039LL);
        printf("  Captures:   %+lld\n", (long long)total.captures   - 351LL);
        printf("  EP:         %+lld\n", (long long)total.ep         - 1LL);
        printf("  Castles:    %+lld\n", (long long)total.castles    - 91LL);
        printf("  Promotions: %+lld\n", (long long)total.promotions - 0LL);
        printf("  Checks:     %+lld\n", (long long)total.checks     - 3LL);
    }
    if (depth == 3) {
        printf("  Nodes:      %+lld\n", (long long)total.nodes      - 97862LL);
        printf("  Captures:   %+lld\n", (long long)total.captures   - 17102LL);
        printf("  EP:         %+lld\n", (long long)total.ep         - 45LL);
        printf("  Castles:    %+lld\n", (long long)total.castles    - 3162LL);
        printf("  Promotions: %+lld\n", (long long)total.promotions - 0LL);
        printf("  Checks:     %+lld\n", (long long)total.checks     - 993LL);
    }
    if (depth == 4) {
        printf("  Nodes:      %+lld\n", (long long)total.nodes      - 4085603LL);
        printf("  Captures:   %+lld\n", (long long)total.captures   - 757163LL);
        printf("  EP:         %+lld\n", (long long)total.ep         - 1929LL);
        printf("  Castles:    %+lld\n", (long long)total.castles    - 128013LL);
        printf("  Promotions: %+lld\n", (long long)total.promotions - 15172LL);
        printf("  Checks:     %+lld\n", (long long)total.checks     - 25523LL);
    }
    if (depth == 5) {
        printf("  Nodes:      %+lld\n", (long long)total.nodes      - 193690690LL);
        printf("  Captures:   %+lld\n", (long long)total.captures   - 35043416LL);
        printf("  EP:         %+lld\n", (long long)total.ep         - 73365LL);
        printf("  Castles:    %+lld\n", (long long)total.castles    - 4993637LL);
        printf("  Promotions: %+lld\n", (long long)total.promotions - 8392LL);
        printf("  Checks:     %+lld\n", (long long)total.checks     - 3309887LL);
    }

    free(from); free(to); free(prom);
    return total.nodes;
}

/* ------------------------------------------------------------------ */
/*  main                                                                */
/* ------------------------------------------------------------------ */

static double now_seconds(void) {
    return (double)clock() / (double)CLOCKS_PER_SEC;
}

int main(int argc, char **argv)
{
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <FEN> <depth> [expected]\n", argv[0]);
        return 2;
    }
    const char *fen = argv[1];
    int depth = atoi(argv[2]);
    unsigned long long expected = 0;
    int have_expected = 0;
    if (argc >= 4) {
        expected = strtoull(argv[3], NULL, 10);
        have_expected = 1;
    }

    Position pos;
    char err[256];
    if (position_from_fen(&pos, fen, err, sizeof err) != POS_OK) {
        fprintf(stderr, "position_from_fen failed: %s\n", err);
        return 3;
    }

    /* Print position so we can verify the right FEN was loaded */
    position_print_ascii(&pos, stdout);
    char loaded[128];
    position_to_fen(&pos, loaded, sizeof loaded);
    printf("Loaded: %s\n\n", loaded);

    const char *tt_env = getenv("TT_SIZE_MB");
    if (tt_env) {
        size_t tt_mb = (size_t)atoi(tt_env);
        tt_init(tt_mb > 0 ? tt_mb : 32);
    } else {
        tt_init(32);
    }
    tt_stats_reset();

    double t0 = now_seconds();
    uint64_t nodes = perft_debug(&pos, depth);
    double t1 = now_seconds();

    printf("\nFEN: %s\n", fen);
    printf("Depth: %d  Nodes: %llu  Time: %.3fs  nps: %.0f\n",
           depth, (unsigned long long)nodes, t1 - t0,
           (t1 - t0) > 0.0 ? (double)nodes / (t1 - t0) : (double)nodes);

    tt_stats_print(stdout);

    if (have_expected) {
        if (nodes == expected) {
            printf("Result: OK (matches expected %llu)\n", (unsigned long long)expected);
            tt_free();
            return 0;
        } else {
            printf("Result: MISMATCH (expected %llu)\n", (unsigned long long)expected);
            tt_free();
            return 4;
        }
    }

    tt_free();
    return 0;
}
