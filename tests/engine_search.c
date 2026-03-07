#include "position.h"
#include "hash.h"
#include "search.h"
#include "hash.h"
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
    tt_init(16); /* 16 MB TT to start */

    int from, to, promo;
    int ok = search_iterative_deepening(&pos, depth, &from, &to, &promo);
    if (!ok) {
        fprintf(stdout, "no legal move found\n");
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
