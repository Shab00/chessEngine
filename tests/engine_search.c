#include "position.h"
#include "movegen.h"
#include "search.h"
#include "tt.h"
#include "hash.h"
#include "search_context.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static double now_seconds(void) {
    return (double)clock() / (double)CLOCKS_PER_SEC;
}

int main(int argc, char **argv)
{
    if (argc < 3) {
        fprintf(stderr, "Usage: %s \"<FEN>\" <depth>\n", argv[0]);
        fprintf(stderr, "Example:\n");
        fprintf(stderr, "  %s \"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1\" 5\n", argv[0]);
        return 2;
    }

    const char *fen   = argv[1];
    int         depth = atoi(argv[2]);

    if (depth <= 0) {
        fprintf(stderr, "depth must be >= 1\n");
        return 2;
    }

    zobrist_init(0);

    Position pos;
    char err[256];
    if (position_from_fen(&pos, fen, err, sizeof err) != POS_OK) {
        fprintf(stderr, "FEN parse failed: %s\n", err);
        return 3;
    }

    position_print_ascii(&pos, stdout);
    char loaded_fen[128];
    position_to_fen(&pos, loaded_fen, sizeof loaded_fen);
    printf("FEN:   %s\n", loaded_fen);
    printf("Depth: %d\n\n", depth);

    const char *tt_env = getenv("TT_SIZE_MB");
    size_t tt_mb = tt_env ? (size_t)atoi(tt_env) : 32;
    if (tt_mb == 0) tt_mb = 32;
    tt_init(tt_mb);
    tt_stats_reset();

    SearchContext ctx;
    search_context_init(&ctx, depth, 0);

    double t0 = now_seconds();
    int from = -1, to = -1, promo = 0;
    int found = search_root(&pos, depth, &from, &to, &promo, &ctx);
    double t1 = now_seconds();

    if (!found || from < 0) {
        printf("Result: no legal move found (checkmate or stalemate)\n");
    } else {
        char from_s[4], to_s[4];
        position_square_to_coords(from, from_s, sizeof from_s);
        position_square_to_coords(to,   to_s,   sizeof to_s);

        if (promo != 0) {
            const char *pnames[] = { "?", "pawn", "knight", "bishop", "rook", "queen", "king" };
            printf("Best move: %s%s (promote to %s)\n",
                   from_s, to_s, pnames[promo < 6 ? promo : 0]);
        } else {
            printf("Best move: %s%s\n", from_s, to_s);
        }
    }

    printf("Time:  %.3fs\n", t1 - t0);
    printf("\n");
    tt_stats_print(stdout);

    tt_free();
    return 0;
}
