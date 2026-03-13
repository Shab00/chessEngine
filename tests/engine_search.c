#include "position.h"
#include "hash.h"
#include "search.h"
#include "tt.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
    if (argc < 3) {
        fprintf(stderr, "usage: %s <FEN> <depth>\n", argv[0]);
        return 2;
    }
    const char *fen = argv[1];
    int depth = atoi(argv[2]);
    Position pos;
    char err[256];
    if (position_from_fen(&pos, fen, err, sizeof err) != POS_OK) {
        fprintf(stderr, "bad fen: %s\n", err);
        return 2;
    }

    zobrist_init(0xC0FFEE123456789ULL);

    /* Initialize TT from environment TT_SIZE_MB or use a default (32 MB). */
    const char *tt_env = getenv("TT_SIZE_MB");
    if (tt_env) {
        size_t tt_mb = (size_t)atoi(tt_env);
        if (tt_mb > 0) tt_init(tt_mb);
        else tt_init(32);
    } else {
        tt_init(32);
    }
    /* Start with zeroed stats for this run. */
    tt_stats_reset();

    int from, to, promo;
    int ok = search_iterative_deepening(&pos, depth, &from, &to, &promo);
    if (!ok) {
        fprintf(stdout, "no legal move found\n");
        tt_stats_print(stdout);
        tt_free();
        return 0;
    }
    char from_s[4], to_s[4];
    position_square_to_coords(from, from_s, sizeof from_s);
    position_square_to_coords(to, to_s, sizeof to_s);
    if (promo != 0) {
        printf("best: %s->%s promote=%d\n", from_s, to_s, promo);
        tt_stats_print(stdout);
    } else {
        printf("best: %s->%s\n", from_s, to_s);
        tt_stats_print(stdout);
    }

    tt_free();
    return 0;
}
